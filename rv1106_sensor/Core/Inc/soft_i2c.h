#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef struct {
    GPIO_TypeDef *SCL_Port;
    uint16_t SCL_Pin;
    GPIO_TypeDef *SDA_Port;
    uint16_t SDA_Pin;
} SoftI2C_Handle_t;

void SoftI2C_Init(SoftI2C_Handle_t *hi2c);
void SoftI2C_Start(SoftI2C_Handle_t *hi2c);
void SoftI2C_Stop(SoftI2C_Handle_t *hi2c);
void SoftI2C_SendByte(SoftI2C_Handle_t *hi2c, uint8_t byte);
uint8_t SoftI2C_ReadByte(SoftI2C_Handle_t *hi2c, uint8_t ack);
uint8_t SoftI2C_WaitAck(SoftI2C_Handle_t *hi2c);
void SoftI2C_Ack(SoftI2C_Handle_t *hi2c);
void SoftI2C_NAck(SoftI2C_Handle_t *hi2c);

/* Helper functions for device register access */
HAL_StatusTypeDef SoftI2C_WriteReg(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t data);
HAL_StatusTypeDef SoftI2C_ReadReg(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t *pData);
HAL_StatusTypeDef SoftI2C_WriteBuffer(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t *pData, uint16_t size);
HAL_StatusTypeDef SoftI2C_ReadBuffer(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t *pData, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* __SOFT_I2C_H */
