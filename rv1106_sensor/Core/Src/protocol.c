#include "protocol.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

// 辅助函数：发送字符串到指定 UART
static void UART_SendString(UART_HandleTypeDef *huart, char *str) {
    HAL_UART_Transmit(huart, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void Protocol_SendSensorData(Sensor_Report_t *report) {
    char buffer[512]; // 确保缓冲区足够大以容纳 JSON 字符串
    
    // 构建 JSON 字符串
    // 使用 snprintf 格式化数据
    // 注意：浮点数打印需要编译器支持 float 格式化 (-u _printf_float)
    int len = snprintf(buffer, sizeof(buffer),
        "{"
        "\"temp\":%.2f,"
        "\"humi\":%.2f,"
        "\"mpu\":{"
            "\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,"
            "\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,"
            "\"t\":%.2f,"
            "\"ang\":{"
                "\"roll\":%.2f,\"pitch\":%.2f,\"yaw\":%.2f"
            "}"
        "},"
        "\"pwr\":{"
            "\"v\":%.2f,\"vs\":%.2f,\"i\":%.4f,\"p\":%.4f"
        "},"
        "\"adc\":{"
            "\"bat\":%.2f,\"h1\":%lu,\"h2\":%lu"
        "},"
        "\"gps\":{"
            "\"lat\":%.6f,\"lon\":%.6f,\"sat\":%d"
        "},"
        "\"motor\":{"
            "\"ang\":%.2f"
        "}"
        "}\n", // 以换行符结尾，方便接收端按行读取
        report->temp_c, report->humi_pct,
        report->acc_x, report->acc_y, report->acc_z,
        report->gyro_x, report->gyro_y, report->gyro_z, report->mpu_temp,
        report->roll, report->pitch, report->yaw,
        report->bus_voltage_v, report->shunt_voltage_mv, report->current_a, report->power_w,
        report->battery_v, report->hall1_val, report->hall2_val,
        report->latitude, report->longitude, report->satellites,
        report->stepper_angle
    );

    if (len > 0 && len < sizeof(buffer)) {
        // UART_SendString(&huart1, buffer);
        UART_SendString(&huart3, buffer);
    } else {
        // 缓冲区溢出处理（可选）
        UART_SendString(&huart1, "{\"error\":\"buffer_overflow\"}\n");
        UART_SendString(&huart3, "{\"error\":\"buffer_overflow\"}\n");
    }
}
