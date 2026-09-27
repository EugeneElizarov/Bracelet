/*
 * uarts.h
 *
 *  Created on: 2 июн. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_ACL_PERIPHERAL_DEMO_BSP_UARTS_H_
#define VENDOR_ACL_PERIPHERAL_DEMO_BSP_UARTS_H_

#include "tl_common.h"

#define UART_RX_DMA_BUFFER_SIZE   1024
#define UART_RX_RING_BUFFER_SIZE  2048
#define UART_TX_BUFFER_SIZE       256
#define UART_DEFAULT_BAUDRATE     115200
#define UART_DEFAULT_TIMEOUT      2

typedef void (* UART_cb)(uart_num_e UART, uint8_t msg, void *data, int count);

void UARTS_Init(void);
bool UART_Set_cb(uart_num_e UART, UART_cb cb);
bool UART_Send(uart_num_e UART, void *data, int length);
bool UART_SendStr(uart_num_e UART, void *str);
uint16_t UART_GetRXCount(uart_num_e UART);
uint16_t UART_Get(uart_num_e UART, void *buffer, uint16_t buffer_count);
void UART_Poll(void);

#endif /* VENDOR_ACL_PERIPHERAL_DEMO_BSP_UARTS_H_ */
