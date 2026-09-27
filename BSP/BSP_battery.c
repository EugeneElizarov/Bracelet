#include "BSP_battery.h"
#include "tl_common.h"


void BSP_BatteryInit(void)
{
	adc_init(NDMA_M_CHN);
    adc_vbat_sample_init(ADC_M_CHANNEL);
    adc_set_vbat_divider(ADC_M_CHANNEL, ADC_VBAT_DIV_1F4);
}

void BSP_BatteryADCPowerSet(bool on)
{
	if (on)
		adc_power_on();
	else
		adc_power_off();
}

void BSP_BatteryMesaureStart(void)
{
	adc_clr_rx_fifo_cnt();
    adc_clr_irq_status();

    adc_start_sample_nodma();
}

bool BSP_BatteryMeasured(void)
{
	return (adc_get_rxfifo_cnt() != 0);
}

unsigned short BSP_BatteryLevelGet(void)
{
	unsigned short value = adc_get_raw_code();

    return adc_calculate_voltage(ADC_M_CHANNEL, value);
}
