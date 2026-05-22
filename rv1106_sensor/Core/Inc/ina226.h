#ifndef __INA226_H
#define __INA226_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "soft_i2c.h"

#define INA226_ADDR 0x80 // Default Address 8-bit (A0, A1 -> GND) [0x40 << 1]

#define INA226_REG_CONFIG 0x00
#define INA226_REG_SHUNT_VOLTAGE 0x01
#define INA226_REG_BUS_VOLTAGE 0x02
#define INA226_REG_POWER 0x03
#define INA226_REG_CURRENT 0x04
#define INA226_REG_CALIBRATION 0x05
#define INA226_REG_MASK_ENABLE 0x06
#define INA226_REG_ALERT_LIMIT 0x07
#define INA226_REG_MANUFACTURER_ID 0xFE
#define INA226_REG_DIE_ID 0xFF

typedef struct {
    float Voltage_V;
    float Current_A;
    float Power_W;
    float ShuntVoltage_mV;
} INA226_Data_t;

uint8_t INA226_Init(SoftI2C_Handle_t *hi2c);
void INA226_SetCalibration(SoftI2C_Handle_t *hi2c, uint16_t calValue);
uint8_t INA226_Read_Data(SoftI2C_Handle_t *hi2c, INA226_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __INA226_H */
