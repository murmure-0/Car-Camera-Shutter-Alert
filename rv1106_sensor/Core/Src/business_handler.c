#include "business_handler.h"
#include "task_queue.h"
#include "stepper.h"
#include "bsp_power.h"
#include <stdio.h>
#include <string.h>

static uint8_t rv1106_power_on = 0;

/**
 * @brief 业务处理初始化
 * @details 初始化业务处理相关的资源
 */
void BusinessHandler_Init(void)
{
    rv1106_power_on = 0;
}

/**
 * @brief 业务处理任务
 * @details 接收消息队列中的消息，根据消息类型执行相应的业务逻辑
 * @param argument 任务参数（未使用）
 */
void BusinessHandler_Task(void const *argument)
{
    TaskMsg_t msg;
    
    for(;;)
    {
        /* 接收消息 */
        if (TaskQueue_ReceiveMsg(&msg, portMAX_DELAY) == pdPASS) {
            switch (msg.type) {
                case TASK_MSG_KEY_PRESS:
                    /* 处理按键事件 */
                    BusinessHandler_ProcessKeyEvent(msg.payload.key_msg.key_id, 
                                                   msg.payload.key_msg.press_type);
                    break;
                    
                case TASK_MSG_SENSOR_UPDATE:
                    /* 处理传感器更新 */
                    BusinessHandler_ProcessSensorUpdate();
                    break;
                    
                case TASK_MSG_GPS_UPDATE:
                    BusinessHandler_ProcessGPSUpdate();
                    break;
                    
                case TASK_MSG_GESTURE:
                    BusinessHandler_ProcessGesture(msg.payload.gesture_msg.gesture,
                                                   msg.payload.gesture_msg.action);
                    break;
                    
                default:
                    break;
            }
        }
    }
}

/**
 * @brief 处理按键事件
 * @details 根据按键ID和按键类型执行相应的业务逻辑
 * @param key_id 按键ID
 * @param press_type 按键类型（0=短按，1=长按）
 */
void BusinessHandler_ProcessKeyEvent(uint8_t key_id, uint8_t press_type)
{
    /* 预留扩展点：根据按键ID和类型执行不同业务逻辑 */
    if (key_id == KEY_ID_PA0) {
        if (press_type == KEY_PRESS_SHORT) {
            /* TODO: PA0短按业务逻辑 */
            printf("PA0_BTN=PRESS\r\n");
        } else if (press_type == KEY_PRESS_LONG) {
            /* PA0长按：开关机控制 */
            if (rv1106_power_on == 0) {
                /* 开机流程 */
                printf("PA0_BTN=LONG (Power ON)\r\n");
                Stepper_SetAngle(60.0f);
                BSP_Power_SetRV1106Power(1);
                rv1106_power_on = 1;
            } else {
                /* 关机流程 */
                printf("PA0_BTN=LONG (Power OFF)\r\n");
                Stepper_SetAngle(0.0f);
                BSP_Power_SetRV1106Power(0);
                rv1106_power_on = 0;
            }
        }
    }
    else if (key_id >= KEY_ID_PC1_KEY1 && key_id <= KEY_ID_PC1_KEY3) {
        if (press_type == KEY_PRESS_SHORT) {
            /* TODO: PC1按键短按业务逻辑 */
            const char *key_name = (key_id == KEY_ID_PC1_KEY1) ? "KEY1" : 
                                   (key_id == KEY_ID_PC1_KEY2) ? "KEY2" : "KEY3";
            printf("PC1_BTN=%s\r\n", key_name);
        } else if (press_type == KEY_PRESS_LONG) {
            /* TODO: PC1按键长按业务逻辑 */
            const char *key_name = (key_id == KEY_ID_PC1_KEY1) ? "KEY1" : 
                                   (key_id == KEY_ID_PC1_KEY2) ? "KEY2" : "KEY3";
            printf("PC1_BTN=%s_LONG\r\n", key_name);
        }
    }
}

/**
 * @brief 处理传感器更新
 * @details 处理传感器数据更新事件
 */
void BusinessHandler_ProcessSensorUpdate(void)
{
    /* TODO: 传感器数据处理逻辑 */
}

/**
 * @brief 处理GPS更新
 * @details 处理GPS数据更新事件
 */
void BusinessHandler_ProcessGPSUpdate(void)
{
}

void BusinessHandler_ProcessGesture(const char *gesture, const char *action)
{
    if (strcmp(gesture, "OK") == 0) {
        if (rv1106_power_on == 0) {
            printf("[GESTURE] OK -> Power ON\r\n");
            Stepper_SetAngle(60.0f);
            BSP_Power_SetRV1106Power(1);
            rv1106_power_on = 1;
        } else {
            printf("[GESTURE] OK -> Already ON\r\n");
        }
    }
    else if (strcmp(gesture, "Four") == 0) {
        if (rv1106_power_on == 1) {
            printf("[GESTURE] Four -> Power OFF\r\n");
            BSP_Power_SetRV1106Power(0);
            Stepper_SetAngle(0.0f);
            rv1106_power_on = 0;
        } else {
            printf("[GESTURE] Four -> Already OFF\r\n");
        }
    }
}
