/*
 * colors.h
 *
 *  Created on: 13 мар. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_SPI_DEMO_WIDGETS_COLORS_H_
#define VENDOR_SPI_DEMO_WIDGETS_COLORS_H_

#include "..\lvgl\lvgl.h"

#define LV_FULLCOLOR_GET_R(color)		((color) & 0xFF)
#define LV_FULLCOLOR_GET_G(color)		(((color) >> 8) & 0xFF)
#define LV_FULLCOLOR_GET_B(color)		(((color) >> 16) & 0xFF)
#define LV_COLOR(fullcolor)	(lv_color_t)LV_COLOR_MAKE(((fullcolor) & 0xFF),			\
                                                      (((fullcolor) >> 8) & 0xFF),	\
                                                      (((fullcolor) >> 16) & 0xFF))

#define LV_COLOR_WHITE					LV_COLOR(0xFFFFFF)
#define LV_COLOR_BLACK					LV_COLOR(0x000000)
#define LV_COLOR_RED					LV_COLOR(0x0000FF)
#define LV_COLOR_GREEN					LV_COLOR(0x008000)
#define LV_COLOR_LIME					LV_COLOR(0x00FF00)
#define LV_COLOR_BLUE					LV_COLOR(0xFF0000)
#define LV_COLOR_YELLOW					LV_COLOR(0x00FFFF)

#define LV_COLOR_APP_MAIN				LV_COLOR(0x7C4715)


#endif /* VENDOR_SPI_DEMO_WIDGETS_COLORS_H_ */
