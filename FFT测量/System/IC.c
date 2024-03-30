#include "stm32f10x.h"
uint16_t ADC_Value;//存放ADC值
void TIM1_Init(void)
{
		TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		TIM_OCInitTypeDef TIM_OCInitStructure;
		
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 		//时钟使能

		TIM_TimeBaseStructure.TIM_Period = 30000-1; 		//设置在下一个更新事件装入活动的自动重装载寄存器周期的值
		TIM_TimeBaseStructure.TIM_Prescaler =7200-1; 			//设置用来作为TIMx时钟频率除数的预分频值
		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//设置时钟分割:不分割
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 		//TIM向上计数模式
		TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);			//根据指定的参数初始化TIMx的时间基数单位
		
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;		//选择定时器模式:TIM脉冲宽度调制模式1
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;		//比较输出使能
		TIM_OCInitStructure.TIM_Pulse = 9;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;		//输出极性:TIM输出比较极性低
		TIM_OC2Init(TIM2, & TIM_OCInitStructure);		//初始化外设TIM2_CH2
		
		TIM_Cmd(TIM2, ENABLE); 			//使能TIMx
}

void ADC1_Init(uint32_t AddrB)
{
    TIM1_Init();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//ADC时钟设置为PCLK2的6分频，即12MHz
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);//

    GPIO_InitTypeDef GPIO_A;
    GPIO_A.GPIO_Mode=GPIO_Mode_AIN;//模拟输入
    GPIO_A.GPIO_Pin=GPIO_Pin_0;
    GPIO_Init(GPIOA,&GPIO_A);

    ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);//ADC1,通道0,采样时间为55.5个周期
    ADC_InitTypeDef ADC_1;
    ADC_1.ADC_Mode=ADC_Mode_Independent;//独立模式
    ADC_1.ADC_DataAlign=ADC_DataAlign_Right;//右对齐
    ADC_1.ADC_ExternalTrigConv= ADC_ExternalTrigConv_T2_CC2;//外部触发源选择
    ADC_1.ADC_NbrOfChannel=1;//1个转换通道
    ADC_1.ADC_ScanConvMode=DISABLE;//非扫描模式
    ADC_1.ADC_ContinuousConvMode=DISABLE;//连续转换模式
    ADC_Init(ADC1,&ADC_1);

    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;//要传输的数据的地址,外设
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADC_Value;//存放数据的地址,内存
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//数据传输方向,外设到内存就是由A到B
    DMA_InitStructure.DMA_BufferSize = 1;//数据传输量,给传输寄存器的值
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//要传输的地址不自增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//要储存的地址自增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//内存数据长度为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;//优先级高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//非内存到内存模式+
    DMA_Init(DMA1_Channel1,&DMA_InitStructure);//初始化DMA1的通道1
    DMA_Cmd(DMA1_Channel1,ENABLE);//开启DMA1的通道1
    ADC_Cmd(ADC1,ENABLE);
    ADC_DMACmd(ADC1,ENABLE);//使能ADC1的DMA



    ADC_ResetCalibration(ADC1);//复位校准
    while(ADC_GetResetCalibrationStatus(ADC1)==SET);//等待复位校准结束
    ADC_StartCalibration(ADC1);//开始校准
    while(ADC_GetCalibrationStatus(ADC1)==SET);//等待校准结束
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
}

