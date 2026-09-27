/*
 * sensors.c
 *
 *  Created on: 13 июн. 2026 г.
 *      Author: eugen
 */

#include "tl_common.h"
#include "sensors.h"
#include "BSP\\BSP_uarts.h"
#include "BBStream.h"
#include "messages.h"

static uint8_t rx_buffer[128];
static uint8_t rx_data_size = 0;

#define GSR_PRESENT				0x01
#define BRACELET_PRESENT				0x02

typedef union
{
	struct
	{
	  uint8_t gsr_present : 1;
	  uint8_t bracelet_present : 1;
	  uint8_t reserved : 6;
	};
	uint8_t data;
}DataFlag;

typedef union
{
	struct
	{
	  uint32_t adc_code : 24;
	  uint32_t spo2 : 8;
	};
	uint32_t data;
}ADC_SPo2;

typedef struct __attribute__((packed))
{
	// Поле				Смещение	Длина
	uint16_t count;		// 0		2
	DataFlag flags;		// 2		1
	uint8_t pulse;		// 3		1
	int16_t temp;		// 4		2 Температура
	uint16_t acc[3];	// 6 		2 * 3 = 6
	float R;			// 10		4
	ADC_SPo2 ADC;		// 14		3
	//SPO2				// 17		1
}SensorData; // 16

static SensorData sensor_data __attribute__((aligned(4)));
static SensorData cdata;
static bool gsr_imitation = false;

#define CHECK_CHANGE(sdata, cdata, field, msg)				if (sdata.field != cdata.field) {sdata.field = cdata.field;Message_Add(msg, 0, 0, 0);}
#define CHECK_CHANGE_VALUE(sdata, cdata, field, msg)		if (sdata.field != cdata.field) {sdata.field = cdata.field;Message_Add(msg, 0, 0, cdata.field);}
#define CHECK_CHANGE_DUAL(sdata, cdata, field, msg1, msg2)	if (sdata.field != cdata.field) {if (cdata.field) {Message_Add(msg1, 0, 0, 0);} else {Message_Add(msg2, 0, 0, 0);} sdata.field = cdata.field;}

static void _sensors_UART0_cb(uart_num_e UART, uint8_t msg, void *data, int count)
{
	if (msg == MESSAGE_UART_RECEIVE_DATA)
	{
		bool prntf = true;
		uint16_t data_size;
		float R;
		count = UART_GetRXCount(UART0);
		if (count > sizeof(sensor_data))
			count = sizeof(sensor_data);
		rx_data_size = UART_Get(UART0, &sensor_data, sizeof(sensor_data));
		if (count >= rx_data_size)
			UART_Get(UART0, NULL, count);
		if ((sensor_data.count + 2) == rx_data_size)
		{
			/*
			if (gsr_imitation)
			{
				sensor_data.flags.gsr_present = 1;
				gsr_imitation = false;
			}
			*/
			if (cdata.flags.bracelet_present != sensor_data.flags.bracelet_present)
			{
				cdata.flags.bracelet_present = sensor_data.flags.bracelet_present;
				Message_Add(cdata.flags.bracelet_present != 0 ? MESSAGE_BRACELET_HAND_ON : MESSAGE_BRACELET_HAND_OFF, 0, 0, 0);
			}
			if (sensor_data.flags.gsr_present)
				Message_Add(MESSAGE_PULSE_GSR, 0, 0, 1);
			CHECK_CHANGE_VALUE(cdata, sensor_data, pulse, MESSAGE_CHANGE_PULSE);
			CHECK_CHANGE_VALUE(cdata, sensor_data, temp, MESSAGE_CHANGE_TEMPERATURE);
			CHECK_CHANGE_VALUE(cdata, sensor_data, ADC.spo2, MESSAGE_CHANGE_SPO2);

			if (cdata.ADC.adc_code != sensor_data.ADC.adc_code)
			{
				Message_Add(MESSAGE_CHANGE_ADC_CODE, 0, 0, sensor_data.ADC.adc_code);
				Message_Add(MESSAGE_CHANGE_RESISTANCE, 0, 0, FLOAT2UINT(sensor_data.R));
				cdata.ADC.adc_code = sensor_data.ADC.adc_code;
				cdata.R = sensor_data.R;
			}
			BBStream_TSKBMSend(&sensor_data);
		}
		else
		{
			if ((rx_data_size > 0))
				tlkapi_send_str_data("Data", &sensor_data, rx_data_size);
		}
		rx_data_size = 0;
	}
}


void Sensors_Init(void)
{
	memset(&cdata, 0, sizeof(cdata));
	UART_Set_cb(UART0, _sensors_UART0_cb);
}

void Sensors_ResetAlarm(void)
{
//	GSR_active = CS_UNKNOWN;
//	Message_Add(MESSAGE_CLOCK_SHOW_HAND, IS_OFF, 0, 0);
//	GSR_timestamp = 0;
}

void Sensors_Sleep(void)
{
	memset(&cdata, 0, sizeof(cdata));
}

void Sensors_Wakeup(void)
{
}

void Sensor_GSRImitation(void)
{
	gsr_imitation = true;
}
