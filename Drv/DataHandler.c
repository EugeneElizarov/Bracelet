/*
 * DataHandler.c
 *
 *  Created on: 13 сент. 2026 г.
 *      Author: eugen
 */

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "DataHandler.h"
#include "softtmrs.h"
#include "../BSP/BSP_battery.h"
#include "../messages.h"
#include "tl_common.h"
#include "stack/ble/ble.h"
#include "../Widgets/clock.h"
#include "../BBStream.h"
#include "../Drv/lv_port.h"
#include "../sensors.h"

#define MAX_BATTERY_VOLTAGE		4100
#define MIN_BATTERY_VOLTAGE		3000
#define TIME_ZONE				(3 * 3600)

#define BATTERY_CHARGING		BIT(0)
#define BATTERY_CHARGED			BIT(1)
#define BRACELET_ON_WRIST		BIT(2)

#define BATTERY_VOLTAGE_KALMAN	5
#define BATTERY_VOLUME_KALMAN	10

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	uint8_t adc_state;

	uint32_t battery_timestamp;
	uint32_t current_time;
	uint32_t sleep_timestamp;

	float R;
	uint32_t bat_percents;
	uint32_t ADC;
	int16_t temperature;
	uint8_t pulse;
	uint8_t SpO2;
	uint8_t GSR_interval;
	bool ble_active;
	bool ble_connected;
	bool hand_on;

	uint32_t bat_voltage;
	uint8_t display;
	uint8_t charge_flag;
	bool sleep;
	bool sensor_active;
	bool connected;
	bool displaysleep;
	bool send_bat_for_time;
}HandledValues;

extern _attribute_ble_data_retention_ dev_char_info_t conn_dev_list[DEVICE_CHAR_INFO_MAX_NUM];

uint32_t Neurocom_TimeTick(void);
struct tm UTC2time(const time_t timer);

static HandledValues values;

static void _clear_values(void)
{
	values.R = 0;
	values.hand_on = false;
	values.pulse = 0;
	values.ADC = 0;
	values.SpO2 = 0;
	values.temperature = 0;
	values.send_bat_for_time = false;
}

static void _enter_deep_sleep(void)
{
	RC_24M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_24M;

	blc_pm_select_internal_32k_crystal();

    pm_set_gpio_wakeup(KEY, WAKEUP_LEVEL_LOW, 1);
    pm_set_gpio_wakeup(CHARGING, WAKEUP_LEVEL_LOW, 1);
    pm_set_gpio_wakeup(CHARGE_END, WAKEUP_LEVEL_LOW, 1);

    pm_set_wakeup_src(PM_WAKEUP_PAD);

    pm_sleep_wakeup(
    	DEEPSLEEP_MODE, //SUSPEND_MODE,
        PM_WAKEUP_PAD,
        PM_TICK_32K,
        0
    );

    sys_reboot();
}

static void DataHandler_ClockTimer(TimerHandle handle)
{
	(void)handle;
	bool sleep = values.sleep;
	bool sensor_active = values.sensor_active;
	bool ble_active = values.ble_active;
	uint32_t ctimestamp = SoftwareTimers_GetCount();

	uint32_t time = Neurocom_TimeTick() + TIME_ZONE;

	if (time != values.current_time)
	{
		bool time_change = false, date_change = false;
		struct tm data_time= UTC2time((time_t)time);

		if (data_time.tm_sec != values.second)
		{
			values.second = data_time.tm_sec;
			//time_change = true;
		}
		if (data_time.tm_min != values.minute)
		{
			values.minute = data_time.tm_min;
			time_change = true;
		}
		if (data_time.tm_hour != values.hour)
		{
			values.hour = data_time.tm_hour;
			time_change = true;
		}
		if (data_time.tm_mday != values.day)
		{
			values.day = data_time.tm_mday;
			date_change = true;
		}
		if (data_time.tm_mon != values.month)
		{
			values.month = data_time.tm_mon;
			date_change = true;
		}
		if (data_time.tm_year != values.year)
		{
			values.year = data_time.tm_year;
			date_change = true;
		}

		if (time_change)
		{
			tlk_printf("Time %02d:%02d:%02d\n", values.hour, values.minute, values.second);
			Message_Add(MESSAGE_PARAM_SET_TIME, 0, P16(values.minute, values.hour), 0);
		}
		if (date_change)
		{
			tlk_printf("Date %02d.%02d.%04d\n", values.day, values.month, values.year);
			Message_Add(MESSAGE_PARAM_SET_DATE, 0, P16(values.day, values.month), values.year);
		}

		values.current_time = time;
	}

	if (((ctimestamp - values.sleep_timestamp) >= MS2COUNT(60000)) && !values.sleep)
	{
		values.sleep = true;

		if (!values.hand_on)
		{
			if (values.charge_flag == 0)
			{
				lv_portsleep();
				gpio_set_low_level(POWER_EN);
				_clear_values();
				Sensors_Sleep();
				tlk_printf("Deep sleep\n");
				while (tlkapi_debug_isBusy())
					tlkapi_debug_handler();
				_enter_deep_sleep();
			}
			values.sensor_active = false;
			if (values.ble_active)
			{
				blc_ll_setAdvEnable(BLC_ADV_DISABLE);
				for (int i = 0; i < ACL_PERIPHR_MAX_NUM; i++)
				{
					if (conn_dev_list[ACL_CENTRAL_MAX_NUM + i].conn_state != 0)
					{
						blc_ll_disconnect(conn_dev_list[ACL_CENTRAL_MAX_NUM + i].conn_handle, HCI_ERR_REMOTE_USER_TERM_CONN);
					}
				}
				while (values.ble_connected)
				{
					blc_sdk_main_loop();
					DataHandler_Poll();
				}
				values.ble_active = false;
				values.ble_connected = false;
			}
		}
		if (values.charge_flag)
		{
			tlk_printf("Sleep. Charge\n");
			if (values.displaysleep)
			{
				gpio_set_high_level(POWER_EN);
				lv_portwakeup();
			}
			Message_Add(MESSAGE_CLOCK_CHANGE_SCREEN, 0, values.displaysleep ? 1 : 0, 0);
			values.displaysleep = false;
		}
		else
		{
			tlk_printf("Sleep. No charge, dspl off\n");
			values.displaysleep = true;
			lv_portsleep();
		}
	}
	if (values.ble_connected)
	{
		if (values.hand_on)
		{
			if (values.pulse)
				BBStream_Send(BB_EVENT_HR_PULSE, &values.pulse);
			if (values.R > 0)
			{
				BBStream_Send(BB_EVENT_R, &values.R);
				BBStream_Send(BB_EVENT_ADC_24, &values.ADC);
			}
			if (values.SpO2)
				BBStream_Send(BB_EVENT_SPO2, &values.SpO2);
			int8_t temp = values.temperature / 256;
			BBStream_Send(BB_EVENT_TEMP, &temp);
			/*
			if (values.GSR_interval)
			{
				values.GSR_interval--;
				if (values.GSR_interval == 0)
					Sensor_GSRImitation();
			}
			*/
		}
	}
}

static bool ClockMessage(Message message)
{
	switch (message->ID)
	{
		case MESSAGE_BLE_CONNECT:
		{
			tlk_printf("Connect\n");
			Message_Add(MESSAGE_PARAM_SET_BLE_CONNECTED, 0, 0, 1);
			values.ble_connected = true;
			//values.GSR_interval = 20;
			break;
		}
		case MESSAGE_BLE_DISCONNECT:
		{
			tlk_printf("Disconnect\n");
			Message_Add(MESSAGE_PARAM_SET_BLE_CONNECTED, 0, 0, 0);
			values.ble_connected = false;
			break;
		}
		case MESSAGE_CHANGE_PULSE:
		{
			tlk_printf("Pulse: %d\n", message->upar32);
			values.pulse = message->upar32;
			if (values.pulse < 20)
				values.pulse = 0;
			Message_Add(MESSAGE_CLOCK_SET_PULSE, 0, values.pulse, 0);
			break;
		}
		case MESSAGE_CHANGE_TEMPERATURE:
		{
			tlk_printf("Temp msg\n");
			values.temperature = message->ipar32;
			break;
		}
		case MESSAGE_CHANGE_RESISTANCE:
		{
			float res;
			memcpy(&res, &(message->upar32), 4);
			res *= 1000;
			if ((res != 0.0f) && ((fabsf(values.R - res) / fabsf(res)) >= 0.1f))
			{
				//tlk_printf("Resistance: %02f\n", res);
				values.R = res;
			}
			break;
		}
		case MESSAGE_CHANGE_ADC_CODE:
		{
			values.ADC = message->upar32;
			break;
		}
		case MESSAGE_CHANGE_SPO2:
		{
			tlk_printf("SpO2: %d\n", message->upar32);
			values.SpO2 = message->upar32;
			break;
		}
		case MESSAGE_PULSE_GSR:
		{
			if (values.ble_connected)
				if (message->upar32)
				{
					tlk_printf("GSR pulse present\n");
					BBStream_Send(BB_EVENT_GSR, NULL);
				}
			break;
		}
		case MESSAGE_BRACELET_HAND_ON:
		{
			tlk_printf("Hand on\n");
			if (!values.ble_active)
			{
				blc_ll_setAdvEnable(BLC_ADV_ENABLE);
				values.ble_active = true;
			}
			if (values.ble_connected)
				BBStream_Send(BB_EVENT_ON_HAND, NULL);
			values.hand_on = true;
			break;
		}
		case MESSAGE_BRACELET_HAND_OFF:
		{
			tlk_printf("Hand off\n");
			if (values.ble_connected)
				BBStream_Send(BB_EVENT_OFF_HAND, NULL);
			//values.hand_on = false;
			_clear_values();
			break;
		}
		case MESSAGE_KEY_CHANGE_STATE:
		{
			if (message->upar8)
			{
				tlk_printf("Key down\n");
				values.sleep_timestamp = SoftwareTimers_GetCount();
				if (values.sleep && (values.charge_flag == 0))
				{
					if (!values.sensor_active)
					{
						gpio_set_high_level(POWER_EN);
						values.sensor_active = true;
					}
					if (values.displaysleep)
					{
						values.displaysleep = false;
						lv_portwakeup();
					}
					Message_Add(MESSAGE_CLOCK_CHANGE_SCREEN, 1, 1, 0);
					tlk_printf("Sleep off\n");
					values.sleep = false;
				}
				/*
				values.display++;
				if (values.display >= Clock_DisplayCount())
					values.display = 0;
				Message_Add(MESSAGE_CLOCK_CHANGE_SCREEN, values.display, 0, 0);
				tlk_printf("Set display %d\n", values.display);
				*/
			}
			else
				tlk_printf("Key up\n");
			break;
		}
		case MESSAGE_BATTERY_CHARGE_OFF:
		{
			if (values.charge_flag != 0)
			{
				values.charge_flag = 0;
				//Message_Add(MESSAGE_CLOCK_SHOW_FLASH, IS_OFF, 0, 0);
				tlk_printf("Sleep off\n");
				values.sleep = false;
				values.sleep_timestamp = SoftwareTimers_GetCount() - MS2COUNT(60000);
				values.sleep = false;
				tlk_printf("Charge off\n");
			}
			break;
		}
		case MESSAGE_BATTERY_CHARGE_ON:
		{
			if (values.charge_flag != BATTERY_CHARGING)
			{
				values.charge_flag = BATTERY_CHARGING;
				//Message_Add(MESSAGE_CLOCK_SHOW_FLASH, IS_FLASH, 0, 0);
				values.sleep_timestamp = SoftwareTimers_GetCount() - MS2COUNT(60000);
				tlk_printf("Charge on\n");
				values.sleep = false;
				_clear_values();
			}
			break;
		}
		case MESSAGE_BATTERY_CHARGED:
		{
			if (values.charge_flag != BATTERY_CHARGED)
			{
				values.charge_flag = BATTERY_CHARGED;
				//Message_Add(MESSAGE_CLOCK_SHOW_FLASH, IS_ON, 0, 0);
				values.sleep_timestamp = SoftwareTimers_GetCount() - MS2COUNT(60000);
				values.sleep = false;
				tlk_printf("Charged\n");
				_clear_values();
			}
			break;
		}
		case MESSAGE_BATTERY_VOLUME:
		{
			Message_Add(MESSAGE_PARAM_SET_BAT_VOLUME, 0, 0, message->upar8);
			break;
		}
		case MESSAGE_PARAM_GET_R:
		{
			Message_Add(MESSAGE_PARAM_SET_R, 0, 0, FLOAT2UINT(values.R));
			break;
		}
		case MESSAGE_PARAM_GET_ADC:
		{
			Message_Add(MESSAGE_PARAM_SET_ADC, 0, 0, values.ADC);
			break;
		}
		case MESSAGE_PARAM_GET_PULSE:
		{
			Message_Add(MESSAGE_PARAM_SET_PULSE, 0, 0, values.pulse);
			break;
		}
		case MESSAGE_PARAM_GET_BAT_VOLUME:
		{
			Message_Add(MESSAGE_PARAM_SET_BAT_VOLUME, 0, 0, values.bat_percents / 100);
			break;
		}
		case MESSAGE_PARAM_GET_RSSI:
		{
			Message_Add(MESSAGE_PARAM_SET_RSSI, 0, 0, 0);
			break;
		}
		case MESSAGE_PARAM_GET_TEMPERATURE:
		{
			Message_Add(MESSAGE_PARAM_SET_TEMPERATURE, 0, 0, INT2UINT(values.temperature));
			break;
		}
		case MESSAGE_PARAM_GET_SPO2:
		{
			Message_Add(MESSAGE_PARAM_SET_SPO2, 0, 0, values.SpO2);
			break;
		}
		case MESSAGE_PARAM_GET_GSR_INTEVAL:
		{
			break;
		}
		case MESSAGE_PARAM_GET_BLE_ACTIVE:
		{
			Message_Add(MESSAGE_PARAM_SET_BLE_ACTIVE, 0, 0, values.ble_active ? 1 : 0);
			break;
		}
		case MESSAGE_PARAM_GET_BLE_CONNECTED:
		{
			Message_Add(MESSAGE_PARAM_SET_BLE_CONNECTED, 0, 0, values.ble_connected? 1 : 0);
			break;
		}
		case MESSAGE_PARAM_GET_HAND_ON:
		{
			Message_Add(MESSAGE_PARAM_SET_HAND_ON, 0, 0, values.hand_on ? 1 : 0);
			break;
		}
		case MESSAGE_PARAM_GET_TIME:
		{
			Message_Add(MESSAGE_PARAM_SET_TIME, 0, P16(values.minute, values.hour), 0);
			break;
		}
		case MESSAGE_PARAM_GET_DATE:
		{
			Message_Add(MESSAGE_PARAM_SET_DATE, 0, P16(values.day, values.month), values.year);
			break;
		}
		default:
		{
			break;
		}
	}
}

void DataHandler_Init(void)
{
	memset(&values, 0, sizeof(values));
	values.battery_timestamp = SoftwareTimers_GetCount();

	SoftTimers_Create(1000, true, DataHandler_ClockTimer);

	Message_AddProcessor(ClockMessage, MESSAGES(MESSAGE_BLE_CONNECT,
												MESSAGE_BLE_DISCONNECT,
												MESSAGE_CHANGE_PULSE,
												MESSAGE_CHANGE_TEMPERATURE,
												MESSAGE_CHANGE_RESISTANCE,
												MESSAGE_CHANGE_ADC_CODE,
												MESSAGE_CHANGE_SPO2,
												MESSAGE_PULSE_GSR,
												MESSAGE_BRACELET_HAND_ON,
												MESSAGE_BRACELET_HAND_OFF,
												MESSAGE_KEY_CHANGE_STATE,
												MESSAGE_BATTERY_CHARGE_OFF,
												MESSAGE_BATTERY_CHARGE_ON,
												MESSAGE_BATTERY_CHARGED,
												MESSAGE_BATTERY_VOLUME,
												MESSAGE_PARAM_GET_R,
												MESSAGE_PARAM_GET_ADC,
												MESSAGE_PARAM_GET_PULSE,
												MESSAGE_PARAM_GET_BAT_VOLUME,
												MESSAGE_PARAM_GET_RSSI,
												MESSAGE_PARAM_GET_TEMPERATURE,
												MESSAGE_PARAM_GET_SPO2,
												MESSAGE_PARAM_GET_GSR_INTEVAL,
												MESSAGE_PARAM_GET_BLE_ACTIVE,
												MESSAGE_PARAM_GET_BLE_CONNECTED,
												MESSAGE_PARAM_GET_HAND_ON,
												MESSAGE_PARAM_GET_TIME,
												MESSAGE_PARAM_GET_DATE));

	BSP_BatteryInit();
	BSP_BatteryADCPowerSet(true);
	SoftwareTimers_Delay(2);
	values.adc_state = 1;
}

uint8_t bat_out_counter = 0;
static int8_t key_emulated = -1;

void DataHandler_Poll(void)
{
	uint32_t timestamp = SoftwareTimers_GetCount();
	/* Обработка заряда батареи */
	switch (values.adc_state)
	{
		case 0:		// Ожидаем интервалда измерения
		{
			if (((timestamp - values.battery_timestamp) >= MS2COUNT(1000)) || !values.send_bat_for_time)
			{
				values.battery_timestamp = timestamp;
				BSP_BatteryADCPowerSet(true);
				if (key_emulated >= 0)
					key_emulated++;
				if (key_emulated >= 45)
				{
					Message_Add(MESSAGE_KEY_CHANGE_STATE, 1, 0, 0);
					key_emulated = 0;
				}
				values.adc_state++;
			}
			break;
		}
		case 1:		// Ожидаем включения АЦП
		{
			if ((timestamp - values.battery_timestamp) >= MS2COUNT(2))
				values.adc_state++;
			break;
		}
		case 2:		// Запускаем измерение
		{
			BSP_BatteryMesaureStart();
			values.adc_state++;
			break;
		}
		case 3:		// Ждем окончания измерения
		{
			if (BSP_BatteryMeasured())
			{
				uint32_t voltage = BSP_BatteryLevelGet();
				if (bat_out_counter < 0xFF)
					bat_out_counter++;
				if ((values.bat_voltage != 0) && values.send_bat_for_time)
					voltage = ((voltage * BATTERY_VOLTAGE_KALMAN) + (uint32_t)values.bat_voltage * (100 - BATTERY_VOLTAGE_KALMAN)) / 100;

				if ((voltage != values.bat_voltage) || !values.send_bat_for_time)
				{
					int32_t percents;
					if ((bat_out_counter > 10) || !values.send_bat_for_time)
					{
						tlk_printf("Bat voltage %d\n", voltage);
						bat_out_counter = 0;
					}
					values.bat_voltage = voltage;
					if (voltage <= MIN_BATTERY_VOLTAGE)
						percents = 0;
					else if (voltage >= MAX_BATTERY_VOLTAGE)
						percents = 10000;
					else
						percents = ((voltage - MIN_BATTERY_VOLTAGE) * 10000 / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE));

					if ((values.bat_percents != 0) && values.send_bat_for_time)
					{
						uint32_t perc = (uint32_t)percents * BATTERY_VOLUME_KALMAN;
						perc += ((uint32_t)values.bat_percents * (100 - BATTERY_VOLUME_KALMAN));
						percents = perc / 100;
					}

					if (values.charge_flag & BATTERY_CHARGED)
						percents = 10000;

					if (percents >= 10000)
					{
						if (((values.charge_flag & BATTERY_CHARGED) != 0) || (values.charge_flag == 0))
							percents = 10000;
						else
							percents = 9900;
					}

					if (percents == 0)
						percents = 1;


					if ((percents != values.bat_percents) || !values.send_bat_for_time)
					{
						uint8_t perc = percents / 100;
						if ((perc != (values.bat_percents / 100)) || !values.send_bat_for_time)
						{
							Message_Add(MESSAGE_BATTERY_VOLUME, perc, 0, 0);
							tlk_printf("Battery %d%%\n", perc);
							values.send_bat_for_time = true;
						}
						values.bat_percents = percents;
					}
				}
				BSP_BatteryADCPowerSet(false);
				values.adc_state = 0;
			}
			break;
		}
	}
	/* Обработка наличия соединения */

	bool connected = false;
	for (unsigned char i = 0; i < ACL_PERIPHR_MAX_NUM; i++)
	{
		if (conn_dev_list[i + ACL_CENTRAL_MAX_NUM].conn_state)
		{
			connected = true;
			break;
		}
	}
	if (connected != values.connected)
	{
		values.connected = connected;
		Message_Add(connected ? MESSAGE_BLE_CONNECT : MESSAGE_BLE_DISCONNECT, 0, 0, 0);
	}
}