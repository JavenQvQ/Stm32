#include "stm32f10x.h"
void Timer_Init()
{
    //配置TIM2的通道1
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//使能TIMx外设时钟
    TIM_InternalClockConfig(TIM2);//设置TIM2时钟源为内部时钟源
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;//定义结构体
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1 ;//设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//设置计数器模式
    TIM_TimeBaseInitStructure.TIM_Period=10000-1;//设置自动重装载寄存器周期的值
    TIM_TimeBaseInitStructure.TIM_Prescaler=7200-1;//设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//根据指定的参数初始化TIMx的时间基数单位
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);//使能指定的TIMx中断,允许更新中断
    TIM_ClearFlag(TIM2,TIM_FLAG_Update);//清除TIMx的更新标志位

    //中断优先级配置
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
    NVIC_InitTypeDef NVIC_InitStructure;//定义结构体
    NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;//设置中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//使能中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//设置抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;//设置响应优先级
    NVIC_Init(&NVIC_InitStructure);//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
    TIM_Cmd(TIM2,ENABLE);//使能TIMx外设 
}
//tim2计时器中断函数
// void TIM2_IRQHandler(void)
// {
//     if(TIM_GetITStatus(TIM2,TIM_IT_Update)!=RESET)//检查指定的TIM中断发生与否:TIM 中断源
//     {
//         TIM_ClearITPendingBit(TIM2,TIM_IT_Update);//清除TIMx的中断待处理位:TIM 中断源
       
//     }
// }
    
