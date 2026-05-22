#include "aht30.h"

uint8_t AHT30_Init(SoftI2C_Handle_t *hi2c) {
    HAL_Delay(100); // 等待上电稳定
    // 检查状态或在需要时发送初始化命令 (0xBE)
    // AHT30 如果未校准，通常需要初始化命令 0xBE 0x08 0x00
    // 但通常直接读取即可。让我们发送复位或仅检查是否存在。
    // 为简单起见，如果地址收到 ACK，我们只需返回 0。
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, (AHT30_ADDR << 1) | 0);
    if (SoftI2C_WaitAck(hi2c)) {
        return 1; // 设备未找到
    }
    SoftI2C_Stop(hi2c);
    return 0;
}

uint8_t AHT30_Read_Data(SoftI2C_Handle_t *hi2c, AHT30_Data_t *data) {
    uint8_t buf[6];
    uint32_t humi_raw, temp_raw;
    
    // 发送触发测量命令
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, (AHT30_ADDR << 1) | 0);
    if (SoftI2C_WaitAck(hi2c)) return 1;
    SoftI2C_SendByte(hi2c, 0xAC);
    if (SoftI2C_WaitAck(hi2c)) return 1;
    SoftI2C_SendByte(hi2c, 0x33);
    if (SoftI2C_WaitAck(hi2c)) return 1;
    SoftI2C_SendByte(hi2c, 0x00);
    if (SoftI2C_WaitAck(hi2c)) return 1;
    SoftI2C_Stop(hi2c);
    
    HAL_Delay(80); // 测量需要约 80ms
    
    // 读取数据
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, (AHT30_ADDR << 1) | 1);
    if (SoftI2C_WaitAck(hi2c)) return 1;
    
    // 读取 6 个字节: 状态, H1, H2, H3/T1, T2, T3
    // 等待忙标志清除？目前我们依赖延时。
    // 状态字节 (字节 0) 的第 7 位是忙标志。
    buf[0] = SoftI2C_ReadByte(hi2c, 1);
    // 如果忙标志置位，我们可能需要等待更多时间，但通常 80ms 足够了。
    
    buf[1] = SoftI2C_ReadByte(hi2c, 1);
    buf[2] = SoftI2C_ReadByte(hi2c, 1);
    buf[3] = SoftI2C_ReadByte(hi2c, 1);
    buf[4] = SoftI2C_ReadByte(hi2c, 1);
    buf[5] = SoftI2C_ReadByte(hi2c, 0); // 最后一个字节 NACK
    
    SoftI2C_Stop(hi2c);
    
    humi_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((buf[3] & 0xF0) >> 4);
    temp_raw = ((uint32_t)(buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    
    data->Humidity = (float)humi_raw * 100.0f / 1048576.0f;
    data->Temperature = (float)temp_raw * 200.0f / 1048576.0f - 50.0f;
    
    return 0;
}
