#include "Hardware.h"
#include "serial.h"

#include <windows.h>
#include <tchar.h>
#include <process.h>
#include <string.h>
#include <stdio.h>

//#define SERIAL_USE_OVERLAPPED
//#define DEBUG_SHOW

#define MAX_COMPORT_STR_LEN 32


static HANDLE hComm;
static TS_SERIAL_SETUP serialSetup;
static HANDLE hSerialReadThread;
static unsigned serialReadThreadId;
static HANDLE hExitEvent;

static uint8_t SerialSetup(void);
static unsigned __stdcall SerialReadThreadFunc(void *pArguments);

uint8_t SerialInit(TS_SERIAL_SETUP *setup)
{
    serialSetup = *setup;
    return SerialSetup();
}

void SerialClose(void)
{
    //SetCommMask(hComm, EV_TXEMPTY);
    SetEvent(hExitEvent);
    WaitForSingleObject(hSerialReadThread, INFINITE);

    CloseHandle(hSerialReadThread);
    CloseHandle(hComm);
}

static uint8_t SerialSetup(void)
{
    uint8_t retVal;
    BOOL status;
    DCB dcbSerialParams;
    COMMTIMEOUTS timeouts;
    TCHAR commPortStr[MAX_COMPORT_STR_LEN];
    DWORD baudRate;

    retVal = 1;
    memset(&dcbSerialParams, 0, sizeof(dcbSerialParams));
    memset(&timeouts, 0, sizeof(timeouts));

    _stprintf(commPortStr, TEXT("\\\\.\\COM%d"), serialSetup.comportNum);
    baudRate = serialSetup.baudRate;

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    hComm = CreateFile(commPortStr,                  //port name
                       GENERIC_READ | GENERIC_WRITE, //Read/Write
                       0,                            // No Sharing
                       NULL,                         // No Security
                       OPEN_EXISTING,                // Open existing port only
#ifndef SERIAL_USE_OVERLAPPED
                       0, // Non Overlapped I/O
#else
                       FILE_FLAG_OVERLAPPED, // Overlapped I/O
#endif
                       NULL); // Null for Comm Devices

    if (hComm == INVALID_HANDLE_VALUE)
    {
        // opening serial port failure
    }
    else
    {
        // Opening Serial port success
        status = GetCommState(hComm, &dcbSerialParams);
        dcbSerialParams.BaudRate = baudRate;   // Setting BaudRate = 9600
        dcbSerialParams.ByteSize = 8;          // Setting ByteSize = 8
        dcbSerialParams.StopBits = ONESTOPBIT; // Setting StopBits = 1
        dcbSerialParams.Parity = NOPARITY;     // Setting Parity = None
        SetCommState(hComm, &dcbSerialParams);
        status = GetCommState(hComm, &dcbSerialParams);
        timeouts.ReadIntervalTimeout = 50;         // in milliseconds
        timeouts.ReadTotalTimeoutConstant = 50;    // in milliseconds
        timeouts.ReadTotalTimeoutMultiplier = 10;  // in milliseconds
        timeouts.WriteTotalTimeoutConstant = 50;   // in milliseconds
        timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds
        SetCommTimeouts(hComm, &timeouts);

        hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hSerialReadThread = (HANDLE)_beginthreadex(NULL, 0, &SerialReadThreadFunc, NULL, 0, &serialReadThreadId);

        retVal = 0;
    }

    return retVal;
}

#ifndef SERIAL_USE_OVERLAPPED
uint16_t SerialWrite(uint8_t *dataV, uint16_t len)
{
    uint16_t retVal;
    BOOL status;
    DWORD dNoOFBytestoWrite;     // No of bytes to write into the port
    DWORD dNoOfBytesWritten = 0; // No of bytes written to the port

    retVal = 0;

    dNoOFBytestoWrite = len;

    status = WriteFile(hComm,              // Handle to the Serial port
                       dataV,              // Data to be written to the port
                       dNoOFBytestoWrite,  //No of bytes to write
                       &dNoOfBytesWritten, //Bytes written
                       NULL);

    if (status == true)
    {
#ifdef DEBUG_SHOW
        printf("Worte %d bytes\n", dNoOfBytesWritten);
#endif
    }
    else
    {
        DWORD err = GetLastError();
#ifdef DEBUG_SHOW
        printf(":( :( >:( FAIL %d", err);
#endif
    }

    return retVal;
}

static unsigned __stdcall SerialReadThreadFunc(void *pArguments)
{
    char TempChar;          //Temporary character used for reading
    char SerialBuffer[256]; //Buffer for storing Rxed Data
    DWORD NoBytesRead;
    BOOL status;
    DWORD dwEventMask;
    DWORD dwWaitRes;
    int i;

    status = SetCommMask(hComm, EV_RXCHAR);

    printf("Waiting for read....\n");
    while (1)
    {
        //status = WaitCommEvent(hComm, &dwEventMask, NULL);
        i = 0;
        do
        {
            ReadFile(hComm,            //Handle of the Serial port
                     &TempChar,        //Temporary character
                     sizeof(TempChar), //Size of TempChar
                     &NoBytesRead,     //Number of bytes read
                     NULL);

            if (NoBytesRead != 0)
            {
                SerialBuffer[i] = TempChar; // Store Tempchar into buffer
                i++;
            }
        } while (NoBytesRead > 0);
        if (i != 0)
        {
#ifdef DEBUG_SHOW
            printf("Read %d chars\n", i);
#endif
            if (serialSetup.serialReadCb != NULL)
            {
                serialSetup.serialReadCb((uint8_t *)SerialBuffer, i);
            }
        }
            dwWaitRes = WaitForSingleObject(hExitEvent, 10);
            if (dwWaitRes == WAIT_OBJECT_0)
            {
                break;
            }
    }
    printf("Read Thread aborted...\n");
    _endthreadex(0);
    return 0;
}

#else
uint16_t SerialWrite(uint8_t *dataV, uint16_t len)
{
    uint16_t retVal;
    BOOL status;
    DWORD dNoOFBytestoWrite;     // No of bytes to write into the port
    DWORD dNoOfBytesWritten = 0; // No of bytes written to the port
    OVERLAPPED oc;
    BOOL ocStatus;

    retVal = 0;

    dNoOFBytestoWrite = len;
    oc.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    status = WriteFile(hComm,             // Handle to the Serial port
                       dataV,             // Data to be written to the port
                       dNoOFBytestoWrite, //No of bytes to write
                       NULL,              //Bytes written
                       &oc);
    ocStatus = GetOverlappedResult(hComm, &oc, &dNoOfBytesWritten, TRUE);
    if (ocStatus == true)
    {
#ifdef DEBUG_SHOW
        printf("Worte %d bytes\n", dNoOfBytesWritten);
#endif
    }
    else
    {
        DWORD err = GetLastError();
#ifdef DEBUG_SHOW
        printf(":( :( >:( FAIL %d", err);
#endif
    }

    return retVal;
}

static unsigned __stdcall SerialReadThreadFunc(void *pArguments)
{
    char TempChar;          //Temporary character used for reading
    char SerialBuffer[256]; //Buffer for storing Rxed Data
    DWORD NoBytesRead;
    OVERLAPPED osStatus;
    BOOL status;
    DWORD dwEventMask;
    DWORD dwWaitRes;
    OVERLAPPED ocRead;
    BOOL ocStatus;

    status = SetCommMask(hComm, EV_RXCHAR);
    ocRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    osStatus.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    printf("Waiting for read....\n");
    while (1)
    {
        status = WaitCommEvent(hComm, &dwEventMask, &osStatus);
        dwWaitRes = WaitForSingleObject(osStatus.hEvent, 100);
        // printf("ComMask: %08X WaitRes: to:%d o0:%d wa:%d\n", dwEventMask, dwWaitRes == WAIT_TIMEOUT,
        //        dwWaitRes == WAIT_OBJECT_0,
        //        dwWaitRes == WAIT_ABANDONED);

        if (dwWaitRes == WAIT_OBJECT_0)
        {
            int i = 0;
            do
            {
                ReadFile(hComm,            //Handle of the Serial port
                         &TempChar,        //Temporary character
                         sizeof(TempChar), //Size of TempChar
                         NULL,             //Number of bytes read
                         &ocRead);

                ocStatus = GetOverlappedResult(hComm, &ocRead, &NoBytesRead, TRUE);
                //if (NoBytesRead != 0)
                {
                    SerialBuffer[i] = TempChar; // Store Tempchar into buffer
                    i++;
                }
            } while (NoBytesRead > 0);
            if (i != 0)
            {
#ifdef DEBUG_SHOW
                printf("Read %d chars\n", i);
#endif
                if (serialSetup.serialReadCb != NULL)
                {
                    serialSetup.serialReadCb((uint8_t *)SerialBuffer, i);
                }
            }
        }
        else
        {
            dwWaitRes = WaitForSingleObject(hExitEvent, 10);
            if (dwWaitRes == WAIT_OBJECT_0)
            {
                break;
            }
        }
    }

    printf("Read Thread aborted...\n");
    _endthreadex(0);

    return 0;
}
#endif