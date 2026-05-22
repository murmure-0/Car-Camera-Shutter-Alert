#ifndef __TASK_QUEUE_H
#define __TASK_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"

/* 消息类型定义 */
typedef enum {
    TASK_MSG_KEY_PRESS = 0,
    TASK_MSG_KEY_LONG_PRESS,
    TASK_MSG_SENSOR_UPDATE,
    TASK_MSG_GPS_UPDATE,
    TASK_MSG_GESTURE,
    TASK_MSG_MAX
} TaskMsgType_t;

/* 按键消息结构 */
typedef struct {
    uint8_t key_id;              /* 按键ID: 0=PA0, 1=PC1_KEY1, 2=PC1_KEY2, 3=PC1_KEY3 */
    uint8_t press_type;          /* 按键类型: 0=短按, 1=长按 */
} KeyMsg_t;

/* 传感器消息结构 */
typedef struct {
    float temperature;
    float humidity;
    float battery_voltage;
    float motor_angle;
} SensorMsg_t;

/* GPS消息结构 */
typedef struct {
    float latitude;
    float longitude;
    uint8_t satellites;
} GPSMsg_t;

typedef struct {
    char gesture[32];
    char action[16];
} GestureMsg_t;

typedef struct {
    TaskMsgType_t type;
    union {
        KeyMsg_t key_msg;
        SensorMsg_t sensor_msg;
        GPSMsg_t gps_msg;
        GestureMsg_t gesture_msg;
        uint32_t data[4];
    } payload;
} TaskMsg_t;

/* 消息队列句柄 */
extern QueueHandle_t xTaskQueue;

/* 消息队列初始化 */
void TaskQueue_Init(void);

/* 发送按键消息 */
BaseType_t TaskQueue_SendKeyMsg(uint8_t key_id, uint8_t press_type, TickType_t xTicksToWait);

/* 发送传感器消息 */
BaseType_t TaskQueue_SendSensorMsg(SensorMsg_t *p_sensor_msg, TickType_t xTicksToWait);

/* 发送GPS消息 */
BaseType_t TaskQueue_SendGPSMsg(GPSMsg_t *p_gps_msg, TickType_t xTicksToWait);

BaseType_t TaskQueue_SendGestureMsg(GestureMsg_t *p_gesture_msg, TickType_t xTicksToWait);

BaseType_t TaskQueue_ReceiveMsg(TaskMsg_t *p_msg, TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_QUEUE_H */
