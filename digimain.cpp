#include "Hardware.h"
#include "serial.h"
#include "digiApi.h"
#include "digiApiCmd.h"
#include "DbgUtil.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>


static void SerialReadCb(uint8_t* readDataV, uint16_t len);

uint8_t StartThreads(void);
unsigned __stdcall DigiThreadFunc( void* pArguments );

int digimain(int argc, char const *argv[])
{
    uint16_t i;
    TS_SERIAL_SETUP serialSetup;

    /* code */
    printf("Digi Main Get a Job and a haircut\n");

    printf("%d args\n",argc);

    for (i=0; i<argc; i++)
    {
        printf("%s\n", argv[i]);
    }

    serialSetup.comportNum = 22;
    serialSetup.baudRate = CBR_115200;
    serialSetup.serialReadCb = SerialReadCb;
    if (SerialInit(&serialSetup) != 0)
    {
        printf("Error opening serial port.");
        exit(1);
    }

    StartThreads();

    SerialClose();

    return 0;
}

void Testola(void)
{
    uint8_t apiMsg[64];
    uint8_t dbgStr[1024];
    uint16_t pLen;
 
    pLen = ApiGetParam((uint8_t*) "VL", 2, apiMsg);
    DumpByteStr(apiMsg,dbgStr,pLen);
    printf((char*)dbgStr);
    SerialWrite(apiMsg, pLen);
}

void FakeInput(void)
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

unsigned __stdcall DigiThreadFunc( void* pArguments )
{
    uint16_t i;
    printf( "In serial thread...\n" );

    for (i=0; i<2; i++)
    {
        Testola();
        //FakeInput();
        Sleep(2000);
        printf("Thread not dead %d\n",i);
    }

    printf("Thread dead\n");

    _endthreadex( 0 );
    return 0;
}

uint8_t StartThreads(void)
{
    HANDLE hThread;
    unsigned threadID;

    printf( "Creating serial thread...\n" );

    // Create the second thread.
    hThread = (HANDLE)_beginthreadex( NULL, 0, &DigiThreadFunc, NULL, 0, &threadID );

    // Wait until second thread terminates. If you comment out the line
    // below, Counter will not be correct because the thread has not
    // terminated, and Counter most likely has not been incremented to
    // 1000000 yet.


   // char myChar = ' ';
   // while(myChar != 'q') {
   //	cout << myCounter << endl;
   //     myChar = (char) getchar();
   //     printf("Input: %c\n",myChar);
   // }
    WaitForSingleObject( hThread, INFINITE );
    printf( "Fred the thread ended\n");
    // Destroy the thread object.
    CloseHandle( hThread );

    return 0;
}

static void SerialReadCb(uint8_t* readDataV, uint16_t len)
{
    uint8_t apiMsg[128];
    uint8_t dbgStr[1024];
    uint16_t pLen;

    DumpByteStr(readDataV,dbgStr,len); 
    printf((char*)dbgStr);

    pLen = ApiDecodeFrame(readDataV,len,apiMsg,128,1);
    
    printf("Gut response\n");
    DumpByteStr(apiMsg,dbgStr,pLen);
    printf((char*)dbgStr);
    DumpAsciiStr(apiMsg,dbgStr,pLen);
    printf((char*)dbgStr);
}

