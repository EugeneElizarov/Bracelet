/*
 * lv_port.c
 *
 *  Created on: 18 февр. 2026 г.
 *      Author: eugen
 */

#include "lv_port.h"
#include "softtmrs.h"
#include "SPD2010.h"
#include "BSPDriver_SPI.h"
#include "../lvgl/lvgl.h"
#include "../def.h"
#include "../messages.h"

//lv_indev_drv_t indev_drv;

#define TFT_CS			GPIO_PE0
#define TFT_LIGHT		GPIO_PE7

#if LV_TICK_CUSTOM == 0
static void lvgl_port_timer_cb(TimerHandle handle)
{
	(void)handle;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
	lv_task_handler();
}
#endif

static void Lvgl_port_rounder_callback(struct _lv_disp_drv_t * disp_drv, lv_area_t * area)
{
  uint16_t x1 = area->x1;
  uint16_t x2 = area->x2;

  // round the start of coordinate down to the nearest 4M number
  area->x1 = (x1 >> 2) << 2;

  // round the end of coordinate up to the nearest 4N+3 number
  area->x2 = ((x2 >> 2) << 2) + 3;
}

static lv_disp_drv_t *flushed_drv = NULL;

static void lvgl_port_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint16_t left = area->x1;
    uint16_t width = area->x2 - area->x1 + 1;
    uint16_t top = area->y1;
    uint16_t height = area->y2 - area->y1 + 1;
    uint32_t size = width;
    size *= height;
   // copy a buffer's content to a specific area of the display
    DRIVER_SPD2010_SetWindow(left, top, width, height);
    flushed_drv = drv;
    DRIVER_SPD2010_Write((TDisplayColor *)color_map, size, false);
    //lv_flushed();
    lv_disp_flush_ready(drv);
    Message_Add(MESSAGE_CLOCK_REFRESH, 0, 0, 0);
}

void lv_flushed(void)
{
	if (flushed_drv)
	{
	  lv_disp_flush_ready(flushed_drv);
	  flushed_drv = NULL;
	}
}

static void lvgl_monitor_cb(lv_disp_drv_t *drv,
                            uint32_t time,
                            uint32_t px)
{
	Message_Add(MESSAGE_CLOCK_REFRESH, 0, 0, 0);
}

void lv_setlight(char state)
{
	gpio_set_level(TFT_LIGHT, state == 0 ? 0 : 1);
}
/*
static void lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    switch (drv->rotated)
    {
    	case LV_DISP_ROT_NONE:
    	{
    		// Rotate LCD display
    		DRIVER_SPD2010_Mirror(true, false);
    		break;
    	}
    	case LV_DISP_ROT_90:
    	{
    		// Rotate LCD display
    		DRIVER_SPD2010_Mirror(true, true);
    		break;
    	}
    	case LV_DISP_ROT_180:
    	{
    		// Rotate LCD display
    		DRIVER_SPD2010_Mirror(false, true);
    		break;
    	}
    	case LV_DISP_ROT_270:
    	{
    		// Rotate LCD display
    		DRIVER_SPD2010_Mirror(false, false);
    		break;
    	}
    	default:
    	{
    		break;
    	}
    }
}
*/

static lv_color_t lvgl_buf1[LVGL_BUF_LEN] __attribute__((aligned(16)));
static lv_color_t lvgl_buf2[LVGL_BUF_LEN] __attribute__((aligned(16)));
static lv_disp_draw_buf_t disp_buf;                                                 // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;                                                      // contains callback functions
static lv_disp_t *disp;

void lv_portinit(void)
{

	BSP_DRIVER_SPI_Init();
	BSP_DRIVER_SPI_Take(BDSID_LSPI, NULL);
	DRIVER_SPD2010_Init(BDSID_LSPI);
	DRIVER_SPD2010_FullDisplaySet();
	//DRIVER_SPD2010_WriteSingleColor(0x00, DRIVER_SPD2010_GetWidth() * DRIVER_SPD2010_GetHeight(), false);

    lv_disp_draw_buf_init(&disp_buf, lvgl_buf1, lvgl_buf2, LVGL_BUF_LEN);                              // initialize LVGL draw buffers

    lv_disp_drv_init(&disp_drv);                                                                        // Create a new screen object and initialize the associated device
    disp_drv.hor_res = DRIVER_SPD2010_GetWidth();
    disp_drv.ver_res = DRIVER_SPD2010_GetHeight();                                                     // Horizontal pixel count
    // disp_drv.rotated = LV_DISP_ROT_90; // е›ѕеѓЏж—‹иЅ¬                                                            // Vertical axis pixel count
    disp_drv.flush_cb = lvgl_port_flush_cb;
    disp_drv.monitor_cb = lvgl_monitor_cb;
    // Function : copy a buffer's content to a specific area of the display
    //disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    disp_drv.rounder_cb = Lvgl_port_rounder_callback;                                    // Function : Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated.
    disp_drv.draw_buf = &disp_buf;                                                                  // LVGL will use this buffer(s) to draw the screens contents
//    disp_drv.user_data = panel_handle;
    disp = lv_disp_drv_register(&disp_drv);
#if LV_TICK_CUSTOM == 0
    SoftTimers_Create(LVGL_TICK_PERIOD_MS, true, lvgl_port_timer_cb);
#endif
    GPIO_Config(TFT_LIGHT, GPIO_OUTPUT, GPIO_PIN_OUT_LOW);

}

void lv_portsleep(void)
{
	DRIVER_SPD2010_Sleep();
	lv_setlight(0);
}

void lv_portwakeup(void)
{
	DRIVER_SPD2010_Wakeup();
	//lv_obj_invalidate(lv_scr_act());
}
