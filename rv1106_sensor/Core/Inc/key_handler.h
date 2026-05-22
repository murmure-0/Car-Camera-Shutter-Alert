#ifndef __KEY_HANDLER_H
#define __KEY_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 按键ID定义 */
#define KEY_ID_PA0           0   /* 独立按键PA0 */
#define KEY_ID_PC1_KEY1      1   /* PC1 ADC按键1 */
#define KEY_ID_PC1_KEY2      2   /* PC1 ADC按键2 */
#define KEY_ID_PC1_KEY3      3   /* PC1 ADC按键3 */

/* 按键类型定义 */
#define KEY_PRESS_SHORT      0   /* 短按 */
#define KEY_PRESS_LONG       1   /* 长按 */

/* 按键处理初始化 */
void KeyHandler_Init(void);

/* 按键扫描任务 */
void KeyHandler_ScanTask(void const *argument);

/* 获取按键状态 */
uint8_t KeyHandler_GetKeyState(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_HANDLER_H */
