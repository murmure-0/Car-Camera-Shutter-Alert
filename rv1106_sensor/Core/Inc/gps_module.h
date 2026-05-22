#ifndef __GPS_MODULE_H
#define __GPS_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint8_t Hour;
    uint8_t Minute;
    uint8_t Second;
    float Latitude;  // Decimal Degrees
    float Longitude; // Decimal Degrees
    uint8_t FixQuality;
    uint8_t Satellites;
    float Altitude;
} GPS_Data_t;

void GPS_Init(void);
void GPS_WakeUp(void);
void GPS_Sleep(void);
uint8_t GPS_ParseGNGGA(char *nmea_sentence, GPS_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __GPS_MODULE_H */
