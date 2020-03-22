#include "Hardware.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>

uint8_t StartThreads(void);

int digimain(int argc, char const *argv[])
{
    uint16_t i;
    /* code */
    printf("Digi Main Get a Job and a haircut\n");

    printf("%d args\n",argc);

    for (i=0; i<argc; i++)
    {
        printf("%s\n", argv[i]);
    }

    StartThreads();

    return 0;
}

unsigned __stdcall SerialThreadFunc( void* pArguments )
{
    uint16_t i;
    printf( "In serial thread...\n" );

    for (i=0; i<5; i++)
    {
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
    hThread = (HANDLE)_beginthreadex( NULL, 0, &SerialThreadFunc, NULL, 0, &threadID );

    // Wait until second thread terminates. If you comment out the line
    // below, Counter will not be correct because the thread has not
    // terminated, and Counter most likely has not been incremented to
    // 1000000 yet.
    WaitForSingleObject( hThread, INFINITE );
    printf( "Fred the thread ended\n");
    // Destroy the thread object.
    CloseHandle( hThread );

    return 0;
}
