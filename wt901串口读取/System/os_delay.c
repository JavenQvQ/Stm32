/***************************************************************************
********************前后系统延迟函数的实现**********************************
****************************************************************************
实现了2个延迟函数：delay_us,delay_ms
这个函数时基采用的是10MS，因此，程序里delay_us能够延迟的时间：0 < us < 10000
如果使用1ms时基，那么请修改宏TIM_BASE
B站：奇遇单片机
作者：龙虾哥
****************************************************************************/

#include "stm32f10x.h"                  // Device header

#define fus 72  //每微妙,时钟个数，这里时钟是72MHZ
#define TIM_BASE 1000
void delay_init(void)
{
	SysTick_Config(720000);
}
void delay_us(uint32_t us)//0 < us < 10000
{
	volatile	uint32_t ticks = 0,current_value = 0,temp = 0,cnt= 0;
      
	if((us >= TIM_BASE)||(us == 0))return;
	
	current_value = SysTick->VAL;
	
	ticks = us*fus-1;
	
	if(ticks <= current_value)
	{
		cnt = SysTick->VAL;
		temp = current_value - ticks;
		while((cnt>=temp)&&(cnt<current_value))
		{
			cnt = SysTick->VAL;
		}
	}
	else
	{	
		cnt = SysTick->VAL;
		temp = ticks - current_value;
		while((cnt>SysTick->LOAD-temp)||(cnt<=current_value))
		{
			cnt = SysTick->VAL;
		}
	}
}
void delay_ms(uint32_t ms)
{
	for(uint32_t i = 0;i < ms;i++)
	{
		delay_us(1000);
	}
}

extern void TASK_OS(void);

/*
void SysTick_Handler(void)
{
	TASK_OS();
}
*/
