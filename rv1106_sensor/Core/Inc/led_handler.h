#ifndef __LED_HANDLER_H
#define __LED_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* LED引脚定义 */
#define LED_SYS_PIN          GPIO_PIN_5   /* LED1 - 系统运行指示 (PB5) */
#define LED_SYS_PORT         GPIOB
#define LED_GPS_PIN          GPIO_PIN_6   /* LED3 - GPS信号指示 (PB6) */
#define LED_GPS_PORT         GPIOB

/* LED状态定义 */
#define LED_OFF              0
#define LED_ON               1
#define LED_BLINK_SLOW       2   /* 1Hz 慢闪 */
#define LED_BLINK_MEDIUM     3   /* 2Hz 中闪 */
#define LED_BLINK_FAST       4   /* 5Hz 快闪 */

/* GPS信号强度定义 */
#define GPS_SIGNAL_NONE      0   /* 无信号 */
#define GPS_SIGNAL_WEAK      1   /* 弱信号 (<4颗) */
#define GPS_SIGNAL_MEDIUM    2   /* 中信号 (4-7颗) */
#define GPS_SIGNAL_STRONG    3   /* 强信号 (≥8颗) */

/* LED控制函数 */
void LedHandler_Init(void);
void LedHandler_UpdateSystemLed(void);
void LedHandler_UpdateGpsLed(uint8_t satellites);
void LedHandler_Task(void const * argument);

/* 设置LED状态 */
void LedHandler_SetLed(uint16_t pin, GPIO_TypeDef* port, uint8_t state);

/* 获取GPS卫星数量 */
uint8_t LedHandler_GetGpsSatellites(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_HANDLER_H */
