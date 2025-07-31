/**************************************************************************
	使用 MCO 输出作为 AD9220 的时钟 -> PA8
	
	注：AD9220 的数据输出
		1. 时钟低电平期间，数据稳定
		2. 时钟高电平期间，数据可变
	在外部中断中读取 12-bit 数据，使用下降沿中断
**************************************************************************/
#include "ad9220.h"

// 添加中断标志变量
volatile uint8_t KEY0_interrupt_flag = 0;
volatile uint8_t KEY1_interrupt_flag = 0;
volatile uint8_t KEY2_interrupt_flag = 0;
volatile uint8_t Key_WakeUp_interrupt_flag = 0;

// GPIO和外部中断初始化函数
void EXTI_Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能GPIO时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE);
    // 使能SYSCFG时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    
    // 配置PE2为上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    
    // 配置PE3为上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    
    // 配置PE4为上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    
    // 配置PA0为下拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置外部中断线
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource2);  // PE2 -> EXTI2
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource3);  // PE3 -> EXTI3
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource4);  // PE4 -> EXTI4
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);  // PA0 -> EXTI0
    
    // 配置EXTI2 (PE2) - 下降沿触发（上拉输入，按下时为低电平）
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置EXTI3 (PE3) - 下降沿触发
    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置EXTI4 (PE4) - 下降沿触发
    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置EXTI0 (PA0) - 上升沿触发（下拉输入，按下时为高电平）
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置NVIC中断优先级
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    
    // EXTI0中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    
    // EXTI2中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    
    // EXTI3中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    
    // EXTI4中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_Init(&NVIC_InitStructure);
}

// wake up key中断处理函数
void EXTI0_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        delay_ms(10); // 消抖
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == Bit_SET)
        {
            Key_WakeUp_interrupt_flag = 1; // 
            printf("WakeUp KEY pressed (PA0)\r\n");
        }
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

//key2
void EXTI2_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line2) != RESET)
    {
        // 简单消抖延时
        delay_ms(10);
        if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) == Bit_RESET)
        {
            KEY2_interrupt_flag = 1;
        }
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
}

//key1
void EXTI3_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line3) != RESET)
    {
        // 简单消抖延时
        delay_ms(10);
        if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == Bit_RESET)
        {
			KEY1_interrupt_flag = 1;
        }
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}


//key0
void EXTI4_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        // 简单消抖延时
        delay_ms(10);
        if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == Bit_RESET)
        {
			KEY0_interrupt_flag = 1;
        }
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

// 中断标志处理函数
void EXTI_Flag_Handle(void)
{


}