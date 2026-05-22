/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RV1106_POWER_EN_Pin GPIO_PIN_13
#define RV1106_POWER_EN_GPIO_Port GPIOC
#define IN_HALL_ADC1_Pin GPIO_PIN_0
#define IN_HALL_ADC1_GPIO_Port GPIOC
#define IN_HALL_ADC2_Pin GPIO_PIN_1
#define IN_HALL_ADC2_GPIO_Port GPIOC
#define BAT_ADC_EN_Pin GPIO_PIN_3
#define BAT_ADC_EN_GPIO_Port GPIOC
#define BOOST_EN_Pin GPIO_PIN_1
#define BOOST_EN_GPIO_Port GPIOA
#define GPS_TX_Pin GPIO_PIN_2
#define GPS_TX_GPIO_Port GPIOA
#define GPS_RX_Pin GPIO_PIN_3
#define GPS_RX_GPIO_Port GPIOA
#define GPS_WAKE_Pin GPIO_PIN_6
#define GPS_WAKE_GPIO_Port GPIOA
#define STM32_TX_Pin GPIO_PIN_4
#define STM32_TX_GPIO_Port GPIOC
#define STM32_RX_Pin GPIO_PIN_5
#define STM32_RX_GPIO_Port GPIOC
#define MPU6050_SDA_Pin GPIO_PIN_10
#define MPU6050_SDA_GPIO_Port GPIOB
#define MPU6050_SCL_Pin GPIO_PIN_11
#define MPU6050_SCL_GPIO_Port GPIOB
#define AHT30_SDA_Pin GPIO_PIN_15
#define AHT30_SDA_GPIO_Port GPIOA
#define AHT30_SCL_Pin GPIO_PIN_10
#define AHT30_SCL_GPIO_Port GPIOC
#define INA226_SCL_Pin GPIO_PIN_11
#define INA226_SCL_GPIO_Port GPIOC
#define INA226_SDA_Pin GPIO_PIN_12
#define INA226_SDA_GPIO_Port GPIOC
#define STEP_D0_Pin GPIO_PIN_2
#define STEP_D0_GPIO_Port GPIOD
#define STEP_D1_Pin GPIO_PIN_3
#define STEP_D1_GPIO_Port GPIOB
#define STEP_D2_Pin GPIO_PIN_4
#define STEP_D2_GPIO_Port GPIOB
#define STEP_D3_Pin GPIO_PIN_5
#define STEP_D3_GPIO_Port GPIOB
#define LED_OUT1_Pin GPIO_PIN_6
#define LED_OUT1_GPIO_Port GPIOB
#define LED_OUT2_Pin GPIO_PIN_7
#define LED_OUT2_GPIO_Port GPIOB
#define LED_OUT3_Pin GPIO_PIN_8
#define LED_OUT3_GPIO_Port GPIOB
#define LED_OUT4_Pin GPIO_PIN_9
#define LED_OUT4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
