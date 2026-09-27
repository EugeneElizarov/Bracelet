/*
 * GPIOGroupInt.h
 *
 *  Created on: 16 мар. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_DRV_GPIOGROUPINT_H_BAK_
#define VENDOR_SPI_DEMO_DRV_GPIOGROUPINT_H_BAK_

#include "tl_common.h"

typedef void (* GPIOGroupInt_cb)(uint8_t group);

int8_t GPIO_GroupRegister(gpio_func_pin_e pin, gpio_irq_trigger_type_e type, uint8_t timeout, GPIOGroupInt_cb cb);
void GPIO_GroupEnable(uint8_t group);
void GPIO_GroupDisable(uint8_t group);
void GPIO_GroupBZZZSetTimeout(uint8_t group, uint8_t timeout);
void GPIO_GroupPoll(void);
bool GPIO_GroupIsEvent(uint8_t group);
int8_t GPIO_GroupFromPin(gpio_func_pin_e pin);


#endif /* VENDOR_SPI_DEMO_DRV_GPIOGROUPINT_H_BAK_ */
