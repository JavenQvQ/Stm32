/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：club.szlcsc.com
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * 
 
 Change Logs:
 * Date           Author       Notes
 * 2024-03-07     LCKFB-LP    first version
 */
 
 #ifndef __BSP_UART_H__
 #define __BSP_UART_H__
 
 #include "stm32f4xx.h"
 
 
 
 //外部可调用函数的声明
void uart1_init(uint32_t __Baud);
void USART1_IRQHandler(void);
 
 
 
 
 
 
 
#endif
