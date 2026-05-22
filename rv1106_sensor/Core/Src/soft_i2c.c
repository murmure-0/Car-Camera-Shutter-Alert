#include "soft_i2c.h"

/** 
 * 软件 I2C：通过 GPIO 模拟 SCL/SDA 时序
 */
#define I2C_DELAY_US 5 // 根据MCU速度调整，大约 100kHz-400kHz

/**
 * 时序延时，影响 I2C 速度
 * @returns {void} 无返回
 */
static void I2C_Delay(void) {
    // 简单延时循环 - 根据您的时钟速度校准
    // 对于 80MHz STM32L4，几个循环就足够了。
    // 这是一个粗略的延时。
    volatile int i = 20; 
    while (i--);
}

/**
 * 将 SDA 配置为输入，用于读取 ACK 或数据
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
static void SDA_Input(SoftI2C_Handle_t *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hi2c->SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP; // 读取时使能上拉
    HAL_GPIO_Init(hi2c->SDA_Port, &GPIO_InitStruct);
}

/**
 * 将 SDA 配置为推挽输出，用于发送数据/时序
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
static void SDA_Output(SoftI2C_Handle_t *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hi2c->SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(hi2c->SDA_Port, &GPIO_InitStruct);
}

/**
 * SDA 置高
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
static void SDA_High(SoftI2C_Handle_t *hi2c) {
    HAL_GPIO_WritePin(hi2c->SDA_Port, hi2c->SDA_Pin, GPIO_PIN_SET);
}

/**
 * SDA 置低
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
static void SDA_Low(SoftI2C_Handle_t *hi2c) {
    HAL_GPIO_WritePin(hi2c->SDA_Port, hi2c->SDA_Pin, GPIO_PIN_RESET);
}

/**
 * 读取 SDA 电平
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {uint8_t} SDA 电平（0/1）
 */
static uint8_t SDA_Read(SoftI2C_Handle_t *hi2c) {
    return (uint8_t)HAL_GPIO_ReadPin(hi2c->SDA_Port, hi2c->SDA_Pin);
}

/**
 * SCL 置高
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
static void SCL_High(SoftI2C_Handle_t *hi2c) {
    HAL_GPIO_WritePin(hi2c->SCL_Port, hi2c->SCL_Pin, GPIO_PIN_SET);
}

/**
 * SCL 置低
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
static void SCL_Low(SoftI2C_Handle_t *hi2c) {
    HAL_GPIO_WritePin(hi2c->SCL_Port, hi2c->SCL_Pin, GPIO_PIN_RESET);
}

/**
 * 初始化软 I2C 引脚状态
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
void SoftI2C_Init(SoftI2C_Handle_t *hi2c) {
    // 假设引脚时钟已使能
    // 配置 SCL 为推挽输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 初始化 SCL
    GPIO_InitStruct.Pin = hi2c->SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(hi2c->SCL_Port, &GPIO_InitStruct);
    
    // 初始化 SDA
    SDA_Output(hi2c);
    
    // 空闲状态
    SCL_High(hi2c);
    SDA_High(hi2c);
}

/**
 * 发送 I2C 起始信号
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
void SoftI2C_Start(SoftI2C_Handle_t *hi2c) {
    SDA_Output(hi2c);
    SDA_High(hi2c);
    SCL_High(hi2c);
    I2C_Delay();
    SDA_Low(hi2c);
    I2C_Delay();
    SCL_Low(hi2c);
}

/**
 * 发送 I2C 停止信号
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
void SoftI2C_Stop(SoftI2C_Handle_t *hi2c) {
    SDA_Output(hi2c);
    SCL_Low(hi2c);
    SDA_Low(hi2c);
    I2C_Delay();
    SCL_High(hi2c);
    I2C_Delay();
    SDA_High(hi2c);
    I2C_Delay();
}

/**
 * 等待从机 ACK，超时返回 1
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {uint8_t} 0=收到ACK，1=超时
 */
uint8_t SoftI2C_WaitAck(SoftI2C_Handle_t *hi2c) {
    uint8_t ucErrTime = 0;
    SDA_Input(hi2c); 
    SCL_High(hi2c);
    I2C_Delay();
    while (SDA_Read(hi2c)) {
        ucErrTime++;
        if (ucErrTime > 250) {
            SoftI2C_Stop(hi2c);
            return 1;
        }
    }
    SCL_Low(hi2c);
    return 0;
}

/**
 * 发送 ACK
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
void SoftI2C_Ack(SoftI2C_Handle_t *hi2c) {
    SCL_Low(hi2c);
    SDA_Output(hi2c);
    SDA_Low(hi2c);
    I2C_Delay();
    SCL_High(hi2c);
    I2C_Delay();
    SCL_Low(hi2c);
}

/**
 * 发送 NACK
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @returns {void} 无返回
 */
void SoftI2C_NAck(SoftI2C_Handle_t *hi2c) {
    SCL_Low(hi2c);
    SDA_Output(hi2c);
    SDA_High(hi2c);
    I2C_Delay();
    SCL_High(hi2c);
    I2C_Delay();
    SCL_Low(hi2c);
}

/**
 * 发送 1 字节
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @param {uint8_t} byte - 要发送的数据
 * @returns {void} 无返回
 */
void SoftI2C_SendByte(SoftI2C_Handle_t *hi2c, uint8_t byte) {
    uint8_t i;
    SDA_Output(hi2c);
    SCL_Low(hi2c);
    for (i = 0; i < 8; i++) {
        if (byte & 0x80)
            SDA_High(hi2c);
        else
            SDA_Low(hi2c);
        byte <<= 1;
        I2C_Delay();
        SCL_High(hi2c);
        I2C_Delay();
        SCL_Low(hi2c);
        I2C_Delay();
    }
}

/**
 * 读取 1 字节
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @param {uint8_t} ack - 1=发送ACK，0=发送NACK
 * @returns {uint8_t} 读取到的数据
 */
uint8_t SoftI2C_ReadByte(SoftI2C_Handle_t *hi2c, uint8_t ack) {
    uint8_t i, receive = 0;
    SDA_Input(hi2c);
    for (i = 0; i < 8; i++) {
        SCL_Low(hi2c);
        I2C_Delay();
        SCL_High(hi2c);
        receive <<= 1;
        if (SDA_Read(hi2c)) receive++;
        I2C_Delay();
    }
    if (!ack)
        SoftI2C_NAck(hi2c);
    else
        SoftI2C_Ack(hi2c);
    return receive;
}

/**
 * 写 1 字节到指定寄存器
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @param {uint8_t} devAddr - 设备地址（含R/W位前的7位地址左移）
 * @param {uint8_t} regAddr - 寄存器地址
 * @param {uint8_t} data - 要写入的数据
 * @returns {HAL_StatusTypeDef} HAL_OK/HAL_ERROR
 */
HAL_StatusTypeDef SoftI2C_WriteReg(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t data) {
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, devAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    SoftI2C_SendByte(hi2c, regAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    SoftI2C_SendByte(hi2c, data);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    SoftI2C_Stop(hi2c);
    return HAL_OK;
}

/**
 * 从指定寄存器读取 1 字节
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @param {uint8_t} devAddr - 设备地址（含R/W位前的7位地址左移）
 * @param {uint8_t} regAddr - 寄存器地址
 * @param {uint8_t*} pData - 读取结果缓冲区
 * @returns {HAL_StatusTypeDef} HAL_OK/HAL_ERROR
 */
HAL_StatusTypeDef SoftI2C_ReadReg(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t *pData) {
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, devAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    SoftI2C_SendByte(hi2c, regAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, devAddr | 1); // Read mode
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    *pData = SoftI2C_ReadByte(hi2c, 0); // NACK
    SoftI2C_Stop(hi2c);
    return HAL_OK;
}

/**
 * 连续写入多个字节
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @param {uint8_t} devAddr - 设备地址（含R/W位前的7位地址左移）
 * @param {uint8_t} regAddr - 起始寄存器地址
 * @param {uint8_t*} pData - 写入数据缓冲区
 * @param {uint16_t} size - 写入字节数
 * @returns {HAL_StatusTypeDef} HAL_OK/HAL_ERROR
 */
HAL_StatusTypeDef SoftI2C_WriteBuffer(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t *pData, uint16_t size) {
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, devAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    SoftI2C_SendByte(hi2c, regAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    for (uint16_t i = 0; i < size; i++) {
        SoftI2C_SendByte(hi2c, pData[i]);
        if (SoftI2C_WaitAck(hi2c)) {
            return HAL_ERROR;
        }
    }
    SoftI2C_Stop(hi2c);
    return HAL_OK;
}

/**
 * 连续读取多个字节
 * @param {SoftI2C_Handle_t*} hi2c - 软 I2C 句柄
 * @param {uint8_t} devAddr - 设备地址（含R/W位前的7位地址左移）
 * @param {uint8_t} regAddr - 起始寄存器地址
 * @param {uint8_t*} pData - 读取数据缓冲区
 * @param {uint16_t} size - 读取字节数
 * @returns {HAL_StatusTypeDef} HAL_OK/HAL_ERROR
 */
HAL_StatusTypeDef SoftI2C_ReadBuffer(SoftI2C_Handle_t *hi2c, uint8_t devAddr, uint8_t regAddr, uint8_t *pData, uint16_t size) {
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, devAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    SoftI2C_SendByte(hi2c, regAddr);
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    
    SoftI2C_Start(hi2c);
    SoftI2C_SendByte(hi2c, devAddr | 1); // Read mode
    if (SoftI2C_WaitAck(hi2c)) {
        return HAL_ERROR;
    }
    for (uint16_t i = 0; i < size; i++) {
        pData[i] = SoftI2C_ReadByte(hi2c, (i < size - 1) ? 1 : 0);
    }
    SoftI2C_Stop(hi2c);
    return HAL_OK;
}
