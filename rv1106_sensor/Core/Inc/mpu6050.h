#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "soft_i2c.h"

#define MPU6050_ADDR         0xD0

#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75

typedef struct {
    int16_t Accel_X_RAW;
    int16_t Accel_Y_RAW;
    int16_t Accel_Z_RAW;
    int16_t Gyro_X_RAW;
    int16_t Gyro_Y_RAW;
    int16_t Gyro_Z_RAW;
    float Ax;
    float Ay;
    float Az;
    float Gx;
    float Gy;
    float Gz;
    float Temperature;
} MPU6050_Data_t;

uint8_t MPU6050_Init(SoftI2C_Handle_t *hi2c);
void MPU6050_Read_All(SoftI2C_Handle_t *hi2c, MPU6050_Data_t *DataStruct);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
