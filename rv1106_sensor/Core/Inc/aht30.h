#ifndef __AHT30_H
#define __AHT30_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "soft_i2c.h"

#define AHT30_ADDR 0x38

typedef struct {
    float Temperature;
    float Humidity;
} AHT30_Data_t;

uint8_t AHT30_Init(SoftI2C_Handle_t *hi2c);
uint8_t AHT30_Read_Data(SoftI2C_Handle_t *hi2c, AHT30_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __AHT30_H */
