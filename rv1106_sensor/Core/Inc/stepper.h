#ifndef __STEPPER_H
#define __STEPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Stepper_Init(void);
void Stepper_Step(int steps, uint32_t delay_ms);
void Stepper_SingleStep(int direction);

// 角度跟踪
void Stepper_ResetPosition(void);
float Stepper_GetAngle(void);
void Stepper_SetAngle(float angle);

// 测试函数
void Stepper_ActivatePhase(uint8_t phase);
void Stepper_SingleStepWithSequence(int direction, const uint8_t sequence[4]);

#ifdef __cplusplus
}
#endif

#endif /* __STEPPER_H */
