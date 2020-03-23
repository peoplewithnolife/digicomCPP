#include "DbgUtil.h"
#include <stdio.h>
#include <string.h>

#define NEW_LINE_POS 32
#define EXTRA_SPACE_POS 8

void DumpByteStr(uint8_t* rawStrV, uint8_t* hexStrV, uint16_t len)
{
   uint16_t rawIdx;

   rawIdx = 0;

   sprintf((char*)hexStrV,"");
   while (rawIdx < len)
   {
      sprintf((char*) &hexStrV[strlen((char*)hexStrV)], "%02X ", rawStrV[rawIdx]);
      rawIdx++;
      if ((rawIdx % NEW_LINE_POS) == 0)
      {
         sprintf((char*) &hexStrV[strlen((char*)hexStrV)], "\x0D\x0A");
      }
      else if ((rawIdx % EXTRA_SPACE_POS) == 0)
      {
         sprintf((char*) &hexStrV[strlen((char*)hexStrV)], "  ");
      }
   }
   sprintf((char*) &hexStrV[strlen((char*)hexStrV)], "\x0D\x0A");
}

void DumpAsciiStr(uint8_t* rawStrV, uint8_t* ascStrV, uint16_t len)
{
   uint16_t rawIdx;
   int8_t charToWrite;
   rawIdx = 0;

   sprintf((char*)ascStrV,"");
   while (rawIdx < len)
   {
      charToWrite = '-';
      if (rawStrV[rawIdx] > 0x1F && rawStrV[rawIdx] < 0x7F)
      {
         charToWrite = (char) rawStrV[rawIdx];
      }
      sprintf((char*) &ascStrV[strlen((char*)ascStrV)], "%c", charToWrite);
      rawIdx++;
   }
   sprintf((char*) &ascStrV[strlen((char*)ascStrV)], "\x0D\x0A");
}

