/*
 * messages.h
 *
 *  Created on: 22 мар. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_MESSAGES_H_
#define VENDOR_SPI_DEMO_MESSAGES_H_

#include <stdbool.h>
#include <stdint.h>
#include "messages_list.h"

#define MESSAGE_COUNT					50
#define PROCESSOR_COUNT					20
#define MESSAGE_PROCESSOR_BUFFER_SIZE	2048

#define COUNT_MSG_ARGS(...)             sizeof((uint8_t[]){__VA_ARGS__})
#define MESSAGES(...)					COUNT_MSG_ARGS(__VA_ARGS__), (uint8_t[]){__VA_ARGS__}

#define P8(p)							((uint8_t)((p) & 0xFF))
#define P16(p8_1, p8_2)					((((uint16_t)P8(p8_2)) << 8) + P8(p8_1))
#define P32(p8_1, p8_2, p8_3, p8_4)		((((uint32_t)P8(p8_4)) << 24) + 			\
		                                 (((uint32_t)P8(p8_3)) << 16) + 			\
		                                 (((uint32_t)P8(p8_2)) << 8)  + P8(p8_1))
#define P16232(p16_1, p16_2)			((((uint32_t)((p16_2)) & 0xFFFF) << 16) + ((p16_1) & 0xFFFF))

#if defined (__clang__)
#pragma anon_unions
#endif

#define FLOAT2UINT(fl)					(((union{float f; uint32_t u;}){.f = (fl)}).u)
#define UINT2FLOAT(ui)					(((union{float f; uint32_t u;}){.u = (ui)}).f)
#define INT2UINT(vi)					(((union{int32_t i; uint32_t u;}){.i = (vi)}).u)
#define UINT2INT(ui)					(((union{inf32_t i; uint32_t u;}){.u = (ui)}).i)


typedef struct
{
	uint8_t ID;
	union
	{
		char cpar8;
		int8_t ipar8;
		uint8_t upar8;
	};
	union
	{
		char cpar16[2];
		int16_t ipar16;
		uint16_t upar16;
	};
	union
	{
		char cpar32[4];
		uint16_t u16par32[2];
		int16_t i16u32[2];
		int32_t ipar32;
		uint32_t upar32;
		void *ppar32;
	};
} Message_t;

typedef Message_t *Message;

typedef bool (* MessageProcessor)(Message message);

void Message_Init(void);
bool Message_Add(const uint8_t ID, uint8_t par1, uint16_t par2, uint32_t par3);
bool Message_AddPtr(const uint8_t ID, uint8_t par1, uint16_t par2, void *par3);
bool Message_AddProcessor(MessageProcessor processor, uint8_t message_count, const uint8_t *message_list);
void Message_Poll(void);

#endif /* VENDOR_SPI_DEMO_MESSAGES_H_ */
