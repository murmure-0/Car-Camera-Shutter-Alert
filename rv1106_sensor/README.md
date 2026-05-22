# STM32 Sensor Hub Project

This project implements drivers for various sensors and modules connected to an STM32 microcontroller (STM32L4 series).

## Hardware Configuration

### Pin Definitions

| Module | Pin Name | GPIO | Description |
| :--- | :--- | :--- | :--- |
| **Power Control** | BOOST_EN | PA1 | Total Sensor Power Enable (Active High) |
| | RV1106_POWER_EN | PC13 | RV1106 Core Board Power Enable (Active High) |
| **Hall Sensor** | IN_HALL_ADC1 | PC0 | Analog Input 1 |
| | IN_HALL_ADC2 | PC1 | Analog Input 2 |
| **Battery** | BAT_ADC_IN | PC2 | Battery Voltage Analog Input |
| | BAT_ADC_EN | PC3 | Battery Measure Enable (Active High) |
| **GPS** | GPS_TX | PA2 | UART TX |
| | GPS_RX | PA3 | UART RX |
| | GPS_WAKE | PA6 | Wakeup / Power Control |
| **Host Comms** | STM32_TX | PC4 | UART to RV1106 |
| | STM32_RX | PC5 | UART from RV1106 |
| **MPU6050** | SDA | PB10 | I2C Data (Software) |
| | SCL | PB11 | I2C Clock (Software) |
| **AHT30** | SDA | PA15 | I2C Data (Software) |
| | SCL | PC10 | I2C Clock (Software) |
| **INA226** | SCL | PC11 | I2C Clock (Software) |
| | SDA | PC12 | I2C Data (Software) |
| **Stepper** | STEP_D0 | PD2 | Coil Driver 0 |
| | STEP_D1 | PB3 | Coil Driver 1 |
| | STEP_D2 | PB4 | Coil Driver 2 |
| | STEP_D3 | PB5 | Coil Driver 3 |

## Software Modules

### 1. Power Control (`bsp_power.h`)
Manages power rails for sensors and external modules.
- `BSP_Power_SetSensorPower(1)`: Enables `BOOST_EN` (Sensors).
- `BSP_Power_SetRV1106Power(1)`: Enables `RV1106_POWER_EN` (RV1106 Board).

### 2. Software I2C Driver (`soft_i2c.h`)
A generic bit-banging I2C driver supporting multiple instances.
- **Initialization**: `SoftI2C_Init(&handle)`
- **Usage**: Populate `SoftI2C_Handle_t` with SCL/SDA ports and pins.

### 3. MPU6050 IMU (`mpu6050.h`)
Driver for MPU6050 Accelerometer and Gyroscope.
- **Features**: Reads 6-axis data and temperature.
- **Interface**: Software I2C.

### 4. AHT30 Temperature & Humidity (`aht30.h`)
Driver for AHT30 sensor.
- **Features**: Reads relative humidity and temperature.
- **Interface**: Software I2C.

### 5. INA226 Power Monitor (`ina226.h`)
Driver for INA226 Voltage and Current sensor.
- **Features**: Reads Bus Voltage, Shunt Voltage, Current, and Power.
- **Interface**: Software I2C.
- **Note**: Calibration must be set via `INA226_SetCalibration` for correct Current/Power readings.

### 6. GPS Module (`gps_module.h`)
Driver for GPS module control and NMEA parsing.
- **Protocol**: NMEA 0183 (GNGGA sentence supported).
- **Features**: Wakeup control, Coordinate parsing.
- **Power**: Controlled via `BSP_Power_SetSensorPower`.

### 7. Stepper Motor (`stepper.h`)
Simple driver for 4-phase stepper motor.
- **Features**: Step generation with delay control.

### 8. BSP ADC (`bsp_adc.h`)
Helper functions for analog readings.
- **Features**: 
  - `BSP_GetBatteryVoltage()`: Handles Enable pin automatically.
  - `BSP_GetHallSensorX()`: Reads Hall sensor analog values.

## Usage Example

```c
#include "bsp_power.h"
#include "mpu6050.h"

SoftI2C_Handle_t hi2c_mpu = {GPIOB, GPIO_PIN_11, GPIOB, GPIO_PIN_10}; // SCL, SDA

void main() {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // 1. Turn on Sensor Power
    BSP_Power_Init();
    BSP_Power_SetSensorPower(1);
    HAL_Delay(100); // Wait for power stable
    
    // 2. Init MPU6050
    SoftI2C_Init(&hi2c_mpu);
    if (MPU6050_Init(&hi2c_mpu) == 0) {
        // Init Success
    }
    
    // 3. Turn on RV1106
    BSP_Power_SetRV1106Power(1);
    
    while(1) {
        // ...
    }
}
```
