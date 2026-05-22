#include "stepper.h"
#include <stdlib.h>

// Stepper Sequence (4-step single phase)
// D0, D1, D2, D3
static const uint8_t step_seq[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

// 全局步数计数器
static int32_t global_step_count = 0;
// 假设每转步数 (根据实际电机调整，28BYJ-48 通常为 2048)
#define STEPS_PER_REV 2048.0f

void Stepper_Init(void) {
    // Stop Motor
    HAL_GPIO_WritePin(STEP_D0_GPIO_Port, STEP_D0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D1_GPIO_Port, STEP_D1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D2_GPIO_Port, STEP_D2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D3_GPIO_Port, STEP_D3_Pin, GPIO_PIN_RESET);
}

static void Stepper_SetPhase(uint8_t step) {
    HAL_GPIO_WritePin(STEP_D0_GPIO_Port, STEP_D0_Pin, step_seq[step][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D1_GPIO_Port, STEP_D1_Pin, step_seq[step][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D2_GPIO_Port, STEP_D2_Pin, step_seq[step][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D3_GPIO_Port, STEP_D3_Pin, step_seq[step][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void Stepper_SetPhaseWithSequence(uint8_t step, const uint8_t sequence[4]) {
    // 根据 sequence 映射 step (0-3) 到实际的相位索引
    // sequence[step] 返回要激活的实际相位 (0, 1, 2, 3)
    // 默认 sequence 是 {0, 1, 2, 3}
    // 另一种可能是 {0, 2, 1, 3} 等等
    
    uint8_t actual_phase = sequence[step];
    
    // 使用原始的 step_seq 来激活对应的物理引脚
    // step_seq[actual_phase] 对应 {D0, D1, D2, D3} 的电平
    HAL_GPIO_WritePin(STEP_D0_GPIO_Port, STEP_D0_Pin, step_seq[actual_phase][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D1_GPIO_Port, STEP_D1_Pin, step_seq[actual_phase][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D2_GPIO_Port, STEP_D2_Pin, step_seq[actual_phase][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEP_D3_GPIO_Port, STEP_D3_Pin, step_seq[actual_phase][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Stepper_SingleStepWithSequence(int direction, const uint8_t sequence[4]) {
    static int current_step = 0;
    
    if (direction > 0) direction = 1;
    if (direction < 0) direction = -1;
    if (direction == 0) return;

    current_step += direction;
    if (current_step > 3) current_step = 0;
    if (current_step < 0) current_step = 3;
    
    Stepper_SetPhaseWithSequence(current_step, sequence);
}

void Stepper_ActivatePhase(uint8_t phase) {
    if (phase > 3) phase = 0;
    Stepper_SetPhase(phase);
}

void Stepper_SingleStep(int direction) {
    static int current_step = 0;
    
    // Normalize direction
    if (direction > 0) direction = 1;
    if (direction < 0) direction = -1;
    if (direction == 0) return;

    current_step += direction;
    global_step_count += direction; // 更新全局位置

    if (current_step > 3) current_step = 0;
    if (current_step < 0) current_step = 3;
    
    Stepper_SetPhase(current_step);
}

void Stepper_ResetPosition(void) {
    global_step_count = 0;
}

float Stepper_GetAngle(void) {
    float angle = global_step_count * (360.0f / STEPS_PER_REV);
    return angle;
}

void Stepper_SetAngle(float angle) {
    float current_angle = Stepper_GetAngle();
    float diff = angle - current_angle;
    
    int steps = (int)(diff * (STEPS_PER_REV / 360.0f));
    
    int direction = (steps > 0) ? 1 : -1;
    steps = abs(steps);
    
    for (int i = 0; i < steps; i++) {
        Stepper_SingleStep(direction);
        HAL_Delay(5);
    }
}

void Stepper_Step(int steps, uint32_t delay_ms) {
    int direction = (steps > 0) ? 1 : -1;
    steps = abs(steps);

    for (int i = 0; i < steps; i++) {
        Stepper_SingleStep(direction);
        HAL_Delay(delay_ms);
    }
    
    // 可选：运动后关闭线圈以省电
    // Stepper_Init(); 
}
