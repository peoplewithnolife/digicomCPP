#include "Hardware.h"
#include "serial.h"
#include "digiApi.h"
#include "digiApiCmd.h"
#include "DbgUtil.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>


unsigned __stdcall DigiThreadFunc( void* pArguments );
static uint8_t nextFrameId;

static void SerialReadCb(uint8_t* readDataV, uint16_t len);
static uint8_t StartThreads(void);
static uint8_t GetNextFrameId(void);
static void FakeInput(void);

int digimain(int argc, char const *argv[])
{
    uint16_t i;
    TS_SERIAL_SETUP serialSetup;

    /* code */
    //printf("Digi Main Get a Job and a haircut\n");
    //printf("%d args\n",argc);
    //for (i=0; i<argc; i++)
    //{
    //    printf("%s\n", argv[i]);
    //}

    serialSetup.comportNum = 22;
    serialSetup.baudRate = CBR_115200;
    serialSetup.serialReadCb = SerialReadCb;
    if (SerialInit(&serialSetup) != 0)
    {
        printf("Error opening serial port.");
        exit(1);
    }

    nextFrameId = 0;
    StartThreads();
    SerialClose();

    return 0;
}

void Testola(void)
{
    uint8_t apiMsg[64];
    uint16_t pLen;
 
    pLen = ApiGetParam((uint8_t*) "VL", 2, GetNextFrameId(), apiMsg);
    SerialWrite(apiMsg, pLen);
}

unsigned __stdcall DigiThreadFunc( void* pArguments )
{
    printf( "Starting Process...\n" );
    Testola();
    printf("Termaiting Process...\n");
    _endthreadex( 0 );
    return 0;
}

static uint8_t StartThreads(void)
{
    HANDLE hThread;
    unsigned threadID;

    printf( "Creating serial thread...\n" );

    hThread = (HANDLE)_beginthreadex( NULL, 0, &DigiThreadFunc, NULL, 0, &threadID );

    WaitForSingleObject( hThread, INFINITE );
    printf( "Exiting\n");
    // Destroy the thread object.
    CloseHandle( hThread );

    return 0;
}

static void SerialReadCb(uint8_t* readDataV, uint16_t len)
{
    uint8_t apiMsg[128];
    uint8_t dbgStr[1024];
    uint16_t pLen;

    pLen = ApiDecodeFrame(readDataV,len,apiMsg,128,1);
    
    DumpAsciiStr(apiMsg,dbgStr,pLen);
    printf((char*)dbgStr);
}

static uint8_t GetNextFrameId(void)
{
    nextFrameId += 1;
    return nextFrameId;
}

static void FakeInput(void)
{
    INPUT ip;

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    ip.ki.wVk = 0x41; // 'a'
    ip.ki.dwFlags = 0;
    SendInput(1,&ip,sizeof(INPUT));

    // Release the "A" key
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}
