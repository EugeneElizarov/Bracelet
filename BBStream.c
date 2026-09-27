/*
 * BBStream.c
 *
 *  Created on: 10 апр. 2026 г.
 *      Author: eugen
 */

#include <stddef.h>
#include <stdint.h>
#include "BBStream.h"
#include "app_config.h"
#include "Drv/softtmrs.h"
#include "types.h"
#include "device_manage.h"
#include "app_att.h"
#include "app_config.h"

#include "utils\ring_buffer.h"

static const u8 event_length[] =
{
	[BB_EVENT_ADC_24]		  		= 3,
	[BB_EVENT_ADC_16]		  		= 2,
	[BB_EVENT_ADC_8]		  		= 1,
	[BB_EVENT_ADC_OVERFLOW]	  		= 2,
	[BB_EVENT_EVENT_OVERFLOW] 		= 0x82,
	[BB_EVENT_BATTERY_LEVEL]  		= 3,
	[BB_EVENT_RSSI]			  		= 1,
	[BB_EVENT_ACC]			  		= 6,
	[BB_EVENT_GYRO]			  		= 6,
	[BB_EVENT_MAG]			  		= 6,
	[BB_EVENT_DEBUG4]		  		= 4,
	[BB_EVENT_HR_BAEVSKY]	  		= 2,
	[BB_EVENT_SPEED]		  		= 2,
	[BB_EVENT_DEBUG2]		  		= 2,
	[BB_EVENT_HR_RAW]		  		= 3,
	[BB_EVENT_HR_PEAK]		  		= 0,
	[BB_EVENT_HR_PULSE]		  		= 1,
	[BB_EVENT_HR_RR]		  		= 2,
	[BB_EVENT_GSR]			  		= 0,
	[BB_EVENT_HAND_CHECK]	  		= 0,
	[BB_EVENT_ON_HAND]		  		= 0,
	[BB_EVENT_OFF_HAND]		  		= 0,
	[BB_EVENT_DRIVER_STATE]	  		= 1,
	[BB_EVENT_BUTTON_RELEASE] 		= 1,
	[BB_EVENT_ACTIVE_ACTION]  		= 1,
	[BB_EVENT_ACC_RESET]	  		= 0,
	[BB_EVENT_TIME_1S]		  		= 0x80,
	[BB_EVENT_TIME_NS]		  		= 0x81,
	[BB_EVENT_TEMP]			  		= 1,
	[BB_EVENT_R]			  		= 4,
	[BB_EVENT_DOG_STATE]	  		= 1,
	[BB_EVENT_TEMP2]		  		= 3,
	[BB_EVENT_PRESSURE]		  		= 3,
	[BB_EVENT_INTERVAL_GSR]	  		= 3,
	[BB_EVENT_HR_ALARM]       		= 0,
	[BB_EVENT_HR_RAW4]		  		= 12,
	[BB_EVENT_TSKBM_DATA11]   	 	= 11,
	[BB_EVENT_TSKBM_DATA16]   	 	= 16,
	[BB_EVENT_SPRV_ADC_RAW_DATA]	= 60,
	[BB_EVENT_SPO2]					= 1,
};

static u8 send_event_block_counter = 0;
static u8 current_connection = 0;
static uint32_t data_stream_timestamp = 0;

typedef struct
{
	ConnectionProtocol protocol;
	Handle buffer;
}Connection;

static uint8_t conn_data_buffers[ACL_PERIPHR_MAX_NUM][EVENT_BUFFER_LENGTH];
static Connection conn_desc[ACL_PERIPHR_MAX_NUM];

void BBStream_Init(void)
{
	current_connection = 0;
	for (int i = 0; i < ACL_PERIPHR_MAX_NUM; i++)
	{
		conn_desc[i].buffer = NULL;
		conn_desc[i].protocol = CP_CLOSED;
	}
}

static uint8_t tmp_data_buffer[255];

void _send_to_data_stream(DataStreamEventType type, void *data)
{
	uint8_t length = event_length[type] & 0x7F;

	tmp_data_buffer[0] = send_event_block_counter++;
	tmp_data_buffer[1] = type;
	if ((data != NULL) && (length > 0))
		memcpy(&(tmp_data_buffer[2]), data, length);

	for (int i = 0; i < ACL_PERIPHR_MAX_NUM; i++)
	{
		switch (conn_desc[i].protocol)
		{
			case CP_SPRV:
			{
				RingBuffer_AddData(conn_desc[i].buffer, tmp_data_buffer, length + 2);
				break;
			}
			case CP_TSKBM:
			{
				if ((type == BB_EVENT_TSKBM_DATA11) || (type == BB_EVENT_TSKBM_DATA16))
                    memcpy(conn_desc[i].buffer, data, length);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

bool BBStream_Send(DataStreamEventType type, void *data)
{
	if ((type < BB_EVENT_COUNT) && (event_length[type] < 0x80))
	{
		_send_to_data_stream(type, data);
		return true;
	}
	return false;
}

bool BBStream_TSKBMSend(void *data)
{
	_send_to_data_stream(BB_EVENT_TSKBM_DATA11, data);
}

static u8 stream_send_buffer[256];

void BBStream_Poll(void)
{
	uint32_t timestamp = SoftwareTimers_GetCount();

	if (current_connection >= ACL_PERIPHR_MAX_NUM)
		current_connection = 0;


    if (conn_dev_list[ACL_CENTRAL_MAX_NUM + current_connection].conn_state)
    {
    	if (conn_desc[current_connection].protocol == CP_CLOSED)
    	{
    		conn_desc[current_connection].protocol = CP_UNKNOWN;
    		conn_desc[current_connection].buffer = NULL;
			tlk_printf("Connection #%d open\n", current_connection);
    	}
    	switch (conn_desc[current_connection].protocol)
    	{
#if NEURO_SENSOR_PROTOCOL != 2
    		case CP_SPRV:
    		{
   				uint8_t count = RingBuffer_ReadData(conn_desc[current_connection].buffer, stream_send_buffer, sizeof(stream_send_buffer));

   				if (count)
   				{
       				if (blc_gatt_pushHandleValueNotify(conn_dev_list[ACL_CENTRAL_MAX_NUM + current_connection].conn_handle, Neurocom_Data_Stream_DP_H, stream_send_buffer, count) == BLE_SUCCESS)
       					RingBuffer_DeleteData(conn_desc[current_connection].buffer);
       				else
       				{
       					tlkapi_printf(true, "Push data (size %d b) error.\n", count);
       				}
       			}
    			break;
    		}
#endif
#if NEURO_SENSOR_PROTOCOL == 2
    		case CP_TSKBM:
    		{
    			if (conn_desc[current_connection].buffer &&
                    ((uint8_t *)conn_desc[current_connection].buffer)[0] != 0)
    			{
    				if (blc_gatt_pushHandleValueNotify(conn_dev_list[ACL_CENTRAL_MAX_NUM + current_connection].conn_handle, TSKBM_Data_DataEDR_DP_H, conn_desc[current_connection].buffer, event_length[BB_EVENT_TSKBM_DATA11]) == BLE_SUCCESS)
    					((uint8_t *)(conn_desc[current_connection].buffer))[0] = 0;
    			}
    		}
#endif
    	}
    }
    else
	{
		if (conn_desc[current_connection].protocol != CP_CLOSED)
		{
			tlk_printf("Connection #%d close\n", current_connection);
			conn_desc[current_connection].protocol = CP_CLOSED;
			conn_desc[current_connection].buffer = NULL;
		}
	}
	if (COUNT2MS(timestamp - data_stream_timestamp) >= 1000)
	{
		_send_to_data_stream(BB_EVENT_TIME_1S, NULL);
		data_stream_timestamp += MS2COUNT(1000);
	}
    current_connection++;
}

void BBStream_SetProtocol(uint16_t connID, ConnectionProtocol protocol)
{
	if (protocol < CP_COUNT)
	{
		for (int i = 0; i < ACL_PERIPHR_MAX_NUM; i++)
		{
			if (conn_dev_list[ACL_CENTRAL_MAX_NUM + i].conn_state != 0)
			{
				if ((conn_dev_list[ACL_CENTRAL_MAX_NUM + i].conn_handle == connID) &&
			    (conn_desc[i].protocol == CP_UNKNOWN))
				{
					switch (protocol)
					{
						case CP_SPRV:
						{
							conn_desc[i].buffer = RingBuffer_Init(conn_data_buffers[i], EVENT_BUFFER_LENGTH);
							break;
						}
						case CP_TSKBM:
						{
							conn_desc[i].buffer = conn_data_buffers[i];
							conn_data_buffers[i][0] = 0;
							break;
						}
						default:
						{
							conn_desc[i].buffer = NULL;
						}
					}
					tlk_printf("Conn #%d set protocol to %s\n", i, (protocol == CP_SPRV) ? "SPRV" : "TSKBM");
					conn_desc[i].protocol = protocol;
					return;
				}
			}
		}
	}
}
