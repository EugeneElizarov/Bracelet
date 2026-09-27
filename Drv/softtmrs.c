/*
 * softtmrs.c
 *
 *  Created on: 11 февр. 2026 г.
 *      Author: eugen
 */

#include "softtmrs.h"
#include "tl_common.h"
#include "..\messages.h"
//#include "..\stimer.h"

typedef struct
{
	unsigned long long timestamp;
	union
	{
		uint32_t value;
		struct
		{
		  uint32_t interval : 31;
	  	  uint32_t periodic : 1;
		};
	};
	SoftTimers_cb cb;
}SoftTimerDef;

static Poll_cb poll_cb_list[POLL_CALLBACK_COUNT] = {NULL};

static SoftTimerDef soft_timers[SOFT_TIMERS_COUNT];

void _soft_timers_start_message_send(TimerHandle handle)
{
	Message_Add(MESSAGE_TIMER_START, 0, 0, 0);
	SoftTimers_Delete(handle);
	tlkapi_printf(true, "MTIMER start\n");
}

void SoftTimers_Init(void)
{
	int i;
	memset(soft_timers, 0, sizeof(soft_timers));
	memset(poll_cb_list, 0, sizeof(poll_cb_list));

	clock_32k_init(CLK_32K_XTAL);
	mtime_clk_init(CLK_32K_XTAL);
	//mtime_clk_init(CLK_32K_RC);

	mtime_set_value(0);

	SoftTimers_Create(1, false, _soft_timers_start_message_send);

	//mtime_set_interval_ms(1);
	//core_mie_enable(FLD_MIE_MTIE);
}
/*
_attribute_ram_code_sec_ void mtime_irq_handler(void)
{
	uint8_t add = 32;
	next_single_value += 768;
	if (next_single_value >= 1000)
	{
		next_single_value -= 1000;
		add++;
	}

	mtime_set_cmp_value(mtime_get_cmp_value() + add);

	for (add = 0; add < SOFT_TIMERS_COUNT; add++)
	{
		if (soft_timers[add].interval)
		{
			soft_timers[add].interval--;
			if (soft_timers[add].interval == 0)
			{
				if (soft_timers[add].critical)
				{
					soft_timers[add].cb(&soft_timers[add]);
				}
				else
				{
					soft_timers[add].overflows++;
					if (soft_timers[add].overflows == 0)
						soft_timers[add].overflows--;
				}
				soft_timers[add].interval = soft_timers[add].period;
			}
		}
	}
}
*/
static TimerHandle _timer_create(uint16_t interval_ms, bool periodic, SoftTimers_cb cb)
{
  if ((interval_ms > 0) && (!periodic || (cb != NULL)))
  {
	  int i;
	  uint32_t interval = TACT_FREQUENCY;
	  interval *= interval_ms;
	  interval /= 1000;
	  for (i = 0; i < SOFT_TIMERS_COUNT; i++)
		  if (soft_timers[i].interval == 0)
		  {
			  soft_timers[i].cb = cb;
			  soft_timers[i].interval = interval;
			  soft_timers[i].periodic = periodic ? 1 : 0;
			  soft_timers[i].timestamp = mtime_get_value();
			  return &soft_timers[i];
		  }
  }
  return NULL;
}
TimerHandle SoftTimers_Create(uint16_t interval_ms, bool periodic, SoftTimers_cb cb)
{
	return _timer_create(interval_ms, periodic, cb);
}

void SoftTimers_Delete(TimerHandle handle)
{
  if (handle)
  {
	((SoftTimerDef *)handle)->value = 0;
	((SoftTimerDef *)handle)->timestamp = 0;
  }
}

static uint8_t timers_poll_index = 0;
static uint8_t poll_callback_index = 0;
static bool timers_check = false;

void SoftwareTimers_Poll(void)
{
	/*
	if ((timers_poll_index >= SOFT_TIMERS_COUNT) &&
		(poll_callback_index >= POLL_CALLBACK_COUNT))
	{
		timers_poll_index = 0;
		poll_callback_index = 0;
		timers_check = false;
	}
*/
	if (timers_check)
	{
		if (timers_poll_index < SOFT_TIMERS_COUNT)
		{
			if (soft_timers[timers_poll_index].interval)
			{
				unsigned long long timestamp = mtime_get_value();
				if ((timestamp - soft_timers[timers_poll_index].timestamp) >= soft_timers[timers_poll_index].interval)
				{
					if (soft_timers[timers_poll_index].cb)
						soft_timers[timers_poll_index].cb(&soft_timers[timers_poll_index]);
					else
						Message_AddPtr(MESSAGE_TIMER_OUT, 0, 0, (TimerHandle)&soft_timers[timers_poll_index]);
					if (soft_timers[timers_poll_index].periodic)
						soft_timers[timers_poll_index].timestamp += soft_timers[timers_poll_index].interval;
					else
						soft_timers[timers_poll_index].interval = 0;
				}
			}
			timers_poll_index++;
		}
		else
		{
			timers_poll_index = 0;
		}
	}
	else
	{
		if (poll_callback_index < POLL_CALLBACK_COUNT)
		{
			if (poll_cb_list[poll_callback_index])
				poll_cb_list[poll_callback_index]();
			poll_callback_index++;
		}
		else
		{
			poll_callback_index = 0;
		}
	}

	timers_check = !timers_check;
}

bool SoftwareTimers_Out(TimerHandle handle)
{
	bool result = false;
	if (handle)
	{
		if (((SoftTimerDef *)handle)->interval == 0)
		{
			result = ((SoftTimerDef *)handle)->timestamp != 0;
			if (result)
				((SoftTimerDef *)handle)->timestamp = 0;
		}
	}
	return result;
}

void SoftwareTimers_Delay(uint32_t timeout)
{
	unsigned long long timestamp = mtime_get_value();
	unsigned long long interval = timeout;

	interval *= TACT_FREQUENCY;
	interval /= 1000;

	while ((mtime_get_value() - timestamp) < interval)
		SoftwareTimers_Poll();
}

void SoftwareTimers_DelayPoll(uint32_t timeout, void (*poll)(void))
{
	unsigned long long timestamp = mtime_get_value();
	unsigned long long interval = timeout;

	interval *= TACT_FREQUENCY;
	interval /= 1000;

	while ((mtime_get_value() - timestamp) < interval)
	{
		SoftwareTimers_Poll();
		if (poll)
			poll();
	}
}

void SoftwareTimers_DelayUs(uint32_t timeout)
{
	unsigned long long timestamp = mtime_get_value();
	unsigned long long interval = timeout;
	bool poll;

	interval *= TACT_FREQUENCY;
	interval /= 1000000;
	interval++;

	poll = interval > 30;

	while ((mtime_get_value() - timestamp) < interval)
	{
		if (poll)
			SoftwareTimers_Poll();
	}
}

uint32_t SoftwareTimers_GetCount(void)
{
	unsigned long long timestamp = mtime_get_value();
	return timestamp & 0xFFFFFFFF;
}

bool SoftTimers_Restart(TimerHandle handle, uint16_t new_interval_ms)
{
	if ((((SoftTimerDef *)handle)->interval != 0) ||
		 ((SoftTimerDef *)handle)->timestamp != 0)
	{
		uint32_t interval = TACT_FREQUENCY;
		interval *= new_interval_ms;
		interval /= 1000;
		((SoftTimerDef *)handle)->interval = interval;
		((SoftTimerDef *)handle)->timestamp = mtime_get_value();
		return true;
	}
	return false;
}

bool SoftwareTimers_AddPoll(Poll_cb poll)
{
	for(int i = 0; i < ARRAY_SIZE(poll_cb_list); i++)
	{
		if (poll_cb_list[i] == NULL)
		{
			poll_cb_list[i] = poll;
			return true;
		}
	}
	return false;
}
