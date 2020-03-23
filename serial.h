#ifndef SERIAL_H
#define SERIAL_H

#include "hardware.h"
#include <windows.h>

typedef struct _TS_SERIAL_SETUP
{
    uint16_t comportNum;
    DWORD baudRate;   
} TS_SERIAL_SETUP;

uint8_t SerialInit(TS_SERIAL_SETUP* setup);

#endif