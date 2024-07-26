#include "stm32f10x.h"
int16_t count;
void Xuanniu_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使能GPIOB时钟
    GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//设置引脚模式为上拉输入
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_13|GPIO_Pin_14;//设置引脚号为13和14
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置引脚速率为50MHz
    GPIO_Init(GPIOB,&GPIO_InitStructure);//根据参数初始化GPIOB的引脚
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//使能AFIO时钟
    GPIO_AFIODeInit();//复位AFIO寄存器
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource13);//设置GPIOB的13引脚为中断源
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource14);//设置GPIOB的14引脚为中断源
    //配置中断线路
    EXTI_InitTypeDef EXTI_InitStructure;//定义结构体
    EXTI_InitStructure.EXTI_Line=EXTI_Line13|EXTI_Line14;//设置中断线路
    EXTI_InitStructure.EXTI_LineCmd=ENABLE;//使能中断线路
    EXTI_InitStructure.EXTI_Mode=EXTI_Mode_Interrupt;//设置中断模式为中断
    EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Falling;//设置中断触发方式为下降沿触发
    EXTI_Init(&EXTI_InitStructure);//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

    //中断优先级配置
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
    NVIC_InitTypeDef NVIC_InitStructure;//定义结构体
    NVIC_InitStructure.NVIC_IRQChannel=EXTI15_10_IRQn;//设置中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//使能中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//设置抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=2;//设置响应优先级
    NVIC_Init(&NVIC_InitStructure);//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
}
//外部中断13和14的中断函数
void EXTI15_10_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line13) == SET)		//判断是否是外部中断0号线触发的中断
	{
		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0)
		{
			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0)		//PB0的下降沿触发中断，此时检测另一相PB1的电平，目的是判断旋转方向
			{
				count ++;					//此方向定义为反转，计数变量自减
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line13);			//清除外部中断0号线的中断标志位
													//中断标志位必须清除
													//否则中断将连续不断地触发，导致主程序卡死
	}
	if (EXTI_GetITStatus(EXTI_Line14) == SET)		//判断是否是外部中断1号线触发的中断
	{
		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0)
		{
			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0)		//PB1的下降沿触发中断，此时检测另一相PB0的电平，目的是判断旋转方向
			{
				count --;					//此方向定义为正转，计数变量自增
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line14);			//清除外部中断1号线的中断标志位
													//中断标志位必须清除
													//否则中断将连续不断地触发，导致主程序卡死
	}
}

//获取旋钮的计数值
int16_t Xuanniu_GetCount()
{
    int16_t count1;
    count1=count;
    count=0;
    return count1;
}
    
    