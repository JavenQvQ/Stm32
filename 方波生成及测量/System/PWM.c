#include "stm32f10x.h"
#include <math.h> // Include the math.h header file to declare the sqrt function
void PWM_Init()
{

    //配置PA1为TIM2的通道1的PWM输出引脚
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能GPIOA时钟
    GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;//设置引脚模式为复用推挽输出
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;//设置引脚号为1
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置引脚速率为50MHz
    GPIO_Init(GPIOA,&GPIO_InitStructure);//根据参数初始化GPIOA的引脚
    //配置TIM2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//使能TIM2时钟
    TIM_InternalClockConfig(TIM2);//设置TIM2时钟源为内部时钟源
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;//定义结构体
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter=0;//设置重复计数器的值,减小延迟
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;//设置时钟分割:TDTS=TIM_CKD_DIV1
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//设置计数器模式
    TIM_TimeBaseInitStructure.TIM_Period=2000-1;//设置自动重装载寄存器周期的值ARR
    TIM_TimeBaseInitStructure.TIM_Prescaler=36-1;//设置用来作为TIMx时钟频率除数的预分频值PSC
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//根据指定的参数初始化TIMx的时间基数单位
    TIM_Cmd(TIM2,ENABLE);//使能TIMx外设
    //配置TIM2的通道2为PWM输出模式
    TIM_OCInitTypeDef TIM_OCInitStructure;//定义结构体
    TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;//选择定时器模式:TIM脉冲宽度调制模式1
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//输出使能
    TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;//设置极性:TIM输出比较极性高
    TIM_OCInitStructure.TIM_OCNPolarity= TIM_OCPolarity_High;//互补输出极性
    TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//比较输出使能
    TIM_OCInitStructure.TIM_Pulse=50;//设置待装入捕获比较寄存器的脉冲值ccr
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;//空闲状态
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;//互补空闲状态
    TIM_OC2Init(TIM2,&TIM_OCInitStructure);//根据TIM_OCInitStruct中指定的参数初始化外设TIMx
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);//使能TIMx在CCR2上的预装载寄存器

}
//设置PWM的占空比
void PWM_SetDutyCycle(uint16_t dutyCycle)
{
    TIM_SetCompare2(TIM2,dutyCycle);//设置待装入捕获比较寄存器的脉冲值ccr
}
//设置PWM的频率
void PWM_SetFrequency(uint16_t f)
{

// 计算最合适的分频和重装值-------------------------------------------
	uint32_t period,prescaler;
	float midFloat,clkFloat ;
	long clkInt;
	long midInt;
	clkFloat = 72000000.0f/f;
	if(clkFloat-(long)clkFloat>=0.5f)  		clkInt = clkFloat+1;
	else							 		clkInt = (long)clkFloat;
	
    midFloat = sqrt(clkFloat);// 开方
	if(midFloat-(long)midFloat>=0.5f)  		midInt = (long)midFloat+1;
	else									midInt = (long)midFloat;
	// 找一组最接近的
	for(int i=midInt;i>=1;i--)
	{
		if(clkInt%i==0)//找到一个能整除的,就是最接近的,因为是从大到小找的
		{
			prescaler = i;
			period = clkInt/i;
			break;
		}
	}
    TIM_PrescalerConfig(TIM2,prescaler-1,TIM_PSCReloadMode_Immediate);//设置用来作为TIMx时钟频率除数的预分频值PSC
    TIM_SetAutoreload(TIM2,period-1);//设置自动重装载寄存器周期的值ARR
    if(period%2!=0)//如果是奇数
    period=period+1;//保证ccr为整数
    TIM_SetCompare2(TIM2,period/2);//设置待装入捕获比较寄存器的脉冲值ccr
}
