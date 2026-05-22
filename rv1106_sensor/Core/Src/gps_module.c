#include "gps_module.h"

// Helper to convert NMEA Lat/Lon (ddmm.mmmm) to Decimal Degrees (dd.dddd)
static float Convert_NMEA_To_Decimal(float nmea_val) {
    int degrees = (int)(nmea_val / 100.0f);
    float minutes = nmea_val - (degrees * 100.0f);
    return degrees + (minutes / 60.0f);
}

void GPS_Init(void) {
    // Note: GPS Power is now controlled via BOOST_EN (Global Sensor Power)
    // Ensure BSP_Power_SetSensorPower(1) is called before using GPS.
    
    // Wake up sequence if needed (Pulse on WAKE pin?)
    // Assuming WAKE high = Active or Pulse to wake.
    HAL_GPIO_WritePin(GPS_WAKE_GPIO_Port, GPS_WAKE_Pin, GPIO_PIN_SET);
}

void GPS_WakeUp(void) {
    HAL_GPIO_WritePin(GPS_WAKE_GPIO_Port, GPS_WAKE_Pin, GPIO_PIN_SET);
}

void GPS_Sleep(void) {
    HAL_GPIO_WritePin(GPS_WAKE_GPIO_Port, GPS_WAKE_Pin, GPIO_PIN_RESET);
}

uint8_t GPS_ParseGNGGA(char *nmea_sentence, GPS_Data_t *data) {
    // Check checksum?
    // Format: $GNGGA,hhmmss.ss,llll.ll,a,yyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,,*hh
    
    char *p = strstr(nmea_sentence, "$GNGGA");
    if (!p) return 1;

    // Use strtok or simple parsing
    // Token 0: $GNGGA
    // Token 1: Time (hhmmss.ss)
    // Token 2: Lat
    // Token 3: N/S
    // Token 4: Lon
    // Token 5: E/W
    // Token 6: Fix Quality
    // Token 7: Satellites
    // Token 8: HDOP
    // Token 9: Altitude
    
    char *token;
    char buffer[100]; // Copy to avoid modifying original or just to be safe
    strncpy(buffer, p, 99);
    buffer[99] = '\0';
    
    token = strtok(buffer, ","); // $GNGGA
    
    // Time
    token = strtok(NULL, ",");
    if (token && strlen(token) >= 6) {
        char temp[3];
        temp[2] = 0;
        
        memcpy(temp, token, 2);
        data->Hour = atoi(temp);
        
        memcpy(temp, token+2, 2);
        data->Minute = atoi(temp);
        
        memcpy(temp, token+4, 2);
        data->Second = atoi(temp);
    }
    
    // 纬度
    token = strtok(NULL, ",");
    float lat_raw = (token) ? atof(token) : 0.0f;
    
    // 北/南
    token = strtok(NULL, ",");
    char ns = (token) ? token[0] : 'N';
    
    // 经度
    token = strtok(NULL, ",");
    float lon_raw = (token) ? atof(token) : 0.0f;
    
    // 东/西
    token = strtok(NULL, ",");
    char ew = (token) ? token[0] : 'E';
    
    data->Latitude = Convert_NMEA_To_Decimal(lat_raw);
    if (ns == 'S') data->Latitude = -data->Latitude;
    
    data->Longitude = Convert_NMEA_To_Decimal(lon_raw);
    if (ew == 'W') data->Longitude = -data->Longitude;
    
    // 定位质量
    token = strtok(NULL, ",");
    data->FixQuality = (token) ? atoi(token) : 0;
    
    // 卫星数
    token = strtok(NULL, ",");
    data->Satellites = (token) ? atoi(token) : 0;
    
    // 水平精度因子 (跳过)
    token = strtok(NULL, ",");
    
    // 海拔
    token = strtok(NULL, ",");
    data->Altitude = (token) ? atof(token) : 0.0f;
    
    return 0;
}
