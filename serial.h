#ifndef SERIAL_H
#define SERIAL_H

#include "hardware.h"
#include <windows.h>

typedef void (*TD_SERIAL_READ_CB)(uint8_t* readDataV, uint16_t len);

typedef struct _TS_SERIAL_SETUP
{
    uint16_t comportNum;
    DWORD baudRate;
    TD_SERIAL_READ_CB serialReadCb;
} TS_SERIAL_SETUP;

uint8_t SerialInit(TS_SERIAL_SETUP* setup);
uint16_t SerialWrite(uint8_t* dataV, uint16_t len);
void SerialClose(void);

#endif