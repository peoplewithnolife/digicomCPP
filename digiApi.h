#ifndef DIGIAPI_H
#define DIGIAPI_H

#include "Hardware.h"

uint16_t ApiGetParam(uint8_t* atStr, uint16_t lenAtStr, uint8_t* paramV);

uint16_t ApiRawFrame(uint8_t* rawStr, uint16_t lenRawStr, uint8_t* paramV);
#endif // DIGIAPI_H
