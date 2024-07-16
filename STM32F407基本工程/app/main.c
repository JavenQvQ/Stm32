/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 *  
 
 Change Logs:
 * Date           Author       Notes
 * 2024-03-08     LCKFB-LP    first version
 */
#include "board.h"
#include "bsp_uart.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "ADC.h"
#include <stdio.h>



int main(void)
{
	
	board_init();
	
	uart1_init(115200U);
	float32_t a=3;
	float32_t b=4;


	Adc_Init();
	arm_abs_f32(&a,&b,1);
	while(1)
	{
		
		
		printf("value = %d\r\n", b);
		delay_ms(1000);
	}
	

}
