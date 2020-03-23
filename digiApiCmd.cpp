#include "Hardware.h"

#include "digiApiCmd.h"
#include "DbgUtil.h"

#include <string.h>
#include <stdio.h>

#define DEBUG_SHOW

#define FRM_BUFF_SIZE 256
#define N_ESCAPED_CHARS 4

static uint8_t tmpFrmBuff[FRM_BUFF_SIZE];

static const uint8_t escapedChars[N_ESCAPED_CHARS] = {0x7E,0x7D,0x11,0x13};

static uint16_t ApiEscapeFrame(uint8_t* frmV, uint16_t frmLen);
static uint16_t ApiDeEscapeFrame(uint8_t* frmV, uint16_t frmLen);

uint16_t  ApiEncodeFrame(uint8_t* apiMsgV, uint16_t inLen, uint8_t* apiEncMsgV, uint16_t maxLen, uint8_t escape)
{
   uint8_t ll,lh;
   uint16_t i;
   uint8_t sum;
   uint16_t ret;

   ll = (uint8_t) (inLen % 256);
   lh = (uint8_t) (inLen / 256);

   sum = 0;
   for (i=0; i<inLen; i++)
   {
      sum += apiMsgV[i];
   }
   sum = 255 - sum;

   apiEncMsgV[0] = 0x7E;
   apiEncMsgV[1] = lh;
   apiEncMsgV[2] = ll;
   for (i=0; i<inLen; i++)
   {
      apiEncMsgV[i+3] = apiMsgV[i];
   }
   apiEncMsgV[i+3] = sum;

   ret = i+4;
   if (escape != 0)
   {
#ifdef DEBUG_SHOW
      uint8_t dbgDork[256];
      DumpByteStr(apiEncMsgV,dbgDork,ret);
      printf("Encode Pre escape\n");
      printf((char*)dbgDork);
#endif
      ret = ApiEscapeFrame(apiEncMsgV, ret);
#ifdef DEBUG_SHOW
      DumpByteStr(apiEncMsgV,dbgDork,ret);
      printf("Encode Post escape\n");
      printf((char*)dbgDork);
#endif
   }

   return ret;
}

uint16_t  ApiDecodeFrame(uint8_t* encFrmV, uint16_t inLen, uint8_t* apiMsgV, uint16_t maxLen, uint8_t escape)
{
   uint16_t frmLen;
   uint16_t frmLenEsc;
   uint16_t sum;
   uint16_t i;

   if (encFrmV[0] != 0x7E)
   {
      return 0;
   }

   if (escape != 0)
   {
#ifdef DEBUG_SHOW
      uint8_t dbgDork[256];
      DumpByteStr(encFrmV,dbgDork,inLen);
      printf("Decode Pre escape\n");
      printf((char*)dbgDork);
#endif
      frmLenEsc = ApiDeEscapeFrame(encFrmV,inLen);
#ifdef DEBUG_SHOW
      DumpByteStr(encFrmV,dbgDork,frmLenEsc);
      printf("Decode Post escape\n");
      printf((char*)dbgDork);
#endif
   }

   frmLen = encFrmV[1] * 256;
   frmLen += encFrmV[2];
   sum = 0;

   for (i=0; i<frmLen; i++)
   {
      sum += encFrmV[i+3];
      apiMsgV[i] = encFrmV[i+3];
   }

   sum+=encFrmV[i+3];

   if (sum%256==0xFF)
   {

   }
   else
   {
      return 0;
   }

   return(frmLen);
}

static uint16_t ApiDeEscapeFrame(uint8_t* frmV, uint16_t frmLen)
{
   uint16_t i;
   uint16_t nEscs;

   nEscs = 0;
   for (i=0; i<frmLen+nEscs; i++)
   {
      if (frmV[i] != 0x7D)
      {
         frmV[i] = frmV[i+nEscs];
      }
      else {
         frmV[i] = frmV[i+nEscs+1] ^ 0x20;
         nEscs ++;
      }
   }
   return frmLen-nEscs;
}

static uint16_t ApiEscapeFrame(uint8_t* frmV, uint16_t frmLen)
{
   uint16_t i;
   uint16_t nEscs;

   nEscs = 0;
   // don't escape the first char
   tmpFrmBuff[0] = frmV[0];

   for (i=1; i<frmLen; i++)
   {
      if (!memchr(escapedChars,frmV[i],N_ESCAPED_CHARS))
      {
         tmpFrmBuff[i+nEscs] = frmV[i];
      }
      else
      {
         tmpFrmBuff[i+nEscs] = 0x7D;
         tmpFrmBuff[i+nEscs+1] = frmV[i] ^ 0x20;
         nEscs ++;
      }
   }

   for (i=0; i<frmLen+nEscs; i++)
   {
      frmV[i] = tmpFrmBuff[i];
   }

   return frmLen+nEscs;
}
