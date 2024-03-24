#include "stm32f10x.h"
void IC_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能GPIOA时钟
    GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//设置引脚模式为上拉输入
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_6;//设置引脚号为6
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置引脚速率为50MHz
    GPIO_Init(GPIOA,&GPIO_InitStructure);//根据参数初始化GPIOA的引脚

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);//使能TIM3时钟
    TIM_InternalClockConfig(TIM3);//设置TIM3时钟源为内部时钟源
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;//定义结构体
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;//设置时钟分割:TDTS=TIM_CKD_DIV1
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//设置计数器模式
    TIM_TimeBaseInitStructure.TIM_Period=65536-1;//设置自动重装载寄存器周期的值ARR
    TIM_TimeBaseInitStructure.TIM_Prescaler=72-1;//设置用来作为TIMx时钟频率除数的预分频值PSC
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter=0;//设置重复计数器的值
    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//根据指定的参数初始化TIMx的时间基数单位
    //配置输入捕获模式
    TIM_ICInitTypeDef TIM_ICInitStructure;//定义结构体
    TIM_ICInitStructure.TIM_Channel=TIM_Channel_1;//选择输入捕获通道
    TIM_ICInitStructure.TIM_ICFilter=0xF;//设置输入滤波器,不滤波
    TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Rising;//上升沿触发
    TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1;//设置输入分频
    TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_DirectTI;//映射到TI1上,直连通道
    TIM_ICInit(TIM3,&TIM_ICInitStructure);//根据TIM_ICInitStruct中指定的参数初始化外设TIMx的输入捕获通道

    TIM_SelectInputTrigger(TIM3,TIM_TS_TI1FP1);//选择输入触发源
    TIM_SelectSlaveMode(TIM3,TIM_SlaveMode_Reset);//选择从模式

    TIM_Cmd(TIM3,ENABLE);//使能TIMx外设
}

uint16_t IC_GetFrequency()
{
    return 1000000/TIM_GetCapture1(TIM3);//返回捕获频率
}


