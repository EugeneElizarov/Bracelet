/*
 * sensors.h
 *
 *  Created on: 13 июн. 2026 г.
 *      Author: eugen
 */

#ifndef VENDOR_ACL_PERIPHERAL_DEMO_SENSORS_H_
#define VENDOR_ACL_PERIPHERAL_DEMO_SENSORS_H_

void Sensors_Init(void);
void Sensors_ResetAlarm(void);
void Sensors_Sleep(void);
void Sensors_Wakeup(void);
void Sensor_GSRImitation(void);

#endif /* VENDOR_ACL_PERIPHERAL_DEMO_SENSORS_H_ */
