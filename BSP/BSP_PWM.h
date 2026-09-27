#ifndef BSP_PWM_H
#define BSP_PWM_H

#include "tl_common.h"

typedef enum
{
    BSP_PWM_PC7 = 0,    // PWM0
    BSP_PWM_PB1 = 1     // PWM1
} bsp_pwm_channel_e;

/*
 * Инициализация:
 *   PWM0 -> PC7
 *   PWM1 -> PB1
 *
 * После инициализации оба канала запущены.
 */
void BSP_PWMInit(void);

/*
 * Частота в mHz:
 *
 *   60000   = 60.000 Hz
 *   100050  = 100.050 Hz
 *   1000000 = 1000.000 Hz
 *
 * Для PB1 допустимый диапазон: 60...4000 Hz.
 * Для PC7 частота должна быть > 0.
 */
void BSP_PWMFrequencySet(
    bsp_pwm_channel_e channel,
    uint32_t frequency_mhz
);

/*
 * Коэффициент заполнения в процентах:
 *   0...100
 */
void BSP_PWMDutyCycleSet(
    bsp_pwm_channel_e channel,
    uint16_t duty_percent
);

/*
 * Остановить только указанный PWM-канал.
 * Сохранённые частота и duty не изменяются.
 */
void BSP_PWMStop(
    bsp_pwm_channel_e channel
);

/*
 * Остановить оба PWM и отключить PWM-функцию
 * на PC7/PB1.
 *
 * Сохранённые параметры каналов уничтожаются.
 */
void BSP_PWMSleep(void);

void BSP_PWMWakeup(void);
#endif /* BSP_PWM_H */
