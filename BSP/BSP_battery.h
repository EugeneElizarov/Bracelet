#ifndef __BSP_BATTERY_H__

#define __BSP_BATTERY_H__

#include <stdbool.h>

void BSP_BatteryInit(void);
void BSP_BatteryADCPowerSet(bool on);
void BSP_BatteryMesaureStart(void);
bool BSP_BatteryMeasured(void);
unsigned short BSP_BatteryLevelGet(void);

#endif
