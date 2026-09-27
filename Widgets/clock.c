/*
 * clock.c
 *
 *  Created on: 5 мар. 2026 г.
 *      Author: eugen
 */

#include "../Drv/softtmrs.h"
#include "clock.h"
#include "colors.h"
#include "../messages.h"
#include "../Drv/lv_port.h"

lv_color_t main_screen_back(void);
static bool ClockMessage(Message message);

#define CLOCK_STEP				    250

#define LV_COLOR_LTEXT				LV_COLOR_MAKE(0x15, 0x47, 0x7C)
#define LV_COLOR_LTEXT_SELECTION	LV_COLOR_GRAY
#define LV_COLOR_COMPANY_MAIN		LV_COLOR_MAKE(0x15, 0x47, 0x7C)

#define LABEL_SIZE(width, height)	(void *)((((uint32_t)((width) & 0xFFFF)) << 16) + ((height) & 0xFFFF))
#define LABEL_WIDTH(label)			(uint16_t)((((uint32_t)((label)->object)) >> 16) & 0xFFFF)
#define LABEL_HEIGHT(label)			(uint16_t)(((uint32_t)((label)->object)) & 0xFFFF)

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(array)				(sizeof(array) / sizeof(array[0]))
#endif

LV_FONT_DECLARE(a14bs);
LV_FONT_DECLARE(arial14b);
LV_FONT_DECLARE(Bahnschrift_40);

LV_IMG_DECLARE(H0);
LV_IMG_DECLARE(H1);
LV_IMG_DECLARE(H2);
LV_IMG_DECLARE(H3);
LV_IMG_DECLARE(H4);
LV_IMG_DECLARE(H5);
LV_IMG_DECLARE(H6);
LV_IMG_DECLARE(H7);
LV_IMG_DECLARE(H8);
LV_IMG_DECLARE(H9);
LV_IMG_DECLARE(H10);
LV_IMG_DECLARE(H11);
LV_IMG_DECLARE(H12);
LV_IMG_DECLARE(H13);
LV_IMG_DECLARE(H14);
LV_IMG_DECLARE(H15);
LV_IMG_DECLARE(H16);
LV_IMG_DECLARE(H17);
LV_IMG_DECLARE(H18);
LV_IMG_DECLARE(H19);
LV_IMG_DECLARE(H20);
LV_IMG_DECLARE(H21);
LV_IMG_DECLARE(H22);
LV_IMG_DECLARE(H23);
LV_IMG_DECLARE(H24);
LV_IMG_DECLARE(H25);
LV_IMG_DECLARE(H26);
LV_IMG_DECLARE(H27);
LV_IMG_DECLARE(H28);
LV_IMG_DECLARE(H29);
LV_IMG_DECLARE(H30);
LV_IMG_DECLARE(H31);
LV_IMG_DECLARE(H32);
LV_IMG_DECLARE(H33);
LV_IMG_DECLARE(H34);
LV_IMG_DECLARE(H35);
LV_IMG_DECLARE(H36);
LV_IMG_DECLARE(H37);
LV_IMG_DECLARE(H38);
LV_IMG_DECLARE(H39);
LV_IMG_DECLARE(H40);
LV_IMG_DECLARE(H41);
LV_IMG_DECLARE(H42);
LV_IMG_DECLARE(H43);
LV_IMG_DECLARE(H44);
LV_IMG_DECLARE(H45);
LV_IMG_DECLARE(H46);
LV_IMG_DECLARE(H47);
LV_IMG_DECLARE(H48);
LV_IMG_DECLARE(H49);
LV_IMG_DECLARE(H50);
LV_IMG_DECLARE(H51);
LV_IMG_DECLARE(H52);
LV_IMG_DECLARE(H53);
LV_IMG_DECLARE(H54);
LV_IMG_DECLARE(H55);
LV_IMG_DECLARE(H56);
LV_IMG_DECLARE(H57);
LV_IMG_DECLARE(H58);
LV_IMG_DECLARE(H59);

LV_IMG_DECLARE(M0);
LV_IMG_DECLARE(M1);
LV_IMG_DECLARE(M2);
LV_IMG_DECLARE(M3);
LV_IMG_DECLARE(M4);
LV_IMG_DECLARE(M5);
LV_IMG_DECLARE(M6);
LV_IMG_DECLARE(M7);
LV_IMG_DECLARE(M8);
LV_IMG_DECLARE(M9);
LV_IMG_DECLARE(M10);
LV_IMG_DECLARE(M11);
LV_IMG_DECLARE(M12);
LV_IMG_DECLARE(M13);
LV_IMG_DECLARE(M14);
LV_IMG_DECLARE(M15);
LV_IMG_DECLARE(M16);
LV_IMG_DECLARE(M17);
LV_IMG_DECLARE(M18);
LV_IMG_DECLARE(M19);
LV_IMG_DECLARE(M20);
LV_IMG_DECLARE(M21);
LV_IMG_DECLARE(M22);
LV_IMG_DECLARE(M23);
LV_IMG_DECLARE(M24);
LV_IMG_DECLARE(M25);
LV_IMG_DECLARE(M26);
LV_IMG_DECLARE(M27);
LV_IMG_DECLARE(M28);
LV_IMG_DECLARE(M29);
LV_IMG_DECLARE(M30);
LV_IMG_DECLARE(M31);
LV_IMG_DECLARE(M32);
LV_IMG_DECLARE(M33);
LV_IMG_DECLARE(M34);
LV_IMG_DECLARE(M35);
LV_IMG_DECLARE(M36);
LV_IMG_DECLARE(M37);
LV_IMG_DECLARE(M38);
LV_IMG_DECLARE(M39);
LV_IMG_DECLARE(M40);
LV_IMG_DECLARE(M41);
LV_IMG_DECLARE(M42);
LV_IMG_DECLARE(M43);
LV_IMG_DECLARE(M44);
LV_IMG_DECLARE(M45);
LV_IMG_DECLARE(M46);
LV_IMG_DECLARE(M47);
LV_IMG_DECLARE(M48);
LV_IMG_DECLARE(M49);
LV_IMG_DECLARE(M50);
LV_IMG_DECLARE(M51);
LV_IMG_DECLARE(M52);
LV_IMG_DECLARE(M53);
LV_IMG_DECLARE(M54);
LV_IMG_DECLARE(M55);
LV_IMG_DECLARE(M56);
LV_IMG_DECLARE(M57);
LV_IMG_DECLARE(M58);
LV_IMG_DECLARE(M59);

LV_IMG_DECLARE(BigHeart);
LV_IMG_DECLARE(SmallHeart);
LV_IMG_DECLARE(Hand);
LV_IMG_DECLARE(Flash);
LV_IMG_DECLARE(BLENo);
LV_IMG_DECLARE(BLE0);
LV_IMG_DECLARE(BLE1);
LV_IMG_DECLARE(BLE2);
LV_IMG_DECLARE(BLEMax);

LV_IMG_DECLARE(MainScreen);

LV_IMG_DECLARE(AM);
LV_IMG_DECLARE(PM);

LV_IMG_DECLARE(Charge);
LV_IMG_DECLARE(BiggestHeart);
LV_IMG_DECLARE(bpm);
LV_IMG_DECLARE(SpO2);
LV_IMG_DECLARE(SpO2_perc);
LV_IMG_DECLARE(GSR);
LV_IMG_DECLARE(GSR_sec);

typedef enum
{
  //OID_BACK,
  OID_MAIN_SCREEN,
  OID_H0,
  OID_H1,
  OID_H2,
  OID_H3,
  OID_H4,
  OID_H5,
  OID_H6,
  OID_H7,
  OID_H8,
  OID_H9,
  OID_H10,
  OID_H11,
  OID_H12,
  OID_H13,
  OID_H14,
  OID_H15,
  OID_H16,
  OID_H17,
  OID_H18,
  OID_H19,
  OID_H20,
  OID_H21,
  OID_H22,
  OID_H23,
  OID_H24,
  OID_H25,
  OID_H26,
  OID_H27,
  OID_H28,
  OID_H29,
  OID_H30,
  OID_H31,
  OID_H32,
  OID_H33,
  OID_H34,
  OID_H35,
  OID_H36,
  OID_H37,
  OID_H38,
  OID_H39,
  OID_H40,
  OID_H41,
  OID_H42,
  OID_H43,
  OID_H44,
  OID_H45,
  OID_H46,
  OID_H47,
  OID_H48,
  OID_H49,
  OID_H50,
  OID_H51,
  OID_H52,
  OID_H53,
  OID_H54,
  OID_H55,
  OID_H56,
  OID_H57,
  OID_H58,
  OID_H59,
  OID_M0,
  OID_M1,
  OID_M2,
  OID_M3,
  OID_M4,
  OID_M5,
  OID_M6,
  OID_M7,
  OID_M8,
  OID_M9,
  OID_M10,
  OID_M11,
  OID_M12,
  OID_M13,
  OID_M14,
  OID_M15,
  OID_M16,
  OID_M17,
  OID_M18,
  OID_M19,
  OID_M20,
  OID_M21,
  OID_M22,
  OID_M23,
  OID_M24,
  OID_M25,
  OID_M26,
  OID_M27,
  OID_M28,
  OID_M29,
  OID_M30,
  OID_M31,
  OID_M32,
  OID_M33,
  OID_M34,
  OID_M35,
  OID_M36,
  OID_M37,
  OID_M38,
  OID_M39,
  OID_M40,
  OID_M41,
  OID_M42,
  OID_M43,
  OID_M44,
  OID_M45,
  OID_M46,
  OID_M47,
  OID_M48,
  OID_M49,
  OID_M50,
  OID_M51,
  OID_M52,
  OID_M53,
  OID_M54,
  OID_M55,
  OID_M56,
  OID_M57,
  OID_M58,
  OID_M59,
  OID_BIGHEART,
  OID_SMALLHEART,
  OID_HAND,
  OID_FLASH,
  OID_BLENO,
  OID_BLE0,
  OID_BLE1,
  OID_BLE2,
  OID_BLEMAX,
  OID_DAY,
  OID_MONTH,
  OID_PULSE,
  OID_AMPM,
  OID_PERCENT,
  OID_BATTERY,
  OID_BATTERYLEVEL,
  OID_BIGGESTHEART,
  OID_BPM,
  OID_SPO2,
  OID_SPO2PERC,
  OID_GSR,
  OID_GSRSEC,
  OID_CHARGE_VALUE,
  OID_VALUE,
  OID_BP,
  OID_COUNT
}Objects_ID;

typedef struct
{
	lv_obj_t *object;
	lv_coord_t x;
	lv_coord_t y;
}object_desc;

#define GET_FLAG(flag)			((uint32_t)1 << (flag))

#define GET_FLAGS2(flag1,														\
                   flag2)		(GET_FLAG(flag1) | GET_FLAG(flag2))

#define GET_FLAGS3(flag1,														\
	               flag2,														\
				   flag3)		(GET_FLAGS2(flag1, flag2) | GET_FLAG(flag3))

#define GET_FLAGS4(flag1,														\
        		   flag2,														\
				   flag3,														\
				   flag4)		(GET_FLAGS2(flag1, flag2) | 					\
                                 GET_FLAGS2(flag3, flag4))

#define GET_FLAGS5(flag1,														\
        		   flag2,														\
				   flag3,														\
				   flag4,														\
				   flag5)		(GET_FLAGS3(flag1, flag2, flag3) |				\
                                 GET_FLAGS2(flag4, flag5))

#define GET_FLAGS6(flag1,														\
        		   flag2,														\
				   flag3,														\
				   flag4,														\
				   flag5,														\
		           flag6)		(GET_FLAGS3(flag1, flag2, flag3) |				\
                                 GET_FLAGS3(flag4, flag5, flag6))

#define GET_FLAGS7(flag1,														\
        		   flag2,														\
				   flag3,														\
				   flag4,														\
				   flag5,														\
				   flag6,														\
		           flag7)		(GET_FLAGS4(flag1, flag2, flag3, flag4) |		\
                                 GET_FLAGS3(flag5, flag6, flag7))

#define GET_FLAGS7(flag1,														\
        		   flag2,														\
				   flag3,														\
				   flag4,														\
				   flag5,														\
				   flag6,														\
		           flag7)		(GET_FLAGS4(flag1, flag2, flag3, flag4) |		\
                                 GET_FLAGS3(flag5, flag6, flag7))

#define GET_FLAGS8(flag1,														\
        		   flag2,														\
				   flag3,														\
				   flag4,														\
				   flag5,														\
				   flag6,														\
				   flag7,														\
		           flag8)		(GET_FLAGS4(flag1, flag2, flag3, flag4) |		\
                                 GET_FLAGS4(flag5, flag6, flag7, flag8))

#define GET_FLAGS12(flag1,														\
        		    flag2,														\
				    flag3,														\
				    flag4,														\
				    flag5,														\
				    flag6,														\
				    flag7,														\
				    flag8,														\
				    flag9,														\
				    flag10,														\
				    flag11,														\
		            flag12)		(GET_FLAGS4(flag1, flag2, flag3, flag4) |		\
        						 GET_FLAGS4(flag5, flag6, flag7, flag8) |		\
                                 GET_FLAGS4(flag9, flag10, flag11, flag12))

static const object_desc objects[] =
{
#ifdef OID_BACK
           [OID_BACK] = {LABEL_SIZE(400, 400), 6, 6},
#endif
    [OID_MAIN_SCREEN] = {&MainScreen,   0,    0},
             [OID_H0] = {&H0,           194,  94},
             [OID_H1] = {&H1,           194,  95},
             [OID_H2] = {&H2,           194,  96},
             [OID_H3] = {&H3,           194,  99},
             [OID_H4] = {&H4,           194,  104},
             [OID_H5] = {&H5,           194,  109},
             [OID_H6] = {&H6,           194,  115},
             [OID_H7] = {&H7,           194,  123},
             [OID_H8] = {&H8,           194,  131},
             [OID_H9] = {&H9,           194,  140},
            [OID_H10] = {&H10,          194,  149},
            [OID_H11] = {&H11,          194,  160},
            [OID_H12] = {&H12,          194,  171},
            [OID_H13] = {&H13,          194,  182},
            [OID_H14] = {&H14,          194,  189},
            [OID_H15] = {&H15,          194,  194},
            [OID_H16] = {&H16,          194,  194},
            [OID_H17] = {&H17,          194,  194},
            [OID_H18] = {&H18,          194,  194},
            [OID_H19] = {&H19,          194,  194},
            [OID_H20] = {&H20,          194,  194},
            [OID_H21] = {&H21,          194,  194},
            [OID_H22] = {&H22,          194,  194},
            [OID_H23] = {&H23,          194,  194},
            [OID_H24] = {&H24,          194,  194},
            [OID_H25] = {&H25,          194,  194},
            [OID_H26] = {&H26,          194,  194},
            [OID_H27] = {&H27,          194,  194},
            [OID_H28] = {&H28,          194,  194},
            [OID_H29] = {&H29,          194,  194},
            [OID_H30] = {&H30,          194,  194},
            [OID_H31] = {&H31,          189,  194},
            [OID_H32] = {&H32,          182,  194},
            [OID_H33] = {&H33,          171,  194},
            [OID_H34] = {&H34,          160,  194},
            [OID_H35] = {&H35,          149,  194},
            [OID_H36] = {&H36,          140,  194},
            [OID_H37] = {&H37,          131,  194},
            [OID_H38] = {&H38,          123,  194},
            [OID_H39] = {&H39,          115,  194},
            [OID_H40] = {&H40,          109,  194},
            [OID_H41] = {&H41,          104, 194},
            [OID_H42] = {&H42,          99, 194},
            [OID_H43] = {&H43,          96, 194},
            [OID_H44] = {&H44,          95, 194},
            [OID_H45] = {&H45,          94, 194},
            [OID_H46] = {&H46,          95, 189},
            [OID_H47] = {&H47,          96, 182},
            [OID_H48] = {&H48,          99, 171},
            [OID_H49] = {&H49,          104, 160},
            [OID_H50] = {&H50,          109,  149},
            [OID_H51] = {&H51,          115,  140},
            [OID_H52] = {&H52,          123,  131},
            [OID_H53] = {&H53,          131,  123},
            [OID_H54] = {&H54,          140,  115},
            [OID_H55] = {&H55,          149,  109},
            [OID_H56] = {&H56,          160,  104},
            [OID_H57] = {&H57,          171,  99},
            [OID_H58] = {&H58,          182,  96},
            [OID_H59] = {&H59,          189,  95},
	         [OID_M0] = {&M0,           194,  38},
	         [OID_M1] = {&M1,           194,  39},
	         [OID_M2] = {&M2,           194,  42},
	         [OID_M3] = {&M3,           194,  46},
	         [OID_M4] = {&M4,           194,  52},
         	 [OID_M5] = {&M5,           194,  60},
	         [OID_M6] = {&M6,           194,  70},
         	 [OID_M7] = {&M7,           194,  81},
	         [OID_M8] = {&M8,           194,  93},
	         [OID_M9] = {&M9,           194,  107},
	        [OID_M10] = {&M10,          194,  121},
	        [OID_M11] = {&M11,          194,  137},
	        [OID_M12] = {&M12,          194,  153},
	        [OID_M13] = {&M13,          194,  170},
	        [OID_M14] = {&M14,          194,  185},
	        [OID_M15] = {&M15,          194,  194},
	        [OID_M16] = {&M16,          194,  194},
	        [OID_M17] = {&M17,          194,  194},
	        [OID_M18] = {&M18,          194,  194},
	        [OID_M19] = {&M19,          194,  194},
	        [OID_M20] = {&M20,          194,  194},
	        [OID_M21] = {&M21,          194,  194},
	        [OID_M22] = {&M22,          194,  194},
	        [OID_M23] = {&M23,          194,  194},
	        [OID_M24] = {&M24,          194,  194},
	        [OID_M25] = {&M25,          194,  194},
	        [OID_M26] = {&M26,          194,  194},
	        [OID_M27] = {&M27,          194,  194},
	        [OID_M28] = {&M28,          194,  194},
	        [OID_M29] = {&M29,          194,  194},
	        [OID_M30] = {&M30,          194,  194},
	        [OID_M31] = {&M31,          185,  194},
	        [OID_M32] = {&M32,          170,  194},
	        [OID_M33] = {&M33,          153,  194},
	        [OID_M34] = {&M34,          137,  194},
	        [OID_M35] = {&M35,          121,  194},
	        [OID_M36] = {&M36,          107,  194},
	        [OID_M37] = {&M37,          93, 194},
	        [OID_M38] = {&M38,          81, 194},
	        [OID_M39] = {&M39,          70, 194},
	        [OID_M40] = {&M40,          60, 194},
	        [OID_M41] = {&M41,          52, 194},
	        [OID_M42] = {&M42,          46, 194},
	        [OID_M43] = {&M43,          42, 194},
	        [OID_M44] = {&M44,          39, 194},
	        [OID_M45] = {&M45,          38, 194},
	        [OID_M46] = {&M46,          39, 185},
	        [OID_M47] = {&M47,          42, 170},
	        [OID_M48] = {&M48,          46, 153},
	        [OID_M49] = {&M49,          52, 137},
	        [OID_M50] = {&M50,          60, 121},
	        [OID_M51] = {&M51,          70, 107},
	        [OID_M52] = {&M52,          81, 93},
	        [OID_M53] = {&M53,          93, 81},
	        [OID_M54] = {&M54,          107,  70},
	        [OID_M55] = {&M55,          121,  60},
	        [OID_M56] = {&M56,          137,  52},
	        [OID_M57] = {&M57,          153,  46},
	        [OID_M58] = {&M58,          170,  42},
	        [OID_M59] = {&M59,          185,  39},

       [OID_BIGHEART] = {&BigHeart,     113, 289},
     [OID_SMALLHEART] = {&SmallHeart,   113, 289},
           [OID_HAND] = {&Hand,         263, 289},
          [OID_FLASH] = {&Flash,        269, 95}, // 287 113 centre
	 	  [OID_BLENO] = {&BLENo,        103, 95},
	 	   [OID_BLE0] = {&BLE0,         103, 95},
	 	   [OID_BLE1] = {&BLE1,         103, 95},
	 	   [OID_BLE2] = {&BLE2,         103, 95},
	 	 [OID_BLEMAX] = {&BLEMax,       103, 95},

		     [OID_BP] = {LABEL_SIZE(24, 12), 273, 107},

		    [OID_DAY] = {LABEL_SIZE(20, 18), 282, 198},
		  [OID_MONTH] = {LABEL_SIZE(48, 18), 312, 198},
		  [OID_PULSE] = {LABEL_SIZE(48, 18), 182, 304},
		  [OID_CHARGE_VALUE] = {LABEL_SIZE(100,40), 156, 270},

		   [OID_AMPM] = {&AM, 189, 189},

		  [OID_VALUE] = {LABEL_SIZE(120,50), 146, 182},
	    [OID_BATTERY] = {&Charge, 		140, 81},
   [OID_BATTERYLEVEL] = {LABEL_SIZE(100,40), 156, 193},
   [OID_BIGGESTHEART] = {&BiggestHeart, 106, 46},
            [OID_BPM] = {&bpm,          163, 329},
		   [OID_SPO2] = {&SpO2,         139, 46},
	   [OID_SPO2PERC] = {&SpO2_perc,    166, 330},
	        [OID_GSR] = {&GSR,          106, 61},
         [OID_GSRSEC] = {&GSR_sec,     155, 330},
};

#ifdef OID_BACK
static lv_style_t back_style;
#endif
static lv_style_t label_style;
static lv_style_t pulse_style;
static lv_style_t image_style;
static lv_style_t ampm_style;
static lv_style_t batlevel_style;
static lv_style_t percent_style;
static lv_style_t value_style;
static lv_style_t main_screen_style;
static lv_style_t slave_screen_style;
static lv_style_t bp_style;

static lv_obj_t *main_display;
static lv_obj_t *charge_display;
static lv_obj_t *pulse_display;
static lv_obj_t *SpO2_display;
static lv_obj_t *GSR_Display;

static lv_obj_t *main_screen;
static lv_obj_t *bp;
static lv_obj_t *minute_head;
static lv_obj_t *hour_head;
static lv_obj_t *heart;
static lv_obj_t *ble;
static lv_obj_t *flash;
static lv_obj_t *hand;
static lv_obj_t *dayo;
static lv_obj_t *montho;
static lv_obj_t *pulseo;
static lv_obj_t *ampm;
static lv_obj_t *battery;
static lv_obj_t *battery_level;
static lv_obj_t *value;

static uint32_t mindexes = 0, hindexes = 0;
static uint16_t counter = 0;
static IconState heart_state = IS_OFF, flash_state = IS_OFF;
static IconState minute_head_state = IS_ON, hour_head_state = IS_ON;

static uint8_t current_display = 1;

#ifdef OID_BACK
static lv_obj_t *back;
#endif

static lv_obj_t *_clock_add_image(const lv_obj_t *screen, const object_desc *object, const lv_style_t *image_style)
{
	lv_obj_t *img = lv_img_create(screen);
	if (img)
	{
	  lv_img_set_src(img, object->object);
	  lv_obj_add_style(img, &image_style, 0);
	  lv_obj_set_pos(img, object->x, object->y);
	}
	return img;
}

LV_FONT_DECLARE(arial18b);

static lv_obj_t *_clock_add_label(const lv_obj_t *screen, const object_desc *object, lv_style_t *style)
{
	lv_obj_t *label = lv_label_create(screen);
    if (label)
    {
	  lv_label_set_text(label, " ");
	  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	  lv_obj_add_style(label, style, 0);
	  lv_obj_set_width(label, LABEL_WIDTH(object));
	  lv_obj_set_height(label, LABEL_HEIGHT(object));
	  lv_obj_set_pos(label, object->x -(lv_obj_get_width(label) >> 1),
		  	                object->y -(lv_obj_get_height(label) >> 1));
    }
	return label;
}

static void clock_tick_cb(TimerHandle timer)
{
	(void)timer;

	if (heart_state == IS_FLASH)
	{
		if (counter < 250)
		{
			lv_img_set_src(heart, objects[OID_SMALLHEART].object);
		}
		else
		{
			lv_img_set_src(heart, objects[OID_BIGHEART].object);
		}
	}

	if (flash_state == IS_FLASH)
	{
		if (counter < 500)
		{
			lv_obj_add_flag(flash, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_clear_flag(flash, LV_OBJ_FLAG_HIDDEN);
		}
	}

	if (minute_head_state == IS_FLASH)
	{
		if (counter < 500)
		{
			lv_obj_add_flag(minute_head, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_clear_flag(minute_head, LV_OBJ_FLAG_HIDDEN);
		}
	}

	if (hour_head_state == IS_FLASH)
	{
		if (counter < 500)
		{
			lv_obj_add_flag(hour_head, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_clear_flag(hour_head, LV_OBJ_FLAG_HIDDEN);
		}
	}

	counter += CLOCK_STEP;
	while (counter >= 1000)
		counter -= 1000;
}

void Clock_Init(void)
{
	int i;

	main_display = lv_obj_create(NULL);

	lv_style_init(&main_screen_style);
	lv_style_set_bg_color(&main_screen_style, LV_COLOR_WHITE);
	lv_obj_add_style(main_display, &main_screen_style, LV_PART_MAIN);

	lv_style_init(&image_style);
	lv_style_set_bg_color(&image_style, LV_COLOR_APP_MAIN);

	lv_style_init(&label_style);
	lv_style_set_text_color(&label_style, LV_COLOR_APP_MAIN);
	lv_style_set_text_font(&label_style, &arial18b);

	lv_style_init(&pulse_style);
	lv_style_set_text_color(&pulse_style, LV_COLOR_APP_MAIN);
	lv_style_set_text_font(&pulse_style, &arial18b);

	lv_style_init(&percent_style);
	lv_style_set_text_color(&percent_style, LV_COLOR_GREEN);
	lv_style_set_text_font(&percent_style,  &Bahnschrift_40);

	lv_style_init(&value_style);
	lv_style_set_text_color(&value_style, LV_COLOR_GREEN);
	lv_style_set_text_font(&value_style,  &Bahnschrift_40);

	lv_style_init(&ampm_style);
	lv_style_set_text_color(&ampm_style, LV_COLOR_APP_MAIN);
	lv_style_set_bg_color(&ampm_style, LV_COLOR_WHITE);
	lv_style_set_bg_grad_color(&ampm_style, LV_COLOR_WHITE);
	lv_style_set_opa(&ampm_style, LV_OPA_COVER);
	lv_style_set_text_font(&ampm_style, &arial18b);
	lv_style_set_arc_rounded(&ampm_style, true);
	lv_style_set_radius(&ampm_style, (LABEL_WIDTH(&objects[OID_AMPM]) + LABEL_HEIGHT(&objects[OID_AMPM])) >> 2);
	lv_style_set_border_width(&ampm_style, 4);
	lv_style_set_border_color(&ampm_style, LV_COLOR_APP_MAIN);

	lv_style_init(&bp_style);
	lv_style_set_bg_color(&bp_style, LV_COLOR_WHITE);
	lv_style_set_radius(&bp_style, 0);
	lv_style_set_arc_rounded(&bp_style, false);
	lv_style_set_border_width(&bp_style, 0);

	main_screen = _clock_add_image(main_display, &objects[OID_MAIN_SCREEN], &image_style);

	bp = lv_obj_create(main_screen);
	lv_obj_set_pos(bp, objects[OID_BP].x, objects[OID_BP].y);
	lv_obj_set_size(bp, 1, LABEL_HEIGHT(&objects[OID_BP]));
	lv_obj_add_style(bp, &bp_style, LV_PART_MAIN);


	heart = _clock_add_image(main_display, &objects[OID_BIGHEART], &image_style);
	ble = _clock_add_image(main_display, &objects[OID_BLENO], &image_style);
	flash = _clock_add_image(main_display, &objects[OID_FLASH], &image_style);
	hand = _clock_add_image(main_display, &objects[OID_HAND], &image_style);

	dayo = _clock_add_label(main_display, &objects[OID_DAY], &pulse_style);
	montho = _clock_add_label(main_display, &objects[OID_MONTH], &label_style);
	pulseo = _clock_add_label(main_display, &objects[OID_PULSE], &pulse_style);

	Clock_ShowFlash(IS_OFF);
	Clock_ShowHand(IS_OFF);
	Clock_ShowHeart(IS_OFF);
	Clock_ShowLevel(0);
	Clock_SetPulse(0, false);
	Clock_SetDate(1, 1, 0);
	Clock_SetTime(0, 0);

	hour_head = _clock_add_image(main_display, &objects[OID_H0], &image_style);
	lv_obj_set_style_transform_pivot_x(hour_head, 12, 0);
	lv_obj_set_style_transform_pivot_y(hour_head, 112, 0);

	minute_head = _clock_add_image(main_display, &objects[OID_M0], &image_style);
	lv_obj_set_style_transform_pivot_x(minute_head, 12, 0);
	lv_obj_set_style_transform_pivot_y(minute_head, 168, 0);

	ampm = _clock_add_image(main_display, &objects[OID_AMPM], &image_style);

	charge_display = lv_obj_create(NULL);

	lv_style_init(&slave_screen_style);
	lv_style_set_bg_color(&slave_screen_style, lv_color_hex(0x000000));
	lv_style_set_bg_opa(&slave_screen_style, LV_OPA_COVER);
	lv_obj_add_style(charge_display, &slave_screen_style, LV_PART_MAIN);

	//lv_obj_set_style_bg_color(charge_display, LV_COLOR_BLACK, LV_PART_MAIN);
	//lv_obj_set_style_bg_opa(charge_display, LV_OPA_COVER, LV_PART_MAIN);

	battery = _clock_add_image(charge_display, &objects[OID_BATTERY], &slave_screen_style);
	lv_style_init(&batlevel_style);
	lv_style_set_text_color(&batlevel_style, LV_COLOR_GREEN);
	lv_style_set_text_font(&batlevel_style,  &Bahnschrift_40);

	battery_level = _clock_add_label(charge_display, &objects[OID_BATTERYLEVEL], &batlevel_style);
	//lv_obj_set_style_text_color(&battery_level, LV_COLOR_RED, 0);
	lv_label_set_text(battery_level, "...");

	lv_scr_load(charge_display);
	lv_scr_load(main_display);

	Clock_ChangeState(CS_UNKNOWN);

	SoftTimers_Create(CLOCK_STEP, true, clock_tick_cb);

	Message_AddProcessor(ClockMessage, MESSAGES(MESSAGE_CLOCK_SET_TIME,
												MESSAGE_CLOCK_SET_DATE,
												MESSAGE_CLOCK_SET_PULSE,
												MESSAGE_CLOCK_SHOW_HEART,
												MESSAGE_CLOCK_SHOW_HAND,
												MESSAGE_CLOCK_SHOW_FLASH,
												MESSAGE_CLOCK_SHOW_LEVEL,
												MESSAGE_CLOCK_SHOW_MHEAD,
												MESSAGE_CLOCK_SHOW_HHEAD,
												MESSAGE_CLOCK_SET_STATE,
												MESSAGE_CLOCK_SET_BATTERY_VOLUME,
												MESSAGE_CLOCK_CHANGE_SCREEN,
												MESSAGE_CLOCK_REFRESH,
												MESSAGE_PARAM_SET_R,
												MESSAGE_PARAM_SET_ADC,
												MESSAGE_PARAM_SET_PULSE,
												MESSAGE_PARAM_SET_BAT_VOLUME,
												MESSAGE_PARAM_SET_RSSI,
												MESSAGE_PARAM_SET_TEMPERATURE,
												MESSAGE_PARAM_SET_SPO2,
												MESSAGE_PARAM_SET_GSR_INTEVAL,
												MESSAGE_PARAM_SET_BLE_ACTIVE,
												MESSAGE_PARAM_SET_BLE_CONNECTED,
												MESSAGE_PARAM_SET_HAND_ON,
												MESSAGE_PARAM_SET_DATE,
												MESSAGE_PARAM_SET_TIME));

	//Message_Add(MESSAGE_CLOCK_CHANGE_SCREEN, 1, 0, 1);
}

static void _clock_refresh_label(lv_obj_t *label, const char *text, object_desc *object)
{
	lv_label_set_text(label, text);
}

static bool _isLeap(uint16_t year)
{
	if ((year % 400) == 0)
		return true;
	if ((year % 100) == 0)
		return false;
	if ((year & 3) == 0)
		return true;
	return false;
}

static bool _valid_date(uint8_t day, uint8_t month, uint16_t year)
{
	if ((day == 0) || (month == 0) || (month > 12))
		return false;
	if (month == 2)
	{
		if (_isLeap(year))
		{
			if (day > 29)
				return false;
		}
		else
		{
			if (day > 28)
				return false;
		}
	}
	else
	{
  	  if ((month == 1) || (month == 3)  || (month == 5) || (month == 7) ||
	      (month == 8) || (month == 10) || (month == 12))
	  {
		  if (day > 31)
			  return false;
	  }
	  else
	  {
		  if (day > 30)
			  return false;
	  }
	}
	return true;
}

bool Clock_ValidTime(uint8_t minute, uint8_t hour)
{
	return (hour < 24) && (minute < 60);
}

bool Clock_SetTime(uint8_t minute, uint8_t hour)
{
  if (Clock_ValidTime(minute, hour))
  {
/*
   	  lv_img_set_angle(minute_head, (int16_t)((uint32_t)60 * 3600 / minute));
	  lv_img_set_angle(hour_head, (int16_t)((uint32_t)12 * 3600 / (hour % 12)));
*/
    object_desc *head = &objects[minute + OID_M0];
    if (lv_img_get_src(minute_head) != head->object)
    {
      int i;
      lv_img_set_src(minute_head, head->object);
	  lv_obj_set_pos(minute_head, head->x, head->y);
	  //lv_obj_invalidate(main_screen);
    }
    head = &objects[((hour % 12) * 5) + (minute / 12) + OID_H0];
    if (lv_img_get_src(hour_head) != head->object)
    {
      lv_img_set_src(hour_head, head->object);
  	  lv_obj_set_pos(hour_head, head->x, head->y);
	  //lv_obj_invalidate(main_screen);
    }
    	if (hour < 12)
    		lv_img_set_src(ampm, &AM);
    	else
    		lv_img_set_src(ampm, &PM);

		return true;
  }
  return false;
}

static void btos(uint8_t byte, char *buf)
{
	char i = (byte > 9) ? (byte > 99) ? 3 : 2 : 1;

	buf[i] = 0;

	while (i--)
	{
		buf[i] = (byte % 10) + '0';
		byte /= 10;
	}
}

bool Clock_ValidDate(uint8_t day, uint8_t month, uint16_t year)
{
	return _valid_date(day, month, year);
}

bool Clock_SetDate(uint8_t day, uint8_t month, uint16_t year)
{
	if (_valid_date(day, month, year))
	{
	  static const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEC"};
	  if ((day >= 1) && (day <= 31) && (month >= 1) && (month <= 12))
	  {
	    char sday[3];

	    btos(day, sday);

	    _clock_refresh_label(dayo,  sday, &objects[OID_DAY]);
	    _clock_refresh_label(montho,  months[month - 1], &objects[OID_MONTH]);
	  }
	  return true;
	}
	return false;
}

void Clock_SetPulse(uint8_t pulse, bool alarm)
{
	char buf[4] = "...";
	lv_color_t color = alarm ? LV_COLOR_RED : LV_COLOR_GREEN;

	if (pulse != 0)
		btos(pulse, buf);

	lv_obj_set_style_text_color(pulseo, color, 0);
	_clock_refresh_label(pulseo, buf, &objects[OID_PULSE]);
}

static void _clock_reset_state(lv_obj_t *icon, IconState *state, IconState new_state)
{
	  switch(new_state)
	  {
	    case IS_OFF:
	    {
	    	lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);

	    	break;
	    }
	    case IS_FLASH:
	    case IS_ON:
	    {
	    	lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
	    	break;
	    }
	    default:
	    {
	    	return;
	    }
	  }
	  if (state)
	  	*state = new_state;
}

void Clock_ShowHeart(IconState state)
{
	_clock_reset_state(heart, &heart_state, state);
	if (state == IS_ON)
		lv_img_set_src(heart, objects[OID_BIGHEART].object);
}

void Clock_ShowHand(IconState state)
{
	_clock_reset_state(hand, NULL, state);
}

void Clock_ShowFlash(IconState state)
{
	_clock_reset_state(flash, &flash_state, state);
}

void Clock_ShowLevel(int8_t percent_level)
{
	if (percent_level >= 0)
	{
	  _clock_reset_state(ble, NULL, IS_ON);
	  if (percent_level == 0)
		  lv_img_set_src(ble, objects[OID_BLENO].object);
	  else if (percent_level < 25)
	      lv_img_set_src(ble, objects[OID_BLE0].object);
	  else if (percent_level < 50)
	      lv_img_set_src(ble, objects[OID_BLE1].object);
	  else if (percent_level < 75)
	      lv_img_set_src(ble, objects[OID_BLE2].object);
	  else
		  lv_img_set_src(ble, objects[OID_BLEMAX].object);
	}
	else
		_clock_reset_state(ble, NULL, IS_OFF);
}

void Clock_ChangeState(ClockState state)
{
	//(void)state;
	lv_color_t color;
	switch (state)
	{
	case CS_UNKNOWN:
	{
		color = LV_COLOR_WHITE;
		break;
	}
	case CS_GOOD:
	{
		color = LV_COLOR_LIME;
		break;
	}
	case CS_WARNING:
	{
		color = LV_COLOR_YELLOW;
		break;
	}
	case CS_ALARM:
	{
		color = LV_COLOR_RED;
		break;
	}
	default:
	{
		return;
	}
	}
	lv_obj_set_style_bg_color(main_display, color, LV_PART_MAIN);
}

void Clock_ShowMinuteHead(IconState state)
{
	_clock_reset_state(minute_head, &minute_head_state, state);
}

void Clock_ShowHourHead(IconState state)
{
	_clock_reset_state(hour_head, &hour_head_state, state);
}

static lv_color_t color_mix(lv_color_t c1, lv_color_t c2, uint8_t pct)
{
    if(pct > 100) pct = 100;

    uint8_t r = ((uint32_t)LV_COLOR_GET_R(c1)   * (100 - pct) +
                 (uint32_t)LV_COLOR_GET_R(c2)   * pct) / 100;

    uint8_t g = ((uint32_t)LV_COLOR_GET_G(c1) * (100 - pct) +
                 (uint32_t)LV_COLOR_GET_G(c2) * pct) / 100;

    uint8_t b = ((uint32_t)LV_COLOR_GET_B(c1)  * (100 - pct) +
                 (uint32_t)LV_COLOR_GET_B(c2)  * pct) / 100;

    return lv_color_make(r, g, b);
}

static lv_color_t get_mix_color(uint8_t mix)
{
	if (mix > 100)
		mix = 100;
	uint16_t r = (mix < 50) ? 255 : (mix > 100) ? 0 : 255 * (100 - mix) / 50,
			 g = (mix > 50) ? 180 : 180 * mix / 50;
	return lv_color_make(r, g, 0);
}

static char bat_vol_text[10];
static bool display_changing = true;

static bool ClockMessage(Message message)
{
	switch (message->ID)
	{
		case MESSAGE_CLOCK_SET_TIME:
		case MESSAGE_PARAM_SET_TIME:
		{
			Clock_SetTime(message->cpar16[0], message->cpar16[1]);
			break;
		}
		case MESSAGE_PARAM_SET_DATE:
		case MESSAGE_CLOCK_SET_DATE:
		{
			Clock_SetDate(message->cpar16[0], message->cpar16[1], message->upar32);
			break;
		}
		case MESSAGE_CLOCK_SET_PULSE:
		{
			//Clock_SetPulse(message->upar16, message->upar8 != 0);
			break;
		}
		case MESSAGE_CLOCK_SHOW_HEART:
		{
			//Clock_ShowHeart((IconState)message->upar8);
			break;
		}
		case MESSAGE_CLOCK_SHOW_HAND:
		{
			//Clock_ShowHand((IconState)message->upar8);
			break;
		}
		case MESSAGE_CLOCK_SHOW_FLASH:
		{
			//Clock_ShowFlash((IconState)message->upar8);
			break;
		}
		case MESSAGE_CLOCK_SHOW_LEVEL:
		{
			//Clock_ShowLevel(message->ipar8);
			break;
		}
		case MESSAGE_CLOCK_SHOW_MHEAD:
		{
			Clock_ShowMinuteHead((IconState)message->upar8);
			break;
		}
		case MESSAGE_CLOCK_SHOW_HHEAD:
		{
			Clock_ShowHourHead((IconState)message->upar8);
			break;
		}
		case MESSAGE_CLOCK_SET_STATE:
		{
			Clock_ChangeState((ClockState)message->upar8);
			break;
		}
		case MESSAGE_CLOCK_SET_BATTERY_VOLUME:
		{
			uint16_t perc = message->upar8;
			lv_color_t color = get_mix_color(perc);
			snprintf(bat_vol_text, sizeof(bat_vol_text), "%d%%", message->upar8);
			lv_style_set_text_color(&batlevel_style, color);
			lv_label_set_text_static(battery_level, bat_vol_text);
			lv_style_set_bg_color(&bp_style, color);
			perc = LABEL_WIDTH(&objects[OID_BP]) * perc / 100;
			if (perc == 0)
				perc = 1;
			lv_obj_set_width(bp, perc);
			break;
		}
		case MESSAGE_CLOCK_CHANGE_SCREEN:
		{
			uint8_t dspl = message->upar8;
			if ((dspl == current_display) && (message->upar16 == 0))
			{
				tlk_printf("Dspl %d. No change\n", dspl);
				break;
			}
			display_changing = true;
			lv_setlight(0);
			switch (dspl)
			{
				case 0:
				{
					lv_scr_load(charge_display);
					lv_obj_invalidate(charge_display);
					lv_obj_invalidate(battery);
					Message_Add(MESSAGE_PARAM_GET_BAT_VOLUME, 0, 0, 0);
					break;
				}
				case 1:
				{
					lv_scr_load(main_display);
					lv_obj_invalidate(main_display);
					Message_Add(MESSAGE_CLOCK_SHOW_MHEAD, IS_ON, 0, 0);
					Message_Add(MESSAGE_CLOCK_SHOW_HHEAD, IS_ON, 0, 0);
					Clock_ShowMinuteHead((IconState)message->upar8);
					Message_Add(MESSAGE_PARAM_GET_BLE_ACTIVE, 0, 0, 0);
					Message_Add(MESSAGE_PARAM_GET_BLE_CONNECTED, 0, 0, 0);
					Message_Add(MESSAGE_PARAM_GET_PULSE, 0, 0, 0);
					Message_Add(MESSAGE_PARAM_GET_BAT_VOLUME, 0, 0, 0);
					Message_Add(MESSAGE_PARAM_GET_DATE, 0, 0, 0);
					Message_Add(MESSAGE_PARAM_GET_TIME, 0, 0, 0);
					break;
				}
				default:
				{
					break;
				}
			}
			tlk_printf("Change display %d => %d\n", current_display, dspl);
			current_display = dspl;
			break;
		}
		case MESSAGE_CLOCK_REFRESH:
		{
			if (display_changing)
			{
				lv_setlight(1);
				display_changing = false;
			}
			break;
		}
		case MESSAGE_PARAM_SET_R:
		{
			break;
		}
		case MESSAGE_PARAM_SET_ADC:
		{
			break;
		}
		case MESSAGE_PARAM_SET_PULSE:
		{
			if (message->upar32 < 20)
			{
				Clock_ShowHeart(IS_OFF);
				Clock_SetPulse(0, 0);
			}
			else
			{
				Clock_ShowHeart(IS_FLASH);
				Clock_SetPulse(message->upar32, 0);
			}
			break;
		}
		case MESSAGE_PARAM_SET_BAT_VOLUME:
		{
			uint16_t perc = (message->upar32 >= 100) ? 100 : message->upar32;
			lv_color_t color = get_mix_color(perc);
			snprintf(bat_vol_text, sizeof(bat_vol_text), "%d%%", perc);
			lv_style_set_text_color(&batlevel_style, color);
			lv_label_set_text_static(battery_level, bat_vol_text);
			lv_style_set_bg_color(&bp_style, color);
			perc = LABEL_WIDTH(&objects[OID_BP]) * perc / 100;
			if (perc == 0)
				perc = 1;
			lv_obj_set_width(bp, perc);
			break;
		}
		case MESSAGE_PARAM_SET_RSSI:
		{
			break;
		}
		case MESSAGE_PARAM_SET_TEMPERATURE:
		{
			break;
		}
		case MESSAGE_PARAM_SET_SPO2:
		{
			break;
		}
		case MESSAGE_PARAM_SET_GSR_INTEVAL:
		{
			break;
		}
		case MESSAGE_PARAM_SET_BLE_ACTIVE:
		{
			Clock_ShowLevel(message->upar32 != 0 ? 0 : -1);
			break;
		}
		case MESSAGE_PARAM_SET_BLE_CONNECTED:
		{
			Clock_ShowLevel(message->upar32 != 0 ? 100 : 0);
			break;
		}
		case MESSAGE_PARAM_SET_HAND_ON:
		{
			Clock_ShowHand(message->upar32 != 0 ? IS_ON : IS_OFF);
			break;
		}
		default:
		{
			break;
		}
	}
	return false;
}

uint8_t Clock_DisplayCount(void)
{
	return 2;
}
