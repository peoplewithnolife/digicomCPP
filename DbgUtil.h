#ifndef DBGUTIL_H
#define DBGUTIL_H
#include "Hardware.h"

void DumpByteStr(uint8_t* rawStrV, uint8_t* hexStrV, uint16_t len);
void DumpAsciiStr(uint8_t* rawStrV, uint8_t* ascStrV, uint16_t len);

#endif // DBGUTIL_H
