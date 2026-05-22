#ifndef __JSON_PARSER_H
#define __JSON_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 消息类型定义 */
typedef enum {
    MSG_TYPE_NETWORK = 0,
    MSG_TYPE_FATIGUE,
    MSG_TYPE_GESTURE,
    MSG_TYPE_UNKNOWN
} JsonMsgType_t;

/* 网络状态数据结构 */
typedef struct {
    uint8_t connected;      /* 1=已连接, 0=未连接 */
    char type[16];          /* "wifi"/"ethernet"/"mobile"/"none" */
} NetworkData_t;

/* 疲劳状态数据结构 */
typedef struct {
    uint8_t is_tired;       /* 1=疲劳, 0=正常 */
} FatigueData_t;

/* 手势识别数据结构 */
typedef struct {
    char gesture[32];       /* 手势名称 */
    char action[16];        /* 执行的动作 */
} GestureData_t;

/* JSON消息结构 */
typedef struct {
    JsonMsgType_t type;
    union {
        NetworkData_t network;
        FatigueData_t fatigue;
        GestureData_t gesture;
    } data;
} JsonMsg_t;

/* 初始化JSON解析模块 */
void JsonParser_Init(void);

/* JSON解析任务 */
void JsonParser_Task(void const *argument);

/* 解析JSON字符串 */
int JsonParser_Parse(const char *json_str, JsonMsg_t *msg);

/* 打印解析结果 */
void JsonParser_PrintResult(const JsonMsg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* __JSON_PARSER_H */
