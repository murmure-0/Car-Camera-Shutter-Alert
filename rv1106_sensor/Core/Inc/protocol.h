#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// 传感器数据结构
typedef struct {
    // 1. 温湿度 (AHT30)
    float temp_c;
    float humi_pct;

    // 2. MPU6050
    float acc_x;
    float acc_y;
    float acc_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float mpu_temp;
    float roll;
    float pitch;
    float yaw;

    // 3. 电源监控 (INA226)
    float bus_voltage_v;
    float shunt_voltage_mv;
    float current_a;
    float power_w;

    // 4. ADC 数据
    float battery_v;
    uint32_t hall1_val;
    uint32_t hall2_val;

    // 5. GPS 数据 (简单版)
    float latitude;
    float longitude;
    uint8_t satellites;

    // 6. 步进电机
    float stepper_angle;

} Sensor_Report_t;

// 发送传感器数据 (JSON 格式)
void Protocol_SendSensorData(Sensor_Report_t *report);

#ifdef __cplusplus
}
#endif

#endif /* __PROTOCOL_H */
