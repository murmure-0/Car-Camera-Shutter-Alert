/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "protocol.h"
#include "stepper.h"
#include "bsp_adc.h"
#include "gps_module.h"
#include "ina226.h"
#include "aht30.h"
#include "mpu6050.h"
#include "soft_i2c.h"
#include "bsp_power.h"
#include "task_queue.h"
#include "key_handler.h"
#include "business_handler.h"
#include "led_handler.h"
#include "json_parser.h"
#include "usart1_handler.h"
#include "protocol.h"
#include "aht30.h"
#include "ina226.h"
#include "bsp_adc.h"
#include "stepper.h"
#include <math.h>

extern SoftI2C_Handle_t hi2c_mpu;
extern SoftI2C_Handle_t hi2c_aht;
extern SoftI2C_Handle_t hi2c_ina;

extern uint8_t g_usart2_rx_buf[];
extern volatile uint16_t g_usart2_rx_len;
extern volatile uint8_t g_usart2_data_ready;

extern uint8_t g_usart3_rx_buf[];
extern volatile uint16_t g_usart3_rx_len;
extern volatile uint8_t g_usart3_data_ready;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId gpsBridgeTaskHandle;
osThreadId ledBlinkTaskHandle;
osThreadId keyScanTaskHandle;
volatile uint32_t g_pc1_adc = 0;
volatile float g_gps_lat = 0.0f;
volatile float g_gps_lon = 0.0f;
volatile uint8_t g_gps_sat = 0;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartGpsBridgeTask(void const * argument);
void StartLedBlinkTask(void const * argument);
void StartKeyScanTask(void const * argument);
void JsonParser_Task(void const * argument);
void Usart1Handler_Task(void const * argument);
void SensorReport_Task(void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  TaskQueue_Init();
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(gpsBridgeTask, StartGpsBridgeTask, osPriorityLow, 0, 512);
  gpsBridgeTaskHandle = osThreadCreate(osThread(gpsBridgeTask), NULL);
  if (gpsBridgeTaskHandle == NULL) {
  }
  osThreadDef(ledBlinkTask, StartLedBlinkTask, osPriorityNormal, 0, 512);
  ledBlinkTaskHandle = osThreadCreate(osThread(ledBlinkTask), NULL);
  if (ledBlinkTaskHandle == NULL) {
  }
  osThreadDef(keyScanTask, KeyHandler_ScanTask, osPriorityNormal, 0, 512);
  keyScanTaskHandle = osThreadCreate(osThread(keyScanTask), NULL);
  if (keyScanTaskHandle == NULL) {
  }
  osThreadDef(businessTask, BusinessHandler_Task, osPriorityNormal, 0, 512);
  osThreadCreate(osThread(businessTask), NULL);
  osThreadDef(ledTask, LedHandler_Task, osPriorityLow, 0, 256);
  osThreadCreate(osThread(ledTask), NULL);
  osThreadDef(jsonTask, JsonParser_Task, osPriorityNormal, 0, 512);
  osThreadCreate(osThread(jsonTask), NULL);
  osThreadDef(usart1Task, Usart1Handler_Task, osPriorityNormal, 0, 512);
  osThreadCreate(osThread(usart1Task), NULL);
  osThreadDef(sensorReportTask, SensorReport_Task, osPriorityNormal, 0, 1024);
  osThreadCreate(osThread(sensorReportTask), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartGpsBridgeTask(void const * argument)
{
  char nmea_buf[128];
  uint16_t idx = 0;
  GPS_Data_t gps_data;

  printf("GPS Bridge Task Started (DMA+IDLE)\r\n");

  for(;;)
  {
    if (g_usart2_data_ready) {
      g_usart2_data_ready = 0;

      if (g_usart2_rx_len > 0) {
        /* 处理接收到的数据 */
        for (uint16_t i = 0; i < g_usart2_rx_len; i++) {
          char c = (char)g_usart2_rx_buf[i];
          if (c == '\n' || c == '\r') {
            if (idx > 0) {
              nmea_buf[idx] = '\0';
              if (GPS_ParseGNGGA(nmea_buf, &gps_data) == 0) {
                taskENTER_CRITICAL();
                g_gps_lat = gps_data.Latitude;
                g_gps_lon = gps_data.Longitude;
                g_gps_sat = gps_data.Satellites;
                taskEXIT_CRITICAL();
                
                /* 打印GPS数据到串口 */
                printf("GPS_DATA: lat=%.6f, lon=%.6f, sat=%d\r\n", 
                       gps_data.Latitude, gps_data.Longitude, gps_data.Satellites);
              }
              idx = 0;
            }
          } else {
            if (idx < (sizeof(nmea_buf) - 1)) {
              nmea_buf[idx++] = c;
            } else {
              idx = 0;
            }
          }
        }
      }
    }

    osDelay(10);
  }
}

/**
 * LED 闪烁任务（主功能描述）
 * @param {void const *} argument - 任务参数（未使用）
 * @returns {void} 无返回值
 */
void StartLedBlinkTask(void const * argument)
{
  uint8_t led_on = 0;

  for(;;)
  {
    led_on = !led_on;
    HAL_GPIO_WritePin(GPIOB, LED_OUT1_Pin|LED_OUT2_Pin|LED_OUT3_Pin|LED_OUT4_Pin, led_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
    osDelay(500);
  }
}

/**
 * LED处理任务
 * @param argument 任务参数（未使用）
 */
void LedHandler_Task(void const * argument)
{
    extern volatile float g_gps_lat;
    extern volatile float g_gps_lon;
    extern volatile uint8_t g_gps_sat;
    
    TickType_t last_tick = xTaskGetTickCount();
    
    /* 初始化LED */
    LedHandler_Init();
    
    for(;;)
    {
        /* 更新系统LED（每100ms闪烁） */
        LedHandler_UpdateSystemLed();
        
        /* 每500ms更新一次GPS LED */
        TickType_t now = xTaskGetTickCount();
        if ((now - last_tick) >= 500) {
            last_tick = now;
            LedHandler_UpdateGpsLed(g_gps_sat);
        }
        
        osDelay(100);
    }
}

/**
 * 内存分配失败钩子（主功能描述）
 * @returns {void} 无返回值
 */
void vApplicationMallocFailedHook(void)
{
  HAL_GPIO_WritePin(GPIOB, LED_OUT1_Pin|LED_OUT2_Pin|LED_OUT3_Pin|LED_OUT4_Pin, GPIO_PIN_RESET);
  printf("MallocFailedHook\r\n");
  for(;;)
  {
  }
}

/**
 * 栈溢出钩子（主功能描述）
 * @param {TaskHandle_t} xTask - 发生溢出的任务句柄
 * @param {char *} pcTaskName - 发生溢出的任务名
 * @returns {void} 无返回值
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  (void)pcTaskName;
  HAL_GPIO_WritePin(GPIOB, LED_OUT1_Pin|LED_OUT2_Pin|LED_OUT3_Pin|LED_OUT4_Pin, GPIO_PIN_RESET);
  printf("StackOverflowHook\r\n");
  for(;;)
  {
  }
}

/* 姿态解算变量 - 互补滤波 */
static float roll_angle = 0.0f;
static float pitch_angle = 0.0f;
static float yaw_angle = 0.0f;

/**
 * @brief 使用互补滤波计算姿态角
 * @param ax, ay, az 加速度 (g)
 * @param gx, gy, gz 角速度 (°/s)
 * @param dt 时间间隔 (s)
 * @param roll, pitch, yaw 输出姿态角
 * 
 * 原理：
 * - 加速度计：测量重力方向，静态准确，动态受干扰
 * - 陀螺仪：测量旋转角速度，积分得角度，但会漂移
 * - 互补滤波：高频用陀螺仪，低频用加速度计
 */
static void CalculateAttitude(float ax, float ay, float az,
                               float gx, float gy, float gz,
                               float dt,
                               float *roll, float *pitch, float *yaw)
{
    /* 1. 加速度计计算姿态角（只在静态时准确）
     * 
     * 当设备静止时，加速度计只测到重力加速度(0,0,1g)
     * 倾斜时，重力分量分布在三个轴上
     * 
     * roll:  绕X轴旋转，由Y和Z轴重力分量决定
     * pitch: 绕Y轴旋转，由X和Z轴重力分量决定
     */
    float acc_pitch = atan2f(ay, az) * 57.29578f;  /* rad to deg */
    float acc_roll = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.29578f;
    
    /* 2. 互补滤波系数
     * 
     * tau = 时间常数，决定信任陀螺仪还是加速度计
     * alpha = tau / (tau + dt)
     * 
     * 如果dt=1s，tau=0.1s：
     * alpha = 0.1 / (0.1 + 1) = 0.09
     * 意味着主要信任加速度计（91%），陀螺仪只用于短期修正（9%）
     */
    const float tau = 0.1f;  /* 时间常数0.1秒 */
    float alpha = tau / (tau + dt);
    float beta = 1.0f - alpha;
    
    /* 3. 互补滤波融合
     * 
     * 新角度 = 陀螺仪积分角度 * alpha + 加速度计角度 * beta
     * 
     * 陀螺仪积分：当前角度 + 角速度 * 时间
     * 这给出了短时间内的精确变化*/
   /* 互补滤波融合
     * 
     * 注意：陀螺仪的gx对应pitch变化，gy对应roll变化
     * 当设备绕X轴旋转(roll)时，Y轴陀螺仪有输出
     * 当设备绕Y轴旋转(pitch)时，X轴陀螺仪有输出
     * 
     * roll取反：使翻滚方向与右手定则一致
     * pitch取反：使俯仰方向与右手定则一致
     */
    pitch_angle = -(alpha * (pitch_angle + gx * dt) + beta * acc_pitch);
    roll_angle = -(alpha * (roll_angle + gy * dt) + beta * acc_roll);
    
    /* 4. 偏航角（Yaw）
     * 
     * 偏航角是绕Z轴旋转，重力方向不变，所以加速度计无法测量
     * 只能靠陀螺仪积分，会随时间漂移
     * 如果需要准确偏航角，需要磁力计（电子罗盘）
     */
    yaw_angle = yaw_angle + gz * dt;
    
    /* 限制角度范围在 -180 ~ +180 度 */
    while (roll_angle > 180.0f) roll_angle -= 360.0f;
    while (roll_angle < -180.0f) roll_angle += 360.0f;
    while (pitch_angle > 180.0f) pitch_angle -= 360.0f;
    while (pitch_angle < -180.0f) pitch_angle += 360.0f;
    while (yaw_angle > 180.0f) yaw_angle -= 360.0f;
    while (yaw_angle < -180.0f) yaw_angle += 360.0f;
    
    /* 输出 */
    *roll = roll_angle;
    *pitch = pitch_angle;
    *yaw = yaw_angle;
}

/**
 * @brief 传感器数据上报任务
 * @details 每秒采集一次所有传感器数据，并以JSON格式发送到串口3
 * @param argument 任务参数（未使用）
 */
void SensorReport_Task(void const * argument)
{
    Sensor_Report_t report;
    MPU6050_Data_t mpu_data;
    AHT30_Data_t aht_data;
    INA226_Data_t ina_data;
    uint32_t last_sensor_read = 0;
    uint32_t last_json_send = 0;
    uint32_t current_time;
    
    printf("Sensor Report Task Started (1Hz to USART3)\r\n");
    
    /* 初始化时间 */
    last_sensor_read = HAL_GetTick();
    
    for(;;)
    {
        current_time = HAL_GetTick();
        
        /* 每100ms读取一次MPU6050并进行姿态解算（10Hz）
         * 这样陀螺仪积分的时间间隔短，误差小
         */
        if ((current_time - last_sensor_read) >= 100) {
            float dt = (current_time - last_sensor_read) / 1000.0f;  /* ms to s */
            last_sensor_read = current_time;
            
            /* 读取MPU6050数据 */
            MPU6050_Read_All(&hi2c_mpu, &mpu_data);
            report.acc_x = mpu_data.Ax;
            report.acc_y = mpu_data.Ay;
            report.acc_z = mpu_data.Az;
            report.gyro_x = mpu_data.Gx;
            report.gyro_y = mpu_data.Gy;
            report.gyro_z = mpu_data.Gz;
            report.mpu_temp = mpu_data.Temperature;
            
            /* 姿态解算（10Hz更新） */
            CalculateAttitude(mpu_data.Ax, mpu_data.Ay, mpu_data.Az,
                              mpu_data.Gx, mpu_data.Gy, mpu_data.Gz,
                              dt,
                              &report.roll, &report.pitch, &report.yaw);
        }
        
        /* 每1000ms发送一次JSON数据（1Hz） */
        if ((current_time - last_json_send) >= 1000) {
            last_json_send = current_time;
            
            /* 读取其他传感器数据 */
            AHT30_Read_Data(&hi2c_aht, &aht_data);
            report.temp_c = aht_data.Temperature;
            report.humi_pct = aht_data.Humidity;
            
            INA226_Read_Data(&hi2c_ina, &ina_data);
            report.bus_voltage_v = ina_data.Voltage_V;
            report.shunt_voltage_mv = ina_data.ShuntVoltage_mV;
            report.current_a = ina_data.Current_A;
            report.power_w = ina_data.Power_W;
            
            report.battery_v = BSP_GetBatteryVoltage();
            report.hall1_val = BSP_GetHallSensor1();
            report.hall2_val = BSP_GetHallSensor2();
            
            taskENTER_CRITICAL();
            report.latitude = g_gps_lat;
            report.longitude = g_gps_lon;
            report.satellites = g_gps_sat;
            taskEXIT_CRITICAL();
            
            report.stepper_angle = Stepper_GetAngle();
            
            /* 发送JSON数据到串口3 */
            Protocol_SendSensorData(&report);
        }
        
        osDelay(10);  /* 10ms循环一次 */
    }
}

/**
 * FreeRTOS 断言钩子（主功能描述）
 * @param {const char *} file - 触发断言的文件名
 * @param {int} line - 触发断言的行号
 * @returns {void} 无返回值
 */
void vAssertCalled(const char *file, int line)
{
  (void)file;
  (void)line;
  HAL_GPIO_WritePin(GPIOB, LED_OUT1_Pin|LED_OUT2_Pin|LED_OUT3_Pin|LED_OUT4_Pin, GPIO_PIN_RESET);
  printf("FreeRTOS ASSERT\r\n");
}

/* USER CODE END Application */
