#include "stm32f10x.h" // Device header
#include "delay.h"
void Key_Init()//按键初始化
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_b;
	GPIO_b.GPIO_Mode=GPIO_Mode_IPU;//上拉输入
	GPIO_b.GPIO_Pin=GPIO_Pin_12|GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13;
	GPIO_b.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_b);
}
uint8_t Key_GetNum()
{
	uint8_t Num;
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==0)
	{
		Delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==0)
		{
		}
		Delay_ms(20);//消除抖动
		Num=3;
	}
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15)==0)
	{
		Delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15)==0)
		{
		}
		Delay_ms(20);//消除抖动
		Num=4;
	}
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)==0)
	{
		Delay_ms(25);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)==0)
		{
		}
		Delay_ms(20);//消除抖动
		Num=1;
	}
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)==0)
	{
		Delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)==0)
		{
		}
		Delay_ms(20);//消除抖动
		Num=2;
	}
	return Num;
}
