#ifndef __BUSINESS_HANDLER_H
#define __BUSINESS_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "key_handler.h"

/* 业务处理初始化 */
void BusinessHandler_Init(void);

/* 业务处理任务 */
void BusinessHandler_Task(void const *argument);

void BusinessHandler_ProcessKeyEvent(uint8_t key_id, uint8_t press_type);
void BusinessHandler_ProcessSensorUpdate(void);
void BusinessHandler_ProcessGPSUpdate(void);
void BusinessHandler_ProcessGesture(const char *gesture, const char *action);

#ifdef __cplusplus
}
#endif

#endif /* __BUSINESS_HANDLER_H */
