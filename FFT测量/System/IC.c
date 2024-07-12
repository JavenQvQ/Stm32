#include "stm32f10x.h"
uint16_t ADC_Value[1024];//存放ADC值
extern uint32_t InBufArray[1024];//存放ADC值
extern u8 flag;

//采样频率为TIM生成的PWM信号频率,为4khz
//采样点数为1024
//分辨率为采样频率/采样点数=4khz/1024=3.90625hz
//ADC转换时间为55.5个周期,即13.5/12M=1.125us

void TIM1_Init(void)
{
		TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		TIM_OCInitTypeDef TIM_OCInitStructure;
		
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); 		//时钟使能,不能用TIM1!TIM1是高级定时器,得加TIM_CtrlPWMOutputs(TIM1,ENABLE)函数

		TIM_TimeBaseStructure.TIM_Period = 1000-1; 		//设置在下一个更新事件装入活动的自动重装载寄存器周期的值
		TIM_TimeBaseStructure.TIM_Prescaler =18-1; 			//设置用来作为TIMx时钟频率除数的预分频值
		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//设置时钟分割:不分割
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 		//TIM向上计数模式
		TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);			//根据指定的参数初始化TIMx的时间基数单位
		
        TIM_OCStructInit(&TIM_OCInitStructure);
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;		//选择定时器模式:TIM脉冲宽度调制模式1
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;		//比较输出使能
		TIM_OCInitStructure.TIM_Pulse = 9;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;		//输出极性:TIM输出比较极性低
		TIM_OC2Init(TIM1, & TIM_OCInitStructure);		//初始化外设TIM2_CH2
		//TIM_GenerateEvent(TIM2,TIM_EventSource_Update);		//使能TIM2在CCR2上的预装载寄存器
		TIM_Cmd(TIM1, ENABLE); 			//使能TIMx
        TIM_CtrlPWMOutputs(TIM1,ENABLE);//使能TIM1的PWM输出
}

void DMA1_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;//要传输的数据的地址,外设
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADC_Value;//存放数据的地址,内存
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//数据传输方向,外设到内存就是由A到B
    DMA_InitStructure.DMA_BufferSize = 1024;//数据传输量,给传输寄存器的值
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//要传输的地址不自增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//要储存的地址自增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//内存数据长度为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;//优先级高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//非内存到内存模式+
    DMA_Init(DMA1_Channel1,&DMA_InitStructure);//初始化DMA1的通道1

    DMA_ClearFlag(DMA1_FLAG_TC1);//清除DMA1通道1传输完成标志
    DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);//开启DMA1通道1传输完成中断
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel=DMA1_Channel1_IRQn;//DMA1通道1中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;//响应优先级1
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//使能中断
    NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA1_Channel1,ENABLE);//开启DMA1的通道1
}


void ADC1_Init(uint32_t AddrB)
{
    TIM1_Init();
    DMA1_Init();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//ADC时钟设置为PCLK2的6分频，即12MHz

    GPIO_InitTypeDef GPIO_A;
    GPIO_A.GPIO_Mode=GPIO_Mode_AIN;//模拟输入
    GPIO_A.GPIO_Pin=GPIO_Pin_0;
    GPIO_Init(GPIOA,&GPIO_A);

    ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_13Cycles5);//ADC1,通道0,采样时间为1.5个周期
    ADC_InitTypeDef ADC_1;
    ADC_1.ADC_Mode=ADC_Mode_Independent;//独立模式
    ADC_1.ADC_DataAlign=ADC_DataAlign_Right;//右对齐
    ADC_1.ADC_ExternalTrigConv= ADC_ExternalTrigConv_T1_CC2;//外部触发源选择
    ADC_1.ADC_NbrOfChannel=1;//1个转换通道
    ADC_1.ADC_ScanConvMode=DISABLE;//非扫描模式
    ADC_1.ADC_ContinuousConvMode=DISABLE;//连续转换模式
    ADC_Init(ADC1,&ADC_1);

    
    ADC_Cmd(ADC1,ENABLE);
    ADC_DMACmd(ADC1,ENABLE);//使能ADC1的DMA



    ADC_ResetCalibration(ADC1);//复位校准
    while(ADC_GetResetCalibrationStatus(ADC1)==SET);//等待复位校准结束
    ADC_StartCalibration(ADC1);//开始校准
    while(ADC_GetCalibrationStatus(ADC1)==SET);//等待校准结束
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
}

void  DMA1_Channel1_IRQHandler(void)
{
	u16 i = 0;
	if(DMA_GetITStatus(DMA1_IT_TC1)!=RESET)
	{
		for(i=0;i<1024;i++)
		{
			InBufArray[i] = ((signed short)(ADC_Value[i])) << 16;	
		}
		DMA_ClearITPendingBit(DMA1_IT_TC1);
        flag=1;
	}
}

