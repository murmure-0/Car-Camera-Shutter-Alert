#include "ina226.h"

// Rshunt = 0.1 Ohm 的默认校准值
// 最大电流 ~0.8A -> 电流分辨率(Current_LSB) = 25uA (0.000025 A)
// 校准值(Cal) = 0.00512 / (Current_LSB * Rshunt)
// 校准值(Cal) = 0.00512 / (0.000025 * 0.1) = 2048 (0x0800)

static uint16_t current_cal_value = 2048; 

uint8_t INA226_Init(SoftI2C_Handle_t *hi2c) {
    uint16_t id;
    uint8_t buf[2];

    // 检查设备ID
    if (SoftI2C_ReadBuffer(hi2c, INA226_ADDR, INA226_REG_MANUFACTURER_ID, buf, 2) != HAL_OK) {
        return 1;
    }
    // TI 的厂商ID是 0x5449
    id = (buf[0] << 8) | buf[1];
    if (id != 0x5449) {
        // return 1; // 针对克隆芯片禁用严格检查
    }

    // 配置: 平均次数 16, 总线电压转换时间 1.1ms, 分流电压转换时间 1.1ms, 模式 连续分流+总线电压
    // 0100 001 100 100 111 -> 0x4127
    uint16_t config = 0x4127;
    buf[0] = (config >> 8) & 0xFF;
    buf[1] = config & 0xFF;
    if (SoftI2C_WriteBuffer(hi2c, INA226_ADDR, INA226_REG_CONFIG, buf, 2) != HAL_OK) return 1;

    // 设置校准值
    INA226_SetCalibration(hi2c, current_cal_value);

    return 0;
}

void INA226_SetCalibration(SoftI2C_Handle_t *hi2c, uint16_t calValue) {
    uint8_t buf[2];
    current_cal_value = calValue;
    buf[0] = (calValue >> 8) & 0xFF;
    buf[1] = calValue & 0xFF;
    SoftI2C_WriteBuffer(hi2c, INA226_ADDR, INA226_REG_CALIBRATION, buf, 2);
}

uint8_t INA226_Read_Data(SoftI2C_Handle_t *hi2c, INA226_Data_t *data) {
    uint8_t buf[2];
    int16_t raw;

    // 读取总线电压
    if (SoftI2C_ReadBuffer(hi2c, INA226_ADDR, INA226_REG_BUS_VOLTAGE, buf, 2) != HAL_OK) return 1;
    raw = (buf[0] << 8) | buf[1];
    data->Voltage_V = (float)raw * 0.00125f;

    // 读取分流电压
    if (SoftI2C_ReadBuffer(hi2c, INA226_ADDR, INA226_REG_SHUNT_VOLTAGE, buf, 2) != HAL_OK) return 1;
    raw = (buf[0] << 8) | buf[1];
    data->ShuntVoltage_mV = (float)raw * 0.0025f; // 2.5uV LSB

    // 读取电流
    if (SoftI2C_ReadBuffer(hi2c, INA226_ADDR, INA226_REG_CURRENT, buf, 2) != HAL_OK) return 1;
    raw = (buf[0] << 8) | buf[1];
    // 电流分辨率(Current LSB) = 25uA (0.025mA)
    data->Current_A = (float)raw * 0.000025f;

    // 读取功率
    if (SoftI2C_ReadBuffer(hi2c, INA226_ADDR, INA226_REG_POWER, buf, 2) != HAL_OK) return 1;
    raw = (buf[0] << 8) | buf[1];
    // 功率分辨率(Power LSB) = 25 * Current_LSB = 25 * 25uA = 625uW (0.000625 W)
    data->Power_W = (float)raw * 0.000625f;

    return 0;
}
