#include "bsp_adc.h"

extern ADC_HandleTypeDef hadc1;

uint32_t BSP_ADC_ReadChannel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5; // Longer sampling for stability
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        uint32_t value = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        return value;
    }
    
    HAL_ADC_Stop(&hadc1);
    return 0;
}

float BSP_GetBatteryVoltage(void) {
    // 使能电池分压
    HAL_GPIO_WritePin(BAT_ADC_EN_GPIO_Port, BAT_ADC_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(10); // 等待稳定
    
    uint32_t adc_val = BSP_ADC_ReadChannel(ADC_CHANNEL_3); // PC2 在 STM32L4 上通常是通道 3
    
    // 禁用电池分压（省电）
    HAL_GPIO_WritePin(BAT_ADC_EN_GPIO_Port, BAT_ADC_EN_Pin, GPIO_PIN_RESET);
    
    // 转换为电压
    // 参考电压 Vref = 3.3V，分辨率 12位 (4095)
    // 引脚电压 = (adc_val / 4095.0) * 3.3
    // 假设电压分压（例如 1/2 或其他）。暂时假设 1/2 分压（输入高达 6.6V）。
    // 您应该根据实际电阻调整乘数。
    // 例如：R1=10k, R2=10k -> 乘数 = 2。
    float pin_voltage = (adc_val / 4095.0f) * 3.3f;
    return pin_voltage * 2.0f; // 假设 1:1 分压
}

uint32_t BSP_GetHallSensor1(void) {
    return BSP_ADC_ReadChannel(ADC_CHANNEL_1); // PC0
}

uint32_t BSP_GetHallSensor2(void) {
    return BSP_ADC_ReadChannel(ADC_CHANNEL_2); // PC1
}
