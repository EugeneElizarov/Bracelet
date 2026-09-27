/*
 * MakeMACAddr.c
 *
 *  Created on: 16 апр. 2026 г.
 *      Author: eugen
 */

#include <stdint.h>
#include "MakeMACAddr.h"

#define SIZE_TABLE_RANDOM_STATIC_ADDR			15
#define B_ADDR_LEN								6

static const uint8_t TSKBM_RandomStaticAddr[SIZE_TABLE_RANDOM_STATIC_ADDR][B_ADDR_LEN]=
{
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD0},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD1},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD2},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD3},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD4},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD5},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD6},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD7},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD8},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xD9},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xDA},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xDB},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xDC},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xDD},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xDE},
 {0xA5,0x5A,0xA5,0x5A,0xA5,0xDF},
};

uint8_t Fletcher4 ( uint8_t *pData, int count )
{
   uint8_t sum1 = 0;
   uint8_t sum2 = 0;
   int index;

   for( index = 0; index < count; index++ )
   {
      sum1 = (sum1 + (pData[index] & 0xFF)) % 4;
      sum2 = (sum2 + sum1) % 4;
      sum1 = (sum1 + ((pData[index] >> 4) & 0xFF)) % 4;
      sum2 = (sum2 + sum1) % 4;
   }

   return (((sum2 << 2) | sum1) & 0xFF);
}

void MakeMAC(void *staticDevMAC, void *staticTSKBMMAC)
{
	int indexTable = Fletcher4 (staticDevMAC, B_ADDR_LEN);
	memcpy(staticTSKBMMAC, TSKBM_RandomStaticAddr [indexTable], B_ADDR_LEN);
}
/*
typedef struct
{
  uint8_t  event;
  uint8_t  status;
} osal_event_hdr_t;

/**
 * GAP_DEVICE_INIT_DONE_EVENT message format.  This message is sent to the
 * app when the Device Initialization is done [initiated by calling
 * GAP_DeviceInit()].
 * /
typedef struct
{
  osal_event_hdr_t  hdr;              //!< GAP_MSG_EVENT and status
  uint8_t opcode;                       //!< GAP_DEVICE_INIT_DONE_EVENT
  uint8_t devAddr[B_ADDR_LEN];          //!< Device's BD_ADDR
  uint16 dataPktLen;                  //!< HC_LE_Data_Packet_Length
  uint8_t numDataPkts;                  //!< HC_Total_Num_LE_Data_Packets
} gapDeviceInitDoneEvent_t;
*/
