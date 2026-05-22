#include "usart1_handler.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"

extern uint8_t g_usart1_rx_buf[];
extern volatile uint16_t g_usart1_rx_len;
extern volatile uint8_t g_usart1_data_ready;

/**
 * @brief 串口1接收任务（空闲中断方式）
 * @param argument 任务参数（未使用）
 * @details 等待空闲中断标志，收到数据后统一打印
 */
void Usart1Handler_Task(void const *argument)
{
    printf("USART1 Handler Task Started (IDLE Interrupt)\r\n");
    
    for(;;)
    {
        /* 等待数据就绪 */
        if (g_usart1_data_ready) {
            g_usart1_data_ready = 0;
            
            if (g_usart1_rx_len > 0) {
                /* 添加字符串结束符 */
                if (g_usart1_rx_len < 256) {
                    g_usart1_rx_buf[g_usart1_rx_len] = '\0';
                }
                
                /* 打印接收到的数据 */
                printf("%s", (char*)g_usart1_rx_buf);
            }
        }
        
        osDelay(10);
    }
}
