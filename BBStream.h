/*
 * BBStream.h
 *
 *  Created on: 10 апр. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_ACL_PERIPHERAL_DEMO_BBSTREAM_H_
#define VENDOR_ACL_PERIPHERAL_DEMO_BBSTREAM_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	CP_CLOSED,
	CP_UNKNOWN,
	CP_SPRV,
	CP_TSKBM,
	CP_COUNT
}ConnectionProtocol;

#define EVENT_BUFFER_LENGTH		256

typedef enum
{
  BB_EVENT_ADC_24,				// 00	3 байта
  BB_EVENT_ADC_16,				// 01	2 байта
  BB_EVENT_ADC_8,				// 02	1 байт
  BB_EVENT_ADC_OVERFLOW,		// 03	2 байта
  BB_EVENT_EVENT_OVERFLOW,		// 04	(2byte) Кол-во пропущенных событий из-за переполнения буфера
  BB_EVENT_BATTERY_LEVEL,		// 05	3 байта
  BB_EVENT_RSSI,				// 06	1 байт
  BB_EVENT_ACC,					// 07	(6byte): (6 byte - xyz)
  BB_EVENT_GYRO,				// 08	(6byte): (6 byte - xyz)
  BB_EVENT_MAG,					// 09	(6 byte - xyz). Данные от магнетометра.
  BB_EVENT_DEBUG4,				// 10	(4byte) Произвольные 4 байта, которые надо передать на PC для отладки
  BB_EVENT_HR_BAEVSKY,			// 11	(2byte) uint16. Индекс напряжения по Баевскому
  BB_EVENT_SPEED,				// 12	(2byte) Скорость АТС в км/ч умноженная на 256. Если скорость не определена, то 0xFFFF.
  BB_EVENT_DEBUG2,				// 13	(2byte) Произвольные 2 байта, которые надо передать на PC для отладки
  BB_EVENT_HR_RAW,				// 14	(3byte) 24 бита полученных от AFE4404.
  BB_EVENT_HR_PEAK,				// 15	(0byte) событие обозначает что был обнаружен пик пульсовой волны
  BB_EVENT_HR_PULSE,			// 16	(1byte) рассчитанное значение пульса
  BB_EVENT_HR_RR,				// 17	(2byte) интервал между пиками [мс]
  BB_EVENT_GSR,					// 18	(0byte) был зарегистрирован КГР
  BB_EVENT_HAND_CHECK,			// 19	(0byte)
  BB_EVENT_ON_HAND,				// 20	(0byte)
  BB_EVENT_OFF_HAND,			// 21	(0byte)
  BB_EVENT_DRIVER_STATE,		// 22	(1byte) В младших 4 битах - состояние. В старших - причина изменения.
  BB_EVENT_BUTTON_RELEASE,		// 23	(1byte) Сколько времени кнопка была нажата (LSB=100мс).
  BB_EVENT_ACTIVE_ACTION,		// 24	(1byte) Тип активного действия.
  BB_EVENT_ACC_RESET,			// 25	(0byte)
  BB_EVENT_TIME_1S,				// 26	(0byte)
  BB_EVENT_TIME_NS,				// 27	(1byte)
  BB_EVENT_TEMP,				// 28	(1byte)
  BB_EVENT_R,					// 29	(4byte)
  BB_EVENT_DOG_STATE,			// 30	(1byte) Возможные значения: 0 – Состояние 1,1 – Состояние 2, 2 – Состояние 3, 3 – Состояние 4, 4 – Состояние 5
  BB_EVENT_TEMP2,				// 31	(3byte)
  BB_EVENT_PRESSURE,			// 32	(3byte)
  BB_EVENT_INTERVAL_GSR,		// 33	(3byte) Интервал между КГР в мс (если это 1-ый КГР с момента подачи питания, то 0).
  BB_EVENT_HR_ALARM,			// 34	(0byte) Событие "Порог пульса"
  BB_EVENT_HR_RAW4,				// 35	(12 bytes) 4 канала по 24 бита сырых данных с датчика пульса
  BB_EVENT_TSKBM_DATA11,		// 36	(11 bytes) Данные в формате протокола ТСКБМ
  BB_EVENT_TSKBM_DATA16,		// 37	(16 bytes) Данные в формате протокола ТСКБМ
  BB_EVENT_SPRV_ADC_RAW_DATA,	// 38	(60 bytes) Последовательные 20 отсчетов 24-х битных данных АЦП с интервалом 1/128 секунды на отсчет
  BB_EVENT_SPO2,				// 39	(1 byte) Оксигинация в %

  BB_EVENT_COUNT,

  BB_NEW_EVENT_FLAG = 0x80,     	//      Признак нового формата данных. После типа данных идет байт длины данных, затем данные, если они есть
  BB_NEW_EVENT_ADC_24 = (BB_EVENT_ADC_24 | BB_NEW_EVENT_FLAG),
  	  	  	  	  	  	  	  	  	// 00	3 байта
  BB_NEW_EVENT_ADC_16,				// 01	2 байта
  BB_NEW_EVENT_ADC_8,				// 02	1 байт
  BB_NEW_EVENT_ADC_OVERFLOW,		// 03	2 байта
  BB_NEW_EVENT_EVENT_OVERFLOW,		// 04	(2byte) Кол-во пропущенных событий из-за переполнения буфера
  BB_NEW_EVENT_BATTERY_LEVEL,		// 05	3 байта
  BB_NEW_EVENT_RSSI,				// 06	1 байт
  BB_NEW_EVENT_ACC,					// 07	(6byte): (6 byte - xyz)
  BB_NEW_EVENT_GYRO,				// 08	(6byte): (6 byte - xyz)
  BB_NEW_EVENT_MAG,					// 09	(6 byte - xyz). Данные от магнетометра.
  BB_NEW_EVENT_DEBUG4,				// 10	(4byte) Произвольные 4 байта, которые надо передать на PC для отладки
  BB_NEW_EVENT_HR_BAEVSKY,			// 11	(2byte) uint16. Индекс напряжения по Баевскому
  BB_NEW_EVENT_SPEED,				// 12	(2byte) Скорость АТС в км/ч умноженная на 256. Если скорость не определена, то 0xFFFF.
  BB_NEW_EVENT_DEBUG2,				// 13	(2byte) Произвольные 2 байта, которые надо передать на PC для отладки
  BB_NEW_EVENT_HR_RAW,				// 14	(3byte) 24 бита полученных от AFE4404.
  BB_NEW_EVENT_HR_PEAK,				// 15	(0byte) событие обозначает что был обнаружен пик пульсовой волны
  BB_NEW_EVENT_HR_PULSE,			// 16	(1byte) рассчитанное значение пульса
  BB_NEW_EVENT_HR_RR,				// 17	(2byte) интервал между пиками [мс]
  BB_NEW_EVENT_GSR,					// 18	(0byte) был зарегистрирован КГР
  BB_NEW_EVENT_HAND_CHECK,			// 19	(0byte)
  BB_NEW_EVENT_ON_HAND,				// 20	(0byte)
  BB_NEW_EVENT_OFF_HAND,			// 21	(0byte)
  BB_NEW_EVENT_DRIVER_STATE,		// 22	(1byte) В младших 4 битах - состояние. В старших - причина изменения.
  BB_NEW_EVENT_BUTTON_RELEASE,		// 23	(1byte) Сколько времени кнопка была нажата (LSB=100мс).
  BB_NEW_EVENT_ACTIVE_ACTION,		// 24	(1byte) Тип активного действия.
  BB_NEW_EVENT_ACC_RESET,			// 25	(0byte)
  BB_NEW_EVENT_TIME_1S,				// 26	(0byte)
  BB_NEW_EVENT_TIME_NS,				// 27	(1byte)
  BB_NEW_EVENT_TEMP,				// 28	(1byte)
  BB_NEW_EVENT_R,					// 29	(4byte)
  BB_NEW_EVENT_DOG_STATE,			// 30	(1byte) Возможные значения: 0 – Состояние 1,1 – Состояние 2, 2 – Состояние 3, 3 – Состояние 4, 4 – Состояние 5
  BB_NEW_EVENT_TEMP2,				// 31	(3byte)
  BB_NEW_EVENT_PRESSURE,			// 32	(3byte)
  BB_NEW_EVENT_INTERVAL_GSR,		// 33	(3byte) Интервал между КГР в мс (если это 1-ый КГР с момента подачи питания, то 0).
  BB_NEW_EVENT_HR_ALARM,			// 34	(0byte) Событие "Порог пульса"
  BB_NEW_EVENT_HR_RAW4,				// 35	(12 bytes) 4 канала по 24 бита сырых данных с датчика пульса
  BB_NEW_EVENT_TSKBM_DATA11,		// 36	(11 bytes) Данные в формате протокола ТСКБМ
  BB_NEW_EVENT_TSKBM_DATA16,		// 37	(16 bytes) Данные в формате протокола ТСКБМ
  BB_NEW_EVENT_SPRV_ADC_RAW_DATA,	// 38	(60 bytes) Последовательные 20 отсчетов 24-х битных данных АЦП с интервалом 1/128 секунды на отсчет

  BB_TSKBM_PROTOCOL = 0xFE,			// 254  (11 bytes) Заглушка для опознавания данных протокола ТСКБМ
  BB_EVENT_END_PAGE = 0xFF 	// 255	(0byte) Пустое событие (конец страницы во Flash памяти заполняется этим событием).
}DataStreamEventType;

void BBStream_Init(void);
bool BBStream_Send(DataStreamEventType type, void *data);
void BBStream_Poll(void);
void BBStream_SetProtocol(uint16_t connID, ConnectionProtocol protocol);
bool BBStream_TSKBMSend(void *data);

#endif /* VENDOR_ACL_PERIPHERAL_DEMO_BBSTREAM_H_ */
