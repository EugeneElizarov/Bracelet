/*
 * def.h
 *
 *  Created on: 18 мар. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_DEF_H_
#define VENDOR_SPI_DEMO_DEF_H_

#include <stdbool.h>
#include "tl_common.h"

#define BLE_ENABLE		1

#define NEUROCOM_MAIN_SERVICE			"74CDFFF0-C174-458E-9EF2-49062C247857"

#define ARRAY_LENGTH(arr)			(sizeof(arr)/sizeof(arr[0]))

//#define STR_LEN_FROM(from, str)	(str[from] == 0 ? (from) : STR_LEN_FROM((from) + 1, str))

//#define STR_LEN(str)			STR_LEN_FROM(0, str)

//#define STR_LEN(str)			strlen(str)

#define STR_LEN(str)			(sizeof(str) - 1)

#define SIZE_SUBARRAY(subarray_str)		sizeof((char[]){subarray_str})


#undef STR2ARRAY_RESULT

#ifndef STR2ARRAY_SOURCE
#define STR2ARRAY_SOURCE		"12345678901"
#endif

#define STRLENS(a,i)        (a[i] == 0) ? i : // repetitive stuff
#define STRLENPADDED(a)     (STRLENS(a, 0) STRLENS(a, 1) STRLENS(a, 2) STRLENS(a, 3) STRLENS(a, 4) \
                             STRLENS(a, 5) STRLENS(a, 6) STRLENS(a, 7) STRLENS(a, 8) STRLENS(a, 9) \
							 STRLENS(a,10) STRLENS(a,11) STRLENS(a,12) STRLENS(a,13) STRLENS(a,14) \
							 STRLENS(a,15) STRLENS(a,16) STRLENS(a,17) STRLENS(a,18) STRLENS(a,19) \
                             STRLENS(a,20) -1)

#define STRLEN(a)           STRLENPADDED(a) // padding required to prevent 'index out of range' issues.

typedef void *Handle;

#define SIZE_OF_SUBARRAY(subarray)	sizeof((uint8_t[]){subarray})

#define ON								1
#define OFF								0

#define GPIO_PIN_OUT_LOW				GPIO_PIN_PULLDOWN_100K
#define GPIO_PIN_OUT_HIGH				GPIO_PIN_PULLUP_10K
#define GPIO_INPUT						true
#define GPIO_OUTPUT						false
#define GPIO_NO_PULL					GPIO_PIN_UP_DOWN_FLOAT

#define I2C0_SCL						GPIO_PF7
#define I2C0_SDA						GPIO_PF6

#define I2C1_SCL						GPIO_PF0
#define I2C1_SDA						GPIO_PF1

#define TOUCH_RESET						GPIO_PF2
#define TOUCH_INT						GPIO_PF3

#define POWER_EN						GPIO_PC2
#define KEY								GPIO_PA0
#define CHARGING						GPIO_PA2
#define CHARGE_END						GPIO_PA6

#define SLEEP_INTERVAL					60000

typedef void *Handle;

void GPIO_Config(gpio_func_pin_e pin, bool as_input, gpio_pull_type_e pull);

#endif /* VENDOR_SPI_DEMO_DEF_H_ */
