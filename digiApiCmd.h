#ifndef DIGIAPICMD_H
#define DIGIAPICMD_H

#include "Hardware.h"

uint16_t  ApiEncodeFrame(uint8_t* apiMsgV, uint16_t inLen, uint8_t* apiEncMsgV, uint16_t maxLen, uint8_t escape);
uint16_t  ApiDecodeFrame(uint8_t* encFrmV, uint16_t inLen, uint8_t* apiMsgV, uint16_t maxLen, uint8_t escape);

#endif // DIGIAPICMD_H
