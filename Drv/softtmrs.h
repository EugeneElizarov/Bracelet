/*
 * softtmrs.h
 *
 *  Created on: 11 февр. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_DRIVER_SOFTTMRS_H_
#define VENDOR_SPI_DEMO_DRIVER_SOFTTMRS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SOFT_TIMERS_COUNT			20
#define POLL_CALLBACK_COUNT			20

#define TACT_FREQUENCY				32768

#define COUNT2MS(count)				((count) * 1000 / TACT_FREQUENCY)
#define MS2COUNT(ms)				((ms) * TACT_FREQUENCY / 1000)

typedef void *TimerHandle;
typedef void (* SoftTimers_cb)(TimerHandle timer);
typedef void (* Poll_cb)(void);

void SoftTimers_Init(void);
TimerHandle SoftTimers_Create(uint16_t interval_ms, bool periodic, SoftTimers_cb cb);
TimerHandle SoftTimers_CreateCritical(uint16_t interval_ms, bool periodic, SoftTimers_cb cb);
bool SoftTimers_Restart(TimerHandle handle, uint16_t new_interval_ms);
void SoftwareTimers_Poll(void);
void SoftTimers_Delete(TimerHandle handle);
bool SoftwareTimers_Out(TimerHandle handle);
void SoftwareTimers_Delay(uint32_t timeout);
void SoftwareTimers_DelayPoll(uint32_t timeout, void (*poll)(void));
void SoftwareTimers_DelayUs(uint32_t timeout);
uint32_t SoftwareTimers_GetCount(void);
bool SoftwareTimers_AddPoll(Poll_cb poll);

#endif /* VENDOR_SPI_DEMO_DRIVER_SOFTTMRS_H_ */
