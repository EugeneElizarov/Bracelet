#include "BSP_PWM.h"
#include "pwm.h"


/*
 * PWM0 -> PC7
 * PWM1 -> PB1
 */

#define BSP_PWM_PC7_ID          PWM0_ID
#define BSP_PWM_PB1_ID          PWM1_ID

#define BSP_PWM_PB1_MIN_MHZ     60000UL
#define BSP_PWM_PB1_MAX_MHZ     4000000UL

/*
 * Требуемая относительная погрешность:
 *
 *     0.005% = 5 / 100000
 *
 * Проверяем без floating point:
 *
 *     abs(Factual - Ftarget) / Ftarget <= 5 / 100000
 */
#define BSP_PWM_FREQ_ERROR_NUM  5ULL
#define BSP_PWM_FREQ_ERROR_DEN  100000ULL


/*
 * Начальные параметры.
 *
 * Их можно изменить здесь, если нужны другие значения
 * после BSP_PWMInit().
 */
#define BSP_PWM_DEFAULT_FREQ_MHZ    1000000UL   /* 1000 Hz */
#define BSP_PWM_DEFAULT_DUTY        50          /* 50% */


/* -------------------------------------------------------------------------- */
/*                         Внутреннее состояние                               */
/* -------------------------------------------------------------------------- */

typedef struct
{
    uint32_t frequency_mhz;
    uint16_t duty_percent;

    uint16_t tmax;
    uint16_t tcmp;

    uint8_t running;
} bsp_pwm_channel_state_t;


static bsp_pwm_channel_state_t s_pwm_channel[2] =
{
    {
        .frequency_mhz = BSP_PWM_DEFAULT_FREQ_MHZ,
        .duty_percent  = BSP_PWM_DEFAULT_DUTY,
        .tmax          = 0,
        .tcmp          = 0,
        .running       = 0
    },

    {
        .frequency_mhz = BSP_PWM_DEFAULT_FREQ_MHZ,
        .duty_percent  = BSP_PWM_DEFAULT_DUTY,
        .tmax          = 0,
        .tcmp          = 0,
        .running       = 0
    }
};


static uint8_t s_pwm_initialized = 0;
static uint8_t s_pwm_clkdiv = 0;


/* -------------------------------------------------------------------------- */
/*                         Вспомогательные функции                            */
/* -------------------------------------------------------------------------- */

static uint32_t BSP_PWMClockGet(uint8_t clkdiv)
{
    return sys_clk.pclk / ((uint32_t)clkdiv + 1U);
}


/*
 * Найти ближайший TMAX для заданной частоты.
 *
 * PWM frequency:
 *
 *     Fpwm = PCLK / ((CLKDIV + 1) * TMAX)
 *
 * frequency_mhz хранится в миллигерцах.
 */
static uint16_t BSP_PWMTmaxCalculate(
    uint32_t frequency_mhz,
    uint8_t clkdiv
)
{
    uint64_t denominator;
    uint64_t tmax;

    if (frequency_mhz == 0)
    {
        return 1;
    }

    /*
     * TMAX = round(PCLK * 1000 /
     *              ((CLKDIV + 1) * frequency_mhz))
     */
    denominator = (uint64_t)(clkdiv + 1U) *
                  (uint64_t)frequency_mhz;

    tmax = ((uint64_t)sys_clk.pclk * 1000ULL +
            denominator / 2ULL) / denominator;

    if (tmax < 1ULL)
    {
        tmax = 1ULL;
    }

    if (tmax > 65535ULL)
    {
        tmax = 65535ULL;
    }

    return (uint16_t)tmax;
}


/*
 * Проверка точности PB1.
 *
 * Используется точная рациональная форма:
 *
 *     Factual = PCLK * 1000 /
 *               ((CLKDIV + 1) * TMAX)
 *
 * Условие:
 *
 *     |Factual - Ftarget| / Ftarget <= 0.005%
 */
static uint8_t BSP_PWMFrequencyAccurate(
    uint32_t frequency_mhz,
    uint8_t clkdiv,
    uint16_t tmax
)
{
    uint64_t numerator;
    uint64_t denominator;
    uint64_t difference;

    if ((frequency_mhz == 0) || (tmax == 0))
    {
        return 0;
    }

    numerator = (uint64_t)sys_clk.pclk * 1000ULL;

    denominator = (uint64_t)(clkdiv + 1U) *
                  (uint64_t)tmax;

    /*
     * Сравниваем:
     *
     *     difference / denominator / frequency
     *
     * с 5 / 100000.
     *
     * То есть:
     *
     *     difference * 100000 <=
     *     frequency * denominator * 5
     */
    if (numerator >=
        (uint64_t)frequency_mhz * denominator)
    {
        difference = numerator -
                     (uint64_t)frequency_mhz * denominator;
    }
    else
    {
        difference = (uint64_t)frequency_mhz * denominator -
                     numerator;
    }

    return (difference * BSP_PWM_FREQ_ERROR_DEN <=
            (uint64_t)frequency_mhz *
            denominator *
            BSP_PWM_FREQ_ERROR_NUM);
}


/*
 * Проверка возможности представить частоту PB1
 * при конкретном CLKDIV.
 */
static uint8_t BSP_PWMClkdivValidForPB1(
    uint32_t frequency_mhz,
    uint8_t clkdiv
)
{
    uint16_t tmax;

    tmax = BSP_PWMTmaxCalculate(frequency_mhz, clkdiv);

    /*
     * Если требуемая частота настолько высокая,
     * что даже TMAX=1 недостаточен.
     */
    if (tmax == 0)
    {
        return 0;
    }

    return BSP_PWMFrequencyAccurate(
        frequency_mhz,
        clkdiv,
        tmax
    );
}


/*
 * Выбор нового CLKDIV для PB1.
 *
 * ВАЖНО:
 *
 * Сначала вызывающий код проверяет текущий CLKDIV.
 * Поэтому сюда мы попадаем только тогда, когда
 * текущий делитель уже не обеспечивает требуемую
 * точность.
 *
 * Из подходящих вариантов выбираем делитель,
 * находящийся ближе всего к текущему.
 *
 * Это уменьшает количество изменений CLKDIV
 * при работе.
 */
static uint8_t BSP_PWMClkdivFindForPB1(
    uint32_t frequency_mhz,
    uint8_t current_clkdiv
)
{
    uint16_t best_distance = 0xFFFF;
    uint8_t best_clkdiv = current_clkdiv;
    uint16_t distance;
    uint16_t div;

    for (div = 0; div <= 255U; ++div)
    {
        if (!BSP_PWMClkdivValidForPB1(
                frequency_mhz,
                (uint8_t)div))
        {
            continue;
        }

        if (div >= current_clkdiv)
        {
            distance = div - current_clkdiv;
        }
        else
        {
            distance = current_clkdiv - div;
        }

        if (distance < best_distance)
        {
            best_distance = distance;
            best_clkdiv = (uint8_t)div;
        }
    }

    return best_clkdiv;
}


/*
 * Расчёт TCMP по TMAX и duty.
 */
static uint16_t BSP_PWMTcmpCalculate(
    uint16_t tmax,
    uint16_t duty_percent
)
{
    uint64_t tcmp;

    if (duty_percent >= 100U)
    {
        return tmax;
    }

    if (duty_percent == 0U)
    {
        return 0;
    }

    tcmp = ((uint64_t)tmax *
            (uint64_t)duty_percent +
            50ULL) / 100ULL;

    if (tcmp > tmax)
    {
        tcmp = tmax;
    }

    return (uint16_t)tcmp;
}


/*
 * Записать рассчитанные параметры одного канала
 * в аппаратные регистры.
 */
static void BSP_PWMChannelApply(
    bsp_pwm_channel_e channel
)
{
    bsp_pwm_channel_state_t *state;
    pwm_id_e pwm_id;

    state = &s_pwm_channel[(uint8_t)channel];

    if (channel == BSP_PWM_PC7)
    {
        pwm_id = PWM0_ID;
    }
    else
    {
        pwm_id = PWM1_ID;
    }

    state->tmax = BSP_PWMTmaxCalculate(
        state->frequency_mhz,
        s_pwm_clkdiv
    );

    state->tcmp = BSP_PWMTCmpCalculate(
        state->tmax,
        state->duty_percent
    );

    pwm_set_tmax(pwm_id, state->tmax);
    pwm_set_tcmp(pwm_id, state->tcmp);
}


/*
 * Пересчитать оба канала после изменения CLKDIV.
 */
static void BSP_PWMAllChannelsApply(void)
{
    BSP_PWMChannelApply(BSP_PWM_PC7);
    BSP_PWMChannelApply(BSP_PWM_PB1);
}


/*
 * Включение одного канала без использования pwm_en_e,
 * определение которого в предоставленном pwm.h мы
 * здесь не используем.
 */
static void BSP_PWMChannelStart(
    bsp_pwm_channel_e channel
)
{
    if (channel == BSP_PWM_PC7)
    {
        reg_pwm_enable |= BIT(PWM0_ID);
    }
    else
    {
        reg_pwm_enable |= BIT(PWM1_ID);
    }

    s_pwm_channel[(uint8_t)channel].running = 1;
}


/*
 * Остановка одного канала.
 */
static void BSP_PWMChannelStop(
    bsp_pwm_channel_e channel
)
{
    if (channel == BSP_PWM_PC7)
    {
        reg_pwm_enable &= ~BIT(PWM0_ID);
    }
    else
    {
        reg_pwm_enable &= ~BIT(PWM1_ID);
    }

    s_pwm_channel[(uint8_t)channel].running = 0;
}


/*
 * Полная остановка обоих PWM.
 */
static void BSP_PWMAllStop(void)
{
    reg_pwm_enable &= ~(BIT(PWM0_ID) | BIT(PWM1_ID));

    s_pwm_channel[BSP_PWM_PC7].running = 0;
    s_pwm_channel[BSP_PWM_PB1].running = 0;
}


/* -------------------------------------------------------------------------- */
/*                              Public API                                    */
/* -------------------------------------------------------------------------- */

void BSP_PWMInit(void)
{
    /*
     * Настройка выводов:
     *
     * PWM0 -> PC7
     * PWM1 -> PB1
     */
    pwm_set_pin(GPIO_FC_PC7, PWM0);
    pwm_set_pin(GPIO_FC_PB1, PWM1);

    /*
     * Нормальный режим PWM.
     *
     * Для PWM0 API явно предоставлен SDK.
     * PWM1 использует штатный режим после reset/init.
     */
    pwm_set_pwm0_mode(PWM_NORMAL_MODE);

    /*
     * Начинаем поиск CLKDIV с 0.
     *
     * Если 0 не обеспечивает требуемую точность
     * PB1, будет выбран подходящий автоматически.
     */
    s_pwm_clkdiv = 0;

    if (!BSP_PWMClkdivValidForPB1(
            s_pwm_channel[BSP_PWM_PB1].frequency_mhz,
            s_pwm_clkdiv))
    {
        s_pwm_clkdiv = BSP_PWMClkdivFindForPB1(
            s_pwm_channel[BSP_PWM_PB1].frequency_mhz,
            s_pwm_clkdiv
        );
    }

    /*
     * Общий делитель PWM.
     */
    pwm_set_clk(s_pwm_clkdiv);

    /*
     * Рассчитать параметры обоих каналов
     * относительно одного общего CLKDIV.
     */
    BSP_PWMAllChannelsApply();

    /*
     * Запустить оба канала.
     */
    BSP_PWMChannelStart(BSP_PWM_PC7);
    BSP_PWMChannelStart(BSP_PWM_PB1);

    s_pwm_initialized = 1;
}


void BSP_PWMFrequencySet(
    bsp_pwm_channel_e channel,
    uint32_t frequency_mhz
)
{
    uint8_t old_clkdiv;
    uint8_t new_clkdiv;

    if ((uint8_t)channel > BSP_PWM_PB1)
    {
        return;
    }

    if (frequency_mhz == 0)
    {
        return;
    }

    /*
     * Для PB1 проверяем установленный диапазон.
     */
    if (channel == BSP_PWM_PB1)
    {
        if ((frequency_mhz < BSP_PWM_PB1_MIN_MHZ) ||
            (frequency_mhz > BSP_PWM_PB1_MAX_MHZ))
        {
            return;
        }

        /*
         * Сначала сохраняем новую требуемую частоту.
         */
        s_pwm_channel[BSP_PWM_PB1].frequency_mhz =
            frequency_mhz;

        old_clkdiv = s_pwm_clkdiv;

        /*
         * Критически важная часть алгоритма:
         *
         * ТЕКУЩИЙ CLKDIV всегда проверяется первым.
         *
         * Если он подходит, CLKDIV не изменяется.
         */
        if (BSP_PWMClkdivValidForPB1(
                frequency_mhz,
                s_pwm_clkdiv))
        {
            /*
             * CLKDIV остаётся прежним.
             *
             * Но параметры ОБОИХ PWM пересчитываем.
             */
            BSP_PWMAllChannelsApply();
            return;
        }

        /*
         * Текущий CLKDIV не обеспечивает ±0.005%.
         *
         * Теперь разрешается его изменить.
         */
        new_clkdiv = BSP_PWMClkdivFindForPB1(
            frequency_mhz,
            old_clkdiv
        );

        /*
         * Если подходящего CLKDIV нет,
         * возвращаем старую частоту PB1.
         */
        if (!BSP_PWMClkdivValidForPB1(
                frequency_mhz,
                new_clkdiv))
        {
            return;
        }

        /*
         * Останавливаем оба канала на время изменения
         * общего делителя.
         */
        BSP_PWMAllStop();

        s_pwm_clkdiv = new_clkdiv;

        pwm_set_clk(s_pwm_clkdiv);

        /*
         * Очень важно:
         *
         * После изменения общего CLKDIV пересчитываем
         * И PB1, И PC7.
         *
         * Для PC7 используется сохранённая частота,
         * поэтому его частота после смены CLKDIV
         * будет максимально близкой к заданной.
         */
        BSP_PWMAllChannelsApply();

        /*
         * Восстанавливаем состояние каналов,
         * которое было до изменения CLKDIV.
         */
        BSP_PWMChannelStart(BSP_PWM_PC7);
        BSP_PWMChannelStart(BSP_PWM_PB1);

        return;
    }


    /*
     * ------------------------------------------------------------------
     * PC7
     * ------------------------------------------------------------------
     *
     * PC7 НИКОГДА не изменяет CLKDIV.
     */
    s_pwm_channel[BSP_PWM_PC7].frequency_mhz =
        frequency_mhz;

    /*
     * Просто рассчитываем ближайший TMAX
     * при уже установленном CLKDIV.
     */
    BSP_PWMChannelApply(BSP_PWM_PC7);
}


void BSP_PWMDutyCycleSet(
    bsp_pwm_channel_e channel,
    uint16_t duty_percent
)
{
    if ((uint8_t)channel > BSP_PWM_PB1)
    {
        return;
    }

    if (duty_percent > 100U)
    {
        duty_percent = 100U;
    }

    s_pwm_channel[(uint8_t)channel].duty_percent =
        duty_percent;

    /*
     * Частота и CLKDIV здесь вообще не меняются.
     */
    BSP_PWMChannelApply(channel);
}


void BSP_PWMStop(
    bsp_pwm_channel_e channel
)
{
    if ((uint8_t)channel > BSP_PWM_PB1)
    {
        return;
    }

    BSP_PWMChannelStop(channel);
}


void BSP_PWMSleep(void)
{
    if (!s_pwm_initialized)
    {
        return;
    }

    /*
     * Останавливаем оба канала.
     */
    BSP_PWMAllStop();

    /*
     * Убираем PWM-функцию с выводов.
     *
     * pwm_set_pin() в предоставленном pwm.c уже использует
     * gpio_set_mux_function() и gpio_function_dis().
     */
    gpio_function_dis((gpio_pin_e)GPIO_FC_PC7);
    gpio_function_dis((gpio_pin_e)GPIO_FC_PB1);

    s_pwm_initialized = 0;
}

void BSP_PWMWakeup(void)
{
    if (s_pwm_initialized)
    {
        return;
    }

    /*
     * Восстанавливаем PWM-функцию на выводах.
     */
    pwm_set_pin(GPIO_FC_PC7, PWM0);
    pwm_set_pin(GPIO_FC_PB1, PWM1);

    /*
     * Восстанавливаем нормальный режим PWM0.
     */
    pwm_set_pwm0_mode(PWM_NORMAL_MODE);

    /*
     * Восстанавливаем общий CLKDIV.
     */
    pwm_set_clk(s_pwm_clkdiv);

    /*
     * Восстанавливаем аппаратные параметры обоих каналов.
     *
     * Каналы НЕ запускаются.
     */
    BSP_PWMAllChannelsApply();

    s_pwm_initialized = 1;
}

