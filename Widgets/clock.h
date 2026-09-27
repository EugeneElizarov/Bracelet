/*
 * clock.h
 *
 *  Created on: 5 мар. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_WIDGETS_CLOCK_H_
#define VENDOR_SPI_DEMO_WIDGETS_CLOCK_H_

#include <stdbool.h>
#include <stdint.h>
#include "../lvgl/lvgl.h"

typedef enum
{
  CS_UNKNOWN,
  CS_GOOD,
  CS_WARNING,
  CS_ALARM,
  CS_COUNT
}ClockState;

typedef enum
{
	IS_OFF,
	IS_FLASH,
	IS_ON,
	IS_COUNT
}IconState;

void Clock_Init(void);
bool Clock_SetTime(uint8_t minute, uint8_t hour);
bool Clock_SetDate(uint8_t day, uint8_t month, uint16_t year);
void Clock_SetPulse(uint8_t pulse, bool alarm);
void Clock_ShowHeart(IconState state);
void Clock_ShowHand(IconState state);
void Clock_ShowFlash(IconState state);
void Clock_ShowLevel(int8_t percent_level);
void Clock_ShowMinuteHead(IconState state);
void Clock_ShowHourHead(IconState state);
void Clock_ChangeState(ClockState state);
bool Clock_ValidDate(uint8_t day, uint8_t month, uint16_t year);
bool Clock_ValidTime(uint8_t minute, uint8_t hour);
uint8_t Clock_DisplayCount(void);

#endif /* VENDOR_SPI_DEMO_WIDGETS_CLOCK_H_ */
