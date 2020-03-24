#include "Hardware.h"
#include "serial.h"
#include "digiApi.h"
#include "digiApiCmd.h"
#include "DbgUtil.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>

#define MAX_API_MSG_LEN 1024

typedef enum _TE_MESSAGE_EVENT_HANDLES
{
    mevt_AT_RSP,
    mevt_TOTAL_MESSAGE_EVENTS
} TE_MESSAGE_EVENT_HANDLES;

typedef struct _TS_API_MESSAGE_IN
{
    uint16_t len;
    uint8_t apiMsgIn[MAX_API_MSG_LEN];
} TS_API_MESSAGE_IN;

unsigned __stdcall DigiThreadFunc(void *pArguments);
static uint8_t nextFrameId;
static uint8_t ApiAtGetParam(uint8_t *atCmd);
static void SerialReadCb(uint8_t *readDataV, uint16_t len);
static uint8_t StartThreads(void);
static uint8_t GetNextFrameId(void);
static void FakeInput(void);

static HANDLE msgEvents[mevt_TOTAL_MESSAGE_EVENTS];
TS_API_MESSAGE_IN atMsgIn;

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
    for (i = 0; i < mevt_TOTAL_MESSAGE_EVENTS; i++)
    {
        msgEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

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

static uint8_t ApiAtGetParam(uint8_t *atCmdIn)
{
    uint8_t apiMsgOut[64];
    uint8_t atCmdOut[3];
    uint16_t pLen;
    uint8_t retVal;

    retVal = 0;
    pLen = ApiGetParam((uint8_t *)atCmdIn, 2, GetNextFrameId(), apiMsgOut);
    SerialWrite(apiMsgOut, pLen);
    if (WaitForSingleObject(msgEvents[mevt_AT_RSP], 100) == WAIT_TIMEOUT)
    {
        printf("Message Failure!!!! :( :( :(\n");
    }
    else
    {
        strncpy_s((char *)atCmdOut, 3, (char *)&atMsgIn.apiMsgIn[2], _TRUNCATE);
        uint8_t dbgStr[1024];

        if (strncmp((char *)atCmdOut, "VL", 2) == 0)
        {
            DumpAsciiStr(atMsgIn.apiMsgIn, dbgStr, atMsgIn.len);
        }
        else if (strncmp((char *)atCmdOut, "HV", 2) == 0)
        {
            DumpAsciiStr(atMsgIn.apiMsgIn, dbgStr, atMsgIn.len);
        }
        else
        {
            printf("Message %s: ", atCmdOut);
            DumpByteStr(&atMsgIn.apiMsgIn[5], dbgStr, atMsgIn.len - 5);
        }

        printf((char *)dbgStr);
        retVal = 1;
    }
    return retVal;
}

static void SetAirplaneMode(uint8_t on);
static void SetAirplaneMode(uint8_t on)
{
    uint8_t apiMsgOut[64];
    uint8_t atStr[16];
    uint8_t atCmdOut[3];
    uint16_t pLen;

    sprintf((char *)atStr, "AM");
    atStr[2] = on;
    pLen = ApiGetParam(atStr, 3, GetNextFrameId(), apiMsgOut);
    SerialWrite(apiMsgOut, pLen);
    if (WaitForSingleObject(msgEvents[mevt_AT_RSP], 5000) == WAIT_TIMEOUT)
    {
        printf("Message Failure!!!! :( :( :(\n");
    }
    else
    {
        uint8_t dbgStr[1024];
        strncpy_s((char *)atCmdOut, 3, (char *)&atMsgIn.apiMsgIn[2], _TRUNCATE);
        printf("Message %s: \n", atCmdOut);
        DumpByteStr(&atMsgIn.apiMsgIn[0], dbgStr, atMsgIn.len);
    }
}

unsigned __stdcall DigiThreadFunc(void *pArguments)
{
    printf("Starting Process...\n");

    ApiAtGetParam((uint8_t *)"VL");
    ApiAtGetParam((uint8_t *)"HV");
    ApiAtGetParam((uint8_t *)"AM");
    ApiAtGetParam((uint8_t *)"DO");
    ApiAtGetParam((uint8_t *)"AI");

    printf("Press q to exit");
    char myChar = ' ';
    while (myChar != 'q')
    {
        myChar = (char)getchar();
        if (myChar == 'a')
        {
            SetAirplaneMode(1);
        }
        else if (myChar == 'n')
        {
            SetAirplaneMode(0);
        }
    }

    printf("Termaiting Process...\n");
    _endthreadex(0);
    return 0;
}

static uint8_t StartThreads(void)
{
    HANDLE hThread;
    unsigned threadID;

    printf("Creating serial thread...\n");

    hThread = (HANDLE)_beginthreadex(NULL, 0, &DigiThreadFunc, NULL, 0, &threadID);

    WaitForSingleObject(hThread, INFINITE);
    printf("Exiting\n");
    // Destroy the thread object.
    CloseHandle(hThread);

    return 0;
}

static void SerialReadCb(uint8_t *readDataV, uint16_t len)
{
    uint8_t apiMsg[128];
    uint16_t pLen;

    pLen = ApiDecodeFrame(readDataV, len, apiMsg, 128, 1);
    if (pLen > 0)
    {
        if (apiMsg[0] == 0x88)
        {
            if (pLen < MAX_API_MSG_LEN)
            {
                atMsgIn.len = pLen;
                memcpy(atMsgIn.apiMsgIn, apiMsg, pLen);
                SetEvent(msgEvents[mevt_AT_RSP]);
            }
        }
        else
        {
            uint8_t dbgStr[1024];
            printf("Unhandled Message\n");
            atMsgIn.len = pLen;
            memcpy(atMsgIn.apiMsgIn, apiMsg, pLen);
            DumpByteStr(&atMsgIn.apiMsgIn[0], dbgStr, atMsgIn.len);
            printf((char *)dbgStr);
        }
    }
}

static uint8_t GetNextFrameId(void)
{
    nextFrameId += 1;
    if (nextFrameId == 0)
    {
        nextFrameId += 1;
    }
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
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "A" key
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}
