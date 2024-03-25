#include "stm32f10x.h"
uint8_t IC_Flag=0;//溢出标志位
uint8_t IC_CNT=0;//溢出计数器
static uint64_t tmp16_CH1;
void IC_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能GPIOA时钟
    GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//设置引脚模式为上拉输入
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_6;//设置引脚号为6
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置引脚速率为50MHz
    GPIO_Init(GPIOA,&GPIO_InitStructure);//根据参数初始化GPIOA的引脚

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);//使能TIM3时钟
    TIM_DeInit(TIM3);//复位TIM3
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;//定义结构体
    TIM_TimeBaseStructure.TIM_Period = 65535;//根据个人需求进行配置       
	TIM_TimeBaseStructure.TIM_Prescaler = 8;  //预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//输入时钟不分频
	TIM_TimeBaseStructure.TIM_CounterMode =	TIM_CounterMode_Up; 	//向上计数
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);

    TIM_ICInitTypeDef TIM_ICInitStructure;//定义结构体
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;     //捕捉上升沿
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;    //捕捉中断
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;       //捕捉不分频
	TIM_ICInitStructure.TIM_ICFilter = 0x3;          //捕捉输入滤波
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

    TIM_ClearFlag(TIM3, TIM_FLAG_Update);  //清除溢出标志
    NVIC_InitTypeDef NVIC_InitStructure;  //定义结构体
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);	  

    TIM_ClearFlag(TIM3,TIM_FLAG_Update);
	TIM_ARRPreloadConfig(TIM3,DISABLE);
	
	/* Enable the CC2 Interrupt Request */
	TIM_ITConfig(TIM3, TIM_IT_CC1|TIM_IT_Update , ENABLE);						
    /* TIM enable counter */
	TIM_Cmd(TIM3, ENABLE);

}

uint16_t IC_GetFrequency()
{
    return 8000000/tmp16_CH1;
}

void TIM3_IRQHandler(void)                        //定时器中断
{
	//频率缓冲区计数
	static u32 this_time_CH1 = 0;
	static u32 last_time_CH1 = 0;
	static u8 capture_number_CH1 = 0;
    static u8 CH1_Cycles_Count=0;


		
	if (TIM_GetITStatus(TIM3,TIM_IT_Update) != RESET)       //计数器更新中断
	{
		  TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		  //溢出次数通道1、2分开统计
		  CH1_Cycles_Count++;
	}
	if(TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET)   //通道1脉冲检测中断
	{
			TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
			if(capture_number_CH1 == 0)                          //收到第一个上升沿
			{
				capture_number_CH1 = 1;
				last_time_CH1 = TIM_GetCapture1(TIM3);
			}
			else if(capture_number_CH1 == 1)                     //收到第二个上升沿
			{
				//capture_number_CH1 = 0;
				this_time_CH1 = TIM_GetCapture1(TIM3);
				if(this_time_CH1 > last_time_CH1)
				{
					tmp16_CH1 = this_time_CH1 - last_time_CH1 + 65536* CH1_Cycles_Count;
				}
				else
				{
					tmp16_CH1 = 65536 * CH1_Cycles_Count - last_time_CH1 + this_time_CH1;
				}			
				CH1_Cycles_Count = 0;
				last_time_CH1 = this_time_CH1;	
			}							
	}
}


