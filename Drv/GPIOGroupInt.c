/*
 * GPIOGroupInt.c
 *
 *  Created on: 16 мар. 2026 г.
 *      Author: eugen
 */

#include <stdint.h>
#include "GPIOGroupInt.h"
#include "../messages.h"

typedef struct
{
	GPIOGroupInt_cb cb;
	uint16_t gpio;
	struct
	{
		uint8_t rising : 1;
		uint8_t active : 1;
		uint8_t enable : 1;
		uint8_t reset : 1;
		uint8_t event : 1;
		uint8_t reserved : 3;
	};
	uint8_t timeout;
	unsigned long long timestamp;
}GPIO_Group_desc;

static GPIO_Group_desc gpio_group[8];

static uint8_t _is_active(GPIO_Group_desc *desc)
{
	return gpio_get_level(desc->gpio) ? desc->rising : 1 - desc->rising;
}

int8_t GPIO_GroupRegister(gpio_func_pin_e pin, gpio_irq_trigger_type_e type, uint8_t timeout, GPIOGroupInt_cb cb)
{
	int8_t group = GPIO_GroupFromPin(pin);
	if (group >= 0)
	{
		if ((gpio_group[group].gpio == GPIO_NONE_PIN) && (cb != NULL))
		{
			gpio_group[group].cb = cb;
			gpio_group[group].gpio = pin;
			gpio_group[group].rising = ((type == INTR_HIGH_LEVEL) || (type == INTR_RISING_EDGE)) ? 1 : 0;
			gpio_group[group].timeout = timeout;
			gpio_group[group].timestamp = 0;
			gpio_group[group].enable = 0;

			GPIO_Config(pin, true, (gpio_group[group].rising != 0) ? GPIO_PIN_PULLDOWN_100K : GPIO_PIN_PULLUP_10K);

	    	GPIO_GroupEnable(group);

	    	return true;
		}
	}
	else
		group = -1;
	return group;
}

void GPIO_GroupEnable(uint8_t group)
{
	if (group < ARRAY_COUNT(gpio_group))
	{
		if (gpio_group[group].enable == 0)
		{
			gpio_group[group].active = _is_active(&gpio_group[group]);
			gpio_group[group].reset = 0;
			gpio_group[group].event = 0;
			gpio_group[group].timestamp = 0;
			gpio_group[group].enable = 1;
		}
	}
}

void GPIO_GroupDisable(uint8_t group)
{
	if (group < ARRAY_COUNT(gpio_group))
		gpio_group[group].enable = 0;
}

void GPIO_GroupBZZZSetTimeout(uint8_t group, uint8_t timeout)
{
	if (group < ARRAY_COUNT(gpio_group))
		gpio_group[group].timeout = timeout;
}

static uint8_t current_group = 0;
static GPIO_Group_desc *current_group_desc = gpio_group;

void GPIO_GroupPoll(void)
{
	if ((current_group_desc->enable != 0) &&
		(current_group_desc->gpio != GPIO_NONE_PIN))
	{
		uint8_t state = _is_active(current_group_desc);
		if (state != current_group_desc->active)
		{
			uint16_t interval;
			if (current_group_desc->timestamp)
				interval = 1 + mtime_get_value() - current_group_desc->timestamp;
			else
				interval = 0;

			if ((state == 0) || (current_group_desc->timeout == 0) || (interval > current_group_desc->timeout))
				current_group_desc->reset = 1;
			if (current_group_desc->reset)
			{
				current_group_desc->active = state;
				current_group_desc->timestamp = 0;
				current_group_desc->reset = 0;
				if (state)
					current_group_desc->event = 1;
			}
		}
		if (current_group_desc->event == 1)
		{
			if (current_group_desc->cb)
				current_group_desc->cb(current_group);
			else
				Message_Add(MESSAGE_GPIO_EVENT, current_group, 0, 0);
			current_group_desc->event = 0;
		}

	}
	current_group_desc++;
	current_group++;
	if (current_group >= ARRAY_COUNT(gpio_group))
	{
		current_group_desc = gpio_group;
		current_group = 0;
	}
}

bool GPIO_GroupIsEvent(uint8_t group)
{
	bool result = false;
	if (group < ARRAY_COUNT(gpio_group))
	{
		if (gpio_group[group].enable != 0)
		{
			result = gpio_group[group].event != 0;
			gpio_group[group].event = 0;
		}
	}
	return result;
}

int8_t GPIO_GroupFromPin(gpio_func_pin_e pin)
{
	uint8_t gg = (pin >> 8) & 0xFF;
	uint8_t group_bit = pin & 0xFF;
	int8_t group = (pin == 0x01) ? 0 : (pin == 0x02) ? 1 : (pin == 0x04) ? 2 :
		           (pin == 0x08) ? 3 : (pin == 0x10) ? 4 : (pin == 0x20) ? 5 :
			       (pin == 0x40) ? 6 : (pin == 0x80) ? 7 : -1;
	if (gg >= 7)
		group = -1;

	return group;
}
