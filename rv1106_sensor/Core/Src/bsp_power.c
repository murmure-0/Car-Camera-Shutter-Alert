#include "bsp_power.h"

void BSP_Power_Init(void) {
    // 引脚通常在 gpio.c (MX_GPIO_Init) 中初始化
    // 如果需要，我们确保它们最初处于安全状态（关闭）。
    // 假设 EN 引脚为高电平有效。
    HAL_GPIO_WritePin(BOOST_EN_GPIO_Port, BOOST_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RV1106_POWER_EN_GPIO_Port, RV1106_POWER_EN_Pin, GPIO_PIN_RESET);
}

void BSP_Power_SetSensorPower(uint8_t enable) {
    if (enable) {
        HAL_GPIO_WritePin(BOOST_EN_GPIO_Port, BOOST_EN_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(BOOST_EN_GPIO_Port, BOOST_EN_Pin, GPIO_PIN_RESET);
    }
}

void BSP_Power_SetRV1106Power(uint8_t enable) {
    if (enable) {
        HAL_GPIO_WritePin(RV1106_POWER_EN_GPIO_Port, RV1106_POWER_EN_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(RV1106_POWER_EN_GPIO_Port, RV1106_POWER_EN_Pin, GPIO_PIN_RESET);
    }
}
