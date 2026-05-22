#include "led_handler.h"
#include "FreeRTOS.h"
#include "task.h"

/* LED状态变量 */
static uint32_t system_led_tick = 0;
static uint8_t gps_led_state = LED_OFF;
static uint32_t gps_led_tick = 0;
static uint8_t gps_satellites = 0;

/**
 * @brief LED初始化
 * @details 初始化LED相关的GPIO引脚
 */
void LedHandler_Init(void)
{
    /* LED1 - 系统运行指示 (PB5) */
    HAL_GPIO_WritePin(LED_SYS_PORT, LED_SYS_PIN, GPIO_PIN_RESET);
    
    /* LED3 - GPS信号指示 (PB6) */
    HAL_GPIO_WritePin(LED_GPS_PORT, LED_GPS_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 设置LED状态
 * @param pin 引脚号
 * @param port GPIO端口
 * @param state LED状态
 */
void LedHandler_SetLed(uint16_t pin, GPIO_TypeDef* port, uint8_t state)
{
    switch (state) {
        case LED_OFF:
            HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
            break;
        case LED_ON:
            HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
            break;
        default:
            /* 闪烁模式由定时任务处理 */
            break;
    }
}

/**
 * @brief 更新系统LED状态
 * @details 每100ms调用一次，实现LED闪烁
 */
void LedHandler_UpdateSystemLed(void)
{
    TickType_t now = xTaskGetTickCount();
    
    /* 每100ms翻转一次 */
    if ((now - system_led_tick) >= 100) {
        system_led_tick = now;
        
        /* 翻转LED状态 */
        HAL_GPIO_TogglePin(LED_SYS_PORT, LED_SYS_PIN);
    }
}

/**
 * @brief 更新GPS LED状态
 * @param satellites 卫星数量
 */
void LedHandler_UpdateGpsLed(uint8_t satellites)
{
    gps_satellites = satellites;
    
    TickType_t now = xTaskGetTickCount();
    
    /* 根据卫星数量设置LED闪烁频率 */
    if (satellites == 0) {
        /* 无信号：熄灭 */
        if (gps_led_state != LED_OFF) {
            gps_led_state = LED_OFF;
            LedHandler_SetLed(LED_GPS_PIN, (GPIO_TypeDef*)LED_GPS_PORT, LED_OFF);
        }
    }
    else if (satellites < 4) {
        /* 弱信号：1Hz 慢闪 */
        if (gps_led_state != LED_BLINK_SLOW) {
            gps_led_state = LED_BLINK_SLOW;
            gps_led_tick = now;
        }
        
        if ((now - gps_led_tick) >= 500) {
            gps_led_tick = now;
            HAL_GPIO_TogglePin(LED_GPS_PORT, LED_GPS_PIN);
        }
    }
    else if (satellites < 8) {
        /* 中信号：2Hz 中闪 */
        if (gps_led_state != LED_BLINK_MEDIUM) {
            gps_led_state = LED_BLINK_MEDIUM;
            gps_led_tick = now;
        }
        
        if ((now - gps_led_tick) >= 250) {
            gps_led_tick = now;
            HAL_GPIO_TogglePin(LED_GPS_PORT, LED_GPS_PIN);
        }
    }
    else {
        /* 强信号：5Hz 快闪 */
        if (gps_led_state != LED_BLINK_FAST) {
            gps_led_state = LED_BLINK_FAST;
            gps_led_tick = now;
        }
        
        if ((now - gps_led_tick) >= 100) {
            gps_led_tick = now;
            HAL_GPIO_TogglePin(LED_GPS_PORT, LED_GPS_PIN);
        }
    }
}

/**
 * @brief 获取当前卫星数量
 * @return 卫星数量
 */
uint8_t LedHandler_GetGpsSatellites(void)
{
    return gps_satellites;
}
