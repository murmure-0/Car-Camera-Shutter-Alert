#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "adc.h"

// Channel Definitions (Check STM32L4 datasheet or generated code mapping)
// PC0 -> ADC1_IN1
// PC1 -> ADC1_IN2
// PC2 -> ADC1_IN3

uint32_t BSP_ADC_ReadChannel(uint32_t channel);
float BSP_GetBatteryVoltage(void);
uint32_t BSP_GetHallSensor1(void);
uint32_t BSP_GetHallSensor2(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADC_H */
