/*
 * key.c
 *
 *  Created on: 21 июл. 2026 г.
 *      Author: eugen
 */


#include "key.h"
#include "../def.h"
#include "softtmrs.h"
#include "../messages.h"

static bool initialized = false;

void Key_Init(void)
{
	GPIO_Config(KEY, GPIO_INPUT, GPIO_PIN_PULLUP_10K);
	GPIO_Config(CHARGING, GPIO_INPUT, GPIO_PIN_PULLUP_10K);
	GPIO_Config(CHARGE_END, GPIO_INPUT, GPIO_PIN_PULLUP_10K);
	initialized = true;
}

static uint32_t change_state_timestamp = 0;
static bool key_state = false;
static uint8_t charge_state = 0;
static uint32_t charging_timestamp = 0;
static uint32_t charged_timestamp = 0;

void Key_Poll()
{

	if (!initialized)
		return;

	uint32_t timestamp = SoftwareTimers_GetCount();
	bool state = gpio_get_level(KEY) == 0;
	if (key_state != state)
	{
		change_state_timestamp = timestamp;
		key_state = state;
	}
	else
	{
		if (change_state_timestamp)
		{
			if ((timestamp - change_state_timestamp) >= MS2COUNT(30))
			{
				change_state_timestamp = 0;
				Message_Add(MESSAGE_KEY_CHANGE_STATE, state ? 1 : 0, 0, 0);
			}
		}
	}

	/* Контроль зарядки */
	state = gpio_get_level(CHARGE_END) == 0;
	//if ((timestamp >= MS2COUNT(5000)) && (timestamp < MS2COUNT(10000)))
	//	state = !state;
	if (state != ((charge_state & 1) != 0))
	{
		if (charged_timestamp == 0)
			charged_timestamp = timestamp;
		if ((timestamp - charged_timestamp) >= MS2COUNT(1000))
		{
			if (state)
				charge_state |= 1;
			else
				charge_state &= ~1;
			charge_state |= 0x10;
		}
	}
	else
		charged_timestamp = 0;

	state = gpio_get_level(CHARGING) == 0;
	//if (timestamp < MS2COUNT(5000))
	//	state = !state;
	if (state != ((charge_state & 2) != 0))
	{
		if (charging_timestamp == 0)
			charging_timestamp = timestamp;
		if ((timestamp - charging_timestamp) >= MS2COUNT(1000))
		{
			if (state)
				charge_state |= 2;
			else
				charge_state &= ~2;
			charge_state |= 0x20;
		}
	}
	else
		charging_timestamp = 0;
	if (charge_state & 0xF0)
	{
		charge_state &= 0x0F;
		switch (charge_state)
		{
			case 0:
			{
				/* Браслет снят с зарядки */
				Message_Add(MESSAGE_BATTERY_CHARGE_OFF, 0, 0, 0);
				break;
			}
			case 2:
			{
				/* Браслет поставлен на зарядку */
				Message_Add(MESSAGE_BATTERY_CHARGE_ON, 0, 0, 0);
				break;
			}
			case 1:
			{
				/* Зарядка окончена */
				Message_Add(MESSAGE_BATTERY_CHARGED, 0, 0, 0);
				break;
			}
			default:
			{
				break;
			}
		}
	}


}
