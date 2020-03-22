#include "Hardware.h"
#include "digimain.h"
#include "stdio.h"

int main(int argc, char const *argv[])
{
    /* code */
    printf("mingw main caller\n");
    digimain(argc,argv);
    return 0;
}
