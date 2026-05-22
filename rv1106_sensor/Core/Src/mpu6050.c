#include "mpu6050.h"

uint8_t MPU6050_Init(SoftI2C_Handle_t *hi2c) {
    uint8_t check, data;

    // 检查设备 ID
    if (SoftI2C_ReadReg(hi2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I, &check) != HAL_OK) {
        return 1; // 错误
    }

    if (check == 0x68) {
        // 唤醒设备
        data = 0x00;
        SoftI2C_WriteReg(hi2c, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, data);

        // 设置数据速率
        data = 0x07;
        SoftI2C_WriteReg(hi2c, MPU6050_ADDR, MPU6050_REG_SMPLRT_DIV, data);

        // 设置加速度计配置 (+/- 2g)
        data = 0x00;
        SoftI2C_WriteReg(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG, data);

        // 设置陀螺仪配置 (+/- 250 dps)
        data = 0x00;
        SoftI2C_WriteReg(hi2c, MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG, data);
        
        return 0; // 成功
    }
    return 1; // 错误
}

void MPU6050_Read_All(SoftI2C_Handle_t *hi2c, MPU6050_Data_t *DataStruct) {
    uint8_t Rec_Data[14];
    int16_t temp;

    // 从 ACCEL_XOUT_H 开始读取 14 个字节
    SoftI2C_ReadBuffer(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, Rec_Data, 14);

    DataStruct->Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    DataStruct->Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    DataStruct->Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
    temp = (int16_t)(Rec_Data[6] << 8 | Rec_Data[7]);
    DataStruct->Gyro_X_RAW = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
    DataStruct->Gyro_Y_RAW = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
    DataStruct->Gyro_Z_RAW = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);

    DataStruct->Ax = DataStruct->Accel_X_RAW / 16384.0;
    DataStruct->Ay = DataStruct->Accel_Y_RAW / 16384.0;
    DataStruct->Az = DataStruct->Accel_Z_RAW / 16384.0;
    
    DataStruct->Temperature = (float)((int16_t)temp / (float)340.0 + (float)36.53);

    DataStruct->Gx = DataStruct->Gyro_X_RAW / 131.0;
    DataStruct->Gy = DataStruct->Gyro_Y_RAW / 131.0;
    DataStruct->Gz = DataStruct->Gyro_Z_RAW / 131.0;
}
