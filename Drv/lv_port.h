/*
 * lv_port.h
 *
 *  Created on: 18 февр. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_DRV_LV_PORT_H_
#define VENDOR_SPI_DEMO_DRV_LV_PORT_H_

#define LVGL_TICK_PERIOD_MS					5
#define LVGL_BUF_LEN						(412 * 25)

void lv_portinit(void);
void lv_portsleep(void);
void lv_portwakeup(void);
void lv_flushed(void);
void lv_setlight(char state);

#endif /* VENDOR_SPI_DEMO_DRV_LV_PORT_H_ */
