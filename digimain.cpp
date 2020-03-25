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
    mevt_MODEM_STATUS,
    mevt_TOTAL_MESSAGE_EVENTS
} TE_MESSAGE_EVENT_HANDLES;

typedef struct _TS_API_MESSAGE_IN
{
    uint16_t len;
    uint8_t apiMsgIn[MAX_API_MSG_LEN];
} TS_API_MESSAGE_IN;

static unsigned __stdcall DigiThreadFunc(void *pArguments);
static unsigned __stdcall InputFunc(void *pArguments);

static uint8_t nextFrameId;
static uint8_t ApiAtGetParam(uint8_t *atCmd);
static void SerialReadCb(uint8_t *readDataV, uint16_t len);
static uint8_t StartThreads(void);
static uint8_t GetNextFrameId(void);
static void FakeInput(void);
static void PrintErrors(void);

static uint8_t UpdateDigiCycler(uint8_t init, DWORD updateMs);

static HANDLE msgEvents[mevt_TOTAL_MESSAGE_EVENTS];
TS_API_MESSAGE_IN atMsgIn;

static uint32_t errMsg, errSleep, errWake, totalTrials;
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
        errMsg ++;
    }
    else
    {
        uint8_t dbgStr[1024];
        strncpy_s((char *)atCmdOut, 3, (char *)&atMsgIn.apiMsgIn[2], _TRUNCATE);
        printf("Message %s: \n", atCmdOut);
        DumpByteStr(&atMsgIn.apiMsgIn[0], dbgStr, atMsgIn.len);
    }
}

typedef enum _TE_DIGI_CYCLE_STATE
{
    dcs_INIT,
    dcs_AWAKE,
    dcs_WAIT_PSM,
    dcs_SLEEP,
    dcs_WAIT_WAKE,
    dcs_WAIT_CELL_CONNECT,
    dcs_TOTAL_STATES
} TE_DIGI_CYCLE_STATE;

static uint8_t UpdateDigiCycler(uint8_t init, DWORD updateMs)
{
    static TE_DIGI_CYCLE_STATE dcState = dcs_INIT;
    static DWORD elapsedTime;

    if ((init != 0) || (dcState == dcs_INIT))
    {
        dcState = dcs_INIT;
    }

    switch (dcState)
    {
    case dcs_INIT:
        /* code */
        //SetAirplaneMode(0);
        elapsedTime = 0;
        dcState = dcs_AWAKE;
        totalTrials ++;
        break;
    case dcs_AWAKE:
        /* code */
        if (elapsedTime > 3000)
        {
            SetAirplaneMode(1);
            printf("Waiting Modem Status Disconnect...\n");
            elapsedTime = 0;
            dcState = dcs_WAIT_PSM;
        }
        break;
    case dcs_WAIT_PSM:
        if (WaitForSingleObject(msgEvents[mevt_MODEM_STATUS], 10) == WAIT_OBJECT_0)
        {
            printf("Got Modem Status:%d Time:%d\n", atMsgIn.apiMsgIn[1],elapsedTime);
            dcState = dcs_SLEEP;
            elapsedTime = 0;
        }
        else
        {
            if (elapsedTime > 30000)
            {
                printf("Never got disconnect...");
                errSleep ++;
#warning Recover from this
                dcState = dcs_WAIT_WAKE;
                elapsedTime = 0;
            }
        }
        /* code */
        break;
    case dcs_SLEEP:
        if (elapsedTime > 10000)
        {
            printf("Waking ...");
            dcState = dcs_WAIT_WAKE;
            elapsedTime = 0;
        }
        /* code */
        break;
    case dcs_WAIT_WAKE:
        /* code */
        if (elapsedTime > 10000)
        {
            printf("Waiting For cell connect ...\n");
            SetAirplaneMode(0);
            dcState = dcs_WAIT_CELL_CONNECT;
            elapsedTime = 0;
        }
        break;
    case dcs_WAIT_CELL_CONNECT:
        /* code */
        if (WaitForSingleObject(msgEvents[mevt_MODEM_STATUS], 10) == WAIT_OBJECT_0)
        {
            printf("Got Modem Status:%d Time:%d\n", atMsgIn.apiMsgIn[1],elapsedTime);
            PrintErrors();
            dcState = dcs_INIT;
            elapsedTime = 0;
        }
        else
        {
            if (elapsedTime > 30000)
            {
                printf("Never got connect...");
                errWake ++;
#warning Recover from this
                dcState = dcs_INIT;
                elapsedTime = 0;
            }
        }
        break;
    default:
        printf("Unknown error !!!!");
        dcState = dcs_INIT;
        break;
    }
    elapsedTime += updateMs;
    return 0;
}

static unsigned __stdcall DigiThreadFunc(void *pArguments)
{
#define UPDATE_MS 1000
    HANDLE hInputThread;
    unsigned hInputThreadId;
    uint8_t quit = 0;

    printf("Starting Process...\n");
    hInputThread = (HANDLE)_beginthreadex(NULL, 0, &InputFunc, NULL, 0, &hInputThreadId);

    ApiAtGetParam((uint8_t *)"VL");
    ApiAtGetParam((uint8_t *)"HV");
    ApiAtGetParam((uint8_t *)"AM");
    ApiAtGetParam((uint8_t *)"DO");
    ApiAtGetParam((uint8_t *)"AI");

    errMsg = 0;
    errSleep = 0;
    errWake = 0;
    UpdateDigiCycler(1, UPDATE_MS);

    while (quit == 0)
    {
        UpdateDigiCycler(0, UPDATE_MS);
        if (WaitForSingleObject(hInputThread, UPDATE_MS) == WAIT_OBJECT_0)
        {
            quit = 1;
        }
    }

    printf("Termaiting Process...\n");
    _endthreadex(0);
    return 0;
}

static unsigned __stdcall InputFunc(void *pArguments)
{
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
    _endthreadex(0);
    return 0;
}

static uint8_t StartThreads(void)
{
    HANDLE hDigiThread;
    unsigned digiThreadID;

    printf("Creating serial thread...\n");

    hDigiThread = (HANDLE)_beginthreadex(NULL, 0, &DigiThreadFunc, NULL, 0, &digiThreadID);

    WaitForSingleObject(hDigiThread, INFINITE);
    printf("Exiting\n");
    // Destroy the thread object.
    CloseHandle(hDigiThread);

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
            if (apiMsg[0] == 0x8A)
            {
                SetEvent(msgEvents[mevt_MODEM_STATUS]);
            }
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

static void PrintErrors(void)
{
    printf("Trials:%d err: Msg:%d Sleep:%d Wake:%d\n",totalTrials, errMsg,errSleep,errWake);
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
