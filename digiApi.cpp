#include "Hardware.h"

#include "digiApi.h"
#include "digiApiCmd.h"

#include <string.h>

#define ENC_FRAME_BUFF_SIZE 256

static uint8_t frmId = 0;
static uint8_t rawFrmBuff[ENC_FRAME_BUFF_SIZE];
static uint8_t encFrmBuff[ENC_FRAME_BUFF_SIZE];

uint16_t ApiGetParam(uint8_t* atStr, uint16_t lenAtStr, uint8_t* paramV)
{
   uint16_t ret;
   uint16_t i;

   frmId ++;
   rawFrmBuff[0] = 0x09;
//   rawFrmBuff[1] = frmId;
   rawFrmBuff[1] = 0x01;
   for (i=0; i<lenAtStr; i++)
   {
      rawFrmBuff[i+2] = atStr[i];
   }

   ret = ApiEncodeFrame(rawFrmBuff,i+2,encFrmBuff,ENC_FRAME_BUFF_SIZE,1);

   for (i=0; i<ret; i++)
   {
      paramV[i] = encFrmBuff[i];
   }
   return ret;
}

uint16_t ApiRawFrame(uint8_t* rawStr, uint16_t lenRawStr, uint8_t* paramV)
{
   uint16_t ret;
   uint16_t i;

   memcpy(rawFrmBuff,rawStr,lenRawStr);
   ret = ApiEncodeFrame(rawFrmBuff,lenRawStr,encFrmBuff,ENC_FRAME_BUFF_SIZE,1);

   for (i=0; i<ret; i++)
   {
      paramV[i] = encFrmBuff[i];
   }
   return ret;
}
