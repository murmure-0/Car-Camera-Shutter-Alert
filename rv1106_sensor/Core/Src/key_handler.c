#include "key_handler.h"
#include "task_queue.h"
#include "bsp_adc.h"
#include "cmsis_os.h"
#include <stdio.h>

/* 按键状态变量 */
static uint8_t last_adc_btn = 0;
static uint32_t adc_btn_press_time = 0;
static uint8_t adc_btn_long_sent = 0;
static GPIO_PinState last_pa0_state = GPIO_PIN_RESET;
static uint32_t pa0_press_time = 0;
static uint8_t pa0_long_sent = 0;

/* 外部变量声明 */
extern volatile uint32_t g_pc1_adc;

/**
 * @brief 按键处理初始化
 * @details 初始化按键相关的硬件和软件状态
 */
void KeyHandler_Init(void)
{
    last_adc_btn = 0;
    adc_btn_press_time = 0;
    adc_btn_long_sent = 0;
    last_pa0_state = GPIO_PIN_RESET;
    pa0_press_time = 0;
    pa0_long_sent = 0;
}

/**
 * @brief 按键扫描任务
 * @details 轮询扫描所有按键状态，检测短按和长按事件
 * @param argument 任务参数（未使用）
 */
void KeyHandler_ScanTask(void const *argument)
{
    for(;;)
    {
        /* 扫描PC1 ADC按键 */
        uint32_t pc1_adc = BSP_ADC_ReadChannel(ADC_CHANNEL_2);
        
        /* 更新全局ADC值 */
        taskENTER_CRITICAL();
        g_pc1_adc = pc1_adc;
        taskEXIT_CRITICAL();

        /* 读取ADC按键值 */
        uint8_t adc_btn = 0;
        if (pc1_adc < 400) {
            adc_btn = KEY_ID_PC1_KEY2;
        } else if (pc1_adc < 1800) {
            adc_btn = KEY_ID_PC1_KEY1;
        } else if (pc1_adc < 4500) {
            adc_btn = KEY_ID_PC1_KEY3;
        }

        /* 检测按键按下事件 */
        if (adc_btn != 0 && adc_btn != last_adc_btn) {
            adc_btn_press_time = xTaskGetTickCount();
            adc_btn_long_sent = 0;
            last_adc_btn = adc_btn;
        }
        /* 检测按键持续按下 */
        else if (adc_btn != 0 && last_adc_btn != 0) {
            TickType_t now = xTaskGetTickCount();
            if ((now - adc_btn_press_time) >= (2000 / portTICK_PERIOD_MS) && !adc_btn_long_sent) {
                /* 发送长按消息 */
                TaskQueue_SendKeyMsg(adc_btn, KEY_PRESS_LONG, 0);
                adc_btn_long_sent = 1;
            }
        }
        /* 检测按键松开事件 */
        if (adc_btn == 0 && last_adc_btn != 0) {
            TickType_t now = xTaskGetTickCount();
            if ((now - adc_btn_press_time) < (2000 / portTICK_PERIOD_MS) && !adc_btn_long_sent) {
                /* 发送短按消息 */
                TaskQueue_SendKeyMsg(last_adc_btn, KEY_PRESS_SHORT, 0);
            }
            last_adc_btn = 0;
            adc_btn_long_sent = 0;
        }

        /* 扫描PA0独立按键 */
        GPIO_PinState pa0_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        
        /* 检测PA0按键按下 */
        if (pa0_state == GPIO_PIN_SET && last_pa0_state == GPIO_PIN_RESET) {
            pa0_press_time = xTaskGetTickCount();
            pa0_long_sent = 0;
            last_pa0_state = pa0_state;
        }
        /* 检测PA0按键持续按下 */
        else if (pa0_state == GPIO_PIN_SET && last_pa0_state == GPIO_PIN_SET) {
            TickType_t now = xTaskGetTickCount();
            if ((now - pa0_press_time) >= (2000 / portTICK_PERIOD_MS) && !pa0_long_sent) {
                /* 发送长按消息 */
                TaskQueue_SendKeyMsg(KEY_ID_PA0, KEY_PRESS_LONG, 0);
                pa0_long_sent = 1;
            }
        }
        /* 检测PA0按键松开 */
        if (pa0_state == GPIO_PIN_RESET && last_pa0_state == GPIO_PIN_SET) {
            TickType_t now = xTaskGetTickCount();
            if ((now - pa0_press_time) < (2000 / portTICK_PERIOD_MS) && !pa0_long_sent) {
                /* 发送短按消息 */
                TaskQueue_SendKeyMsg(KEY_ID_PA0, KEY_PRESS_SHORT, 0);
            }
            last_pa0_state = GPIO_PIN_RESET;
            pa0_long_sent = 0;
        }

        osDelay(100);
    }
}

/**
 * @brief 获取按键状态
 * @return 当前按键状态
 */
uint8_t KeyHandler_GetKeyState(void)
{
    return last_adc_btn;
}
