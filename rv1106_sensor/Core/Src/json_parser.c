#include "json_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "task_queue.h"

extern uint8_t g_usart3_rx_buf[];
extern volatile uint16_t g_usart3_rx_len;
extern volatile uint8_t g_usart3_data_ready;

/**
 * @brief 查找子字符串
 */
static char* find_substring(const char *str, const char *substr)
{
    return strstr(str, substr);
}

/**
 * @brief 提取字符串值
 */
static int extract_string_value(const char *str, const char *key, char *value, size_t max_len)
{
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":\"", key);
    
    char *start = find_substring(str, search_key);
    if (start == NULL) {
        return -1;
    }
    
    start += strlen(search_key);
    char *end = strchr(start, '"');
    if (end == NULL) {
        return -1;
    }
    
    size_t len = end - start;
    if (len >= max_len) {
        len = max_len - 1;
    }
    
    strncpy(value, start, len);
    value[len] = '\0';
    
    return 0;
}

/**
 * @brief 提取布尔值
 */
static int extract_bool_value(const char *str, const char *key, uint8_t *value)
{
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    char *start = find_substring(str, search_key);
    if (start == NULL) {
        return -1;
    }
    
    start += strlen(search_key);
    
    /* 跳过空格 */
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    
    if (strncmp(start, "true", 4) == 0) {
        *value = 1;
        return 0;
    } else if (strncmp(start, "false", 5) == 0) {
        *value = 0;
        return 0;
    }
    
    return -1;
}

/**
 * @brief 解析网络状态消息
 */
static int parse_network_msg(const char *json_str, NetworkData_t *data)
{
    /* 提取connected字段 */
    if (extract_bool_value(json_str, "connected", &data->connected) != 0) {
        return -1;
    }
    
    /* 提取type字段 */
    if (extract_string_value(json_str, "type", data->type, sizeof(data->type)) != 0) {
        strcpy(data->type, "unknown");
    }
    
    return 0;
}

/**
 * @brief 解析疲劳状态消息
 */
static int parse_fatigue_msg(const char *json_str, FatigueData_t *data)
{
    /* 提取is_tired字段 */
    if (extract_bool_value(json_str, "is_tired", &data->is_tired) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 解析手势识别消息
 */
static int parse_gesture_msg(const char *json_str, GestureData_t *data)
{
    /* 提取gesture字段 */
    if (extract_string_value(json_str, "gesture", data->gesture, sizeof(data->gesture)) != 0) {
        return -1;
    }
    
    /* 提取action字段 */
    if (extract_string_value(json_str, "action", data->action, sizeof(data->action)) != 0) {
        strcpy(data->action, "none");
    }
    
    return 0;
}

/**
 * @brief 初始化JSON解析模块
 */
void JsonParser_Init(void)
{
    /* 初始化完成 */
}

/**
 * @brief 解析JSON字符串
 * @param json_str JSON字符串
 * @param msg 解析结果
 * @return 0=成功, -1=失败
 */
int JsonParser_Parse(const char *json_str, JsonMsg_t *msg)
{
    if (json_str == NULL || msg == NULL) {
        return -1;
    }
    
    char type_value[32];
    if (extract_string_value(json_str, "type", type_value, sizeof(type_value)) == 0) {
        if (strcmp(type_value, "network") == 0) {
            msg->type = MSG_TYPE_NETWORK;
            if (parse_network_msg(json_str, &msg->data.network) != 0) {
                return -1;
            }
        }
        else if (strcmp(type_value, "fatigue") == 0) {
            msg->type = MSG_TYPE_FATIGUE;
            if (parse_fatigue_msg(json_str, &msg->data.fatigue) != 0) {
                return -1;
            }
        }
        else if (strcmp(type_value, "gesture") == 0) {
            msg->type = MSG_TYPE_GESTURE;
            if (parse_gesture_msg(json_str, &msg->data.gesture) != 0) {
                return -1;
            }
        }
        else {
            msg->type = MSG_TYPE_UNKNOWN;
            return -1;
        }
    }
    else {
        if (find_substring(json_str, "\"gesture\"") != NULL) {
            msg->type = MSG_TYPE_GESTURE;
            if (parse_gesture_msg(json_str, &msg->data.gesture) != 0) {
                return -1;
            }
        }
        else if (find_substring(json_str, "\"is_tired\"") != NULL) {
            msg->type = MSG_TYPE_FATIGUE;
            if (parse_fatigue_msg(json_str, &msg->data.fatigue) != 0) {
                return -1;
            }
        }
        else if (find_substring(json_str, "\"connected\"") != NULL) {
            msg->type = MSG_TYPE_NETWORK;
            if (parse_network_msg(json_str, &msg->data.network) != 0) {
                return -1;
            }
        }
        else {
            msg->type = MSG_TYPE_UNKNOWN;
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief 打印解析结果
 * @param msg 解析后的消息
 */
void JsonParser_PrintResult(const JsonMsg_t *msg)
{
    if (msg == NULL) {
        return;
    }
    
    switch (msg->type) {
        case MSG_TYPE_NETWORK:
            printf("[NETWORK] connected=%s, type=%s\r\n",
                   msg->data.network.connected ? "true" : "false",
                   msg->data.network.type);
            break;
            
        case MSG_TYPE_FATIGUE:
            printf("[FATIGUE] is_tired=%s\r\n",
                   msg->data.fatigue.is_tired ? "true" : "false");
            break;
            
        case MSG_TYPE_GESTURE:
            printf("[GESTURE] gesture=%s, action=%s\r\n",
                   msg->data.gesture.gesture,
                   msg->data.gesture.action);
            break;
            
        default:
            printf("[UNKNOWN] Unknown message type\r\n");
            break;
    }
}

/**
 * @brief JSON解析任务
 * @param argument 任务参数（未使用）
 */
void JsonParser_Task(void const *argument)
{
    char json_buf[256];
    uint16_t idx = 0;
    uint8_t in_json = 0;
    JsonMsg_t msg;
    
    printf("JSON Parser Task Started (RV1106->USART3 DMA+IDLE)\r\n");
    
    for(;;)
    {
        if (g_usart3_data_ready) {
            g_usart3_data_ready = 0;
            
            if (g_usart3_rx_len > 0) {
                /* 先将原始数据发送到串口1 */
                printf("[USART3 RAW] ");
                for (uint16_t i = 0; i < g_usart3_rx_len; i++) {
                    printf("%02X ", g_usart3_rx_buf[i]);
                }
                printf("\r\n");

                /* 处理接收到的数据 */
                for (uint16_t i = 0; i < g_usart3_rx_len; i++) {
                    char c = (char)g_usart3_rx_buf[i];
                    
                    /* 检测JSON开始 */
                    if (c == '{') {
                        in_json = 1;
                        idx = 0;
                        json_buf[idx++] = c;
                    }
                    /* 检测JSON结束 */
                    else if (c == '}' && in_json) {
                        if (idx < sizeof(json_buf) - 1) {
                            json_buf[idx++] = c;
                            json_buf[idx] = '\0';
                            
                            if (JsonParser_Parse(json_buf, &msg) == 0) {
                                JsonParser_PrintResult(&msg);
                                
                                if (msg.type == MSG_TYPE_GESTURE) {
                                    GestureMsg_t gesture_msg;
                                    strncpy(gesture_msg.gesture, msg.data.gesture.gesture, sizeof(gesture_msg.gesture) - 1);
                                    gesture_msg.gesture[sizeof(gesture_msg.gesture) - 1] = '\0';
                                    strncpy(gesture_msg.action, msg.data.gesture.action, sizeof(gesture_msg.action) - 1);
                                    gesture_msg.action[sizeof(gesture_msg.action) - 1] = '\0';
                                    TaskQueue_SendGestureMsg(&gesture_msg, 0);
                                }
                            } else {
                                printf("[JSON_PARSE_ERROR] %s\r\n", json_buf);
                            }
                        }
                        in_json = 0;
                        idx = 0;
                    }
                    /* 接收JSON内容 */
                    else if (in_json && idx < sizeof(json_buf) - 1) {
                        json_buf[idx++] = c;
                    }
                }
            }
        }
        
        osDelay(10);
    }
}
