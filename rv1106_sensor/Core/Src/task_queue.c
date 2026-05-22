#include "task_queue.h"

/* 消息队列句柄 */
QueueHandle_t xTaskQueue = NULL;

/* 消息队列配置 */
#define TASK_QUEUE_LENGTH    (16)
#define TASK_QUEUE_ITEM_SIZE sizeof(TaskMsg_t)

/**
 * @brief 初始化任务消息队列
 * @details 创建FreeRTOS消息队列，用于任务间通信
 */
void TaskQueue_Init(void)
{
    xTaskQueue = xQueueCreate(TASK_QUEUE_LENGTH, TASK_QUEUE_ITEM_SIZE);
    
    if (xTaskQueue != NULL) {
        /* 队列创建成功 */
    }
}

/**
 * @brief 发送按键消息
 * @param key_id 按键ID
 * @param press_type 按键类型（0=短按，1=长按）
 * @param xTicksToWait 等待超时时间
 * @return pdPASS成功，pdFAIL失败
 */
BaseType_t TaskQueue_SendKeyMsg(uint8_t key_id, uint8_t press_type, TickType_t xTicksToWait)
{
    TaskMsg_t msg;
    
    msg.type = TASK_MSG_KEY_PRESS;
    msg.payload.key_msg.key_id = key_id;
    msg.payload.key_msg.press_type = press_type;
    
    return xQueueSend(xTaskQueue, &msg, xTicksToWait);
}

/**
 * @brief 发送传感器消息
 * @param p_sensor_msg 传感器数据指针
 * @param xTicksToWait 等待超时时间
 * @return pdPASS成功，pdFAIL失败
 */
BaseType_t TaskQueue_SendSensorMsg(SensorMsg_t *p_sensor_msg, TickType_t xTicksToWait)
{
    TaskMsg_t msg;
    
    msg.type = TASK_MSG_SENSOR_UPDATE;
    msg.payload.sensor_msg.temperature = p_sensor_msg->temperature;
    msg.payload.sensor_msg.humidity = p_sensor_msg->humidity;
    msg.payload.sensor_msg.battery_voltage = p_sensor_msg->battery_voltage;
    msg.payload.sensor_msg.motor_angle = p_sensor_msg->motor_angle;
    
    return xQueueSend(xTaskQueue, &msg, xTicksToWait);
}

/**
 * @brief 发送GPS消息
 * @param p_gps_msg GPS数据指针
 * @param xTicksToWait 等待超时时间
 * @return pdPASS成功，pdFAIL失败
 */
BaseType_t TaskQueue_SendGPSMsg(GPSMsg_t *p_gps_msg, TickType_t xTicksToWait)
{
    TaskMsg_t msg;
    
    msg.type = TASK_MSG_GPS_UPDATE;
    msg.payload.gps_msg.latitude = p_gps_msg->latitude;
    msg.payload.gps_msg.longitude = p_gps_msg->longitude;
    msg.payload.gps_msg.satellites = p_gps_msg->satellites;
    
    return xQueueSend(xTaskQueue, &msg, xTicksToWait);
}

BaseType_t TaskQueue_SendGestureMsg(GestureMsg_t *p_gesture_msg, TickType_t xTicksToWait)
{
    TaskMsg_t msg;
    
    msg.type = TASK_MSG_GESTURE;
    strncpy(msg.payload.gesture_msg.gesture, p_gesture_msg->gesture, sizeof(msg.payload.gesture_msg.gesture) - 1);
    msg.payload.gesture_msg.gesture[sizeof(msg.payload.gesture_msg.gesture) - 1] = '\0';
    strncpy(msg.payload.gesture_msg.action, p_gesture_msg->action, sizeof(msg.payload.gesture_msg.action) - 1);
    msg.payload.gesture_msg.action[sizeof(msg.payload.gesture_msg.action) - 1] = '\0';
    
    return xQueueSend(xTaskQueue, &msg, xTicksToWait);
}

BaseType_t TaskQueue_ReceiveMsg(TaskMsg_t *p_msg, TickType_t xTicksToWait)
{
    return xQueueReceive(xTaskQueue, p_msg, xTicksToWait);
}
