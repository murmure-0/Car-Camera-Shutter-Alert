#ifndef __BSP_POWER_H
#define __BSP_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* Power Control Functions */

/**
 * @brief Enable/Disable Total Sensor Power (Boost Converter)
 * @param enable: 1 to enable, 0 to disable
 */
void BSP_Power_SetSensorPower(uint8_t enable);

/**
 * @brief Enable/Disable RV1106 Core Board Power
 * @param enable: 1 to enable, 0 to disable
 */
void BSP_Power_SetRV1106Power(uint8_t enable);

/**
 * @brief Initialize Power Control Pins
 *        (Call this if not initialized in gpio.c, though usually they are)
 */
void BSP_Power_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_POWER_H */
