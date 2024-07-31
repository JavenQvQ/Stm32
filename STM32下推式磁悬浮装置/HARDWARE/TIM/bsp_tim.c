#include "bsp_tim.h"


void coil_PWM_init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_11; //TIM1_CH1  TIM1_CH4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM_TimeBaseStructure.TIM_Period = arr;
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //默认不分频
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟不分频
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数模式
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //PWM模式1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_Pulse = 0; //设置待装入捕获比较寄存器的脉冲值
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //TIM输出比较极性高
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC4Init(TIM1, &TIM_OCInitStructure);
	
	TIM_CtrlPWMOutputs(TIM1,ENABLE);	//MOE 主输出使能

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable); //CH1预装载使能
	TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable); //CH4预装载使能
	
	TIM_ARRPreloadConfig(TIM1, ENABLE); //使能TIMx在ARR上的预装载寄存器
	
	TIM_Cmd(TIM1, ENABLE);
}

//TIM3初始化,中断频率为1KHz
void TIM3_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	
	//初始化定时器1 TIM1	 
	TIM_TimeBaseStructure.TIM_Period = 4999; //设定计数器自动重装值 
	TIM_TimeBaseStructure.TIM_Prescaler = 71; 	//预分频器   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  
	
	//中断分组初始化
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);//清中断标志位
	
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);//允许更新中断
	TIM_Cmd(TIM3, ENABLE);
}

void PWM_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能GPIOA时钟
    GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;//设置引脚模式为复用推挽输出
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0;//设置引脚号为0
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置引脚速率为50MHz
    GPIO_Init(GPIOA,&GPIO_InitStructure);//根据参数初始化GPIOA的引脚
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//使能TIM2时钟
    TIM_InternalClockConfig(TIM2);//设置TIM2时钟源为内部时钟源
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;//定义结构体
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;//设置时钟分割:TDTS=TIM_CKD_DIV1
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//设置计数器模式
    TIM_TimeBaseInitStructure.TIM_Period=3599;//设置自动重装载寄存器周期的值ARR
    TIM_TimeBaseInitStructure.TIM_Prescaler=0;//设置用来作为TIMx时钟频率除数的预分频值PSC
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//根据指定的参数初始化TIMx的时间基数单位
    TIM_Cmd(TIM2,ENABLE);//使能TIMx外设
    //配置TIM2的通道1的PWM输出
    TIM_OCInitTypeDef TIM_OCInitStructure;//定义结构体
    TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;//选择定时器模式:TIM脉冲宽度调制模式1
    TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;//  设置极性:TIM输出比较极性高
    TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//比较输出使能
    TIM_OCInitStructure.TIM_Pulse=0;//设置待装入捕获比较寄存器的脉冲值ccr
    TIM_OC1Init(TIM2,&TIM_OCInitStructure);//根据TIM_OCInitStruct中指定的参数初始化外设TIMx
}