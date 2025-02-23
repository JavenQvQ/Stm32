 #include "board.h"
 #include "ADC.h"
 #include "arm_math.h"
#define ADC1_DR_ADDRESS  ((uint32_t)0x4001204C)          //DMA读取的ADC1地址

u16 ADC1_ConvertedValue[ ADC1_DMA_Size];
uint8_t ADC1_DMA_Flag;

/**********************************************************
 * 函 数 名 称：Get_Adc
 * 函 数 功 能：获得某个通道值
 * 传 入 参 数：CHx：通道数字
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：LP
**********************************************************/
uint16_t Get_Adc(uint8_t CHx) 	 
{
	
	ADC_RegularChannelConfig(ADC1, CHx, 1, ADC_SampleTime_480Cycles );			    
  
	ADC_SoftwareStartConv(ADC1);	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
 
	return ADC_GetConversionValue(ADC1);
}
 
 /**********************************************************
 * 函 数 名 称：Get_Adc_Average
 * 函 数 功 能：计算得出某个通道给定次数采样的平均值 
 * 传 入 参 数：CHx		：通道数字
 *				times	：采集次数
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：LP
**********************************************************/
uint16_t Get_Adc_Average(uint8_t CHx,uint8_t times)// 
{
	
	uint32_t value = 0;
	uint8_t t;
	
	for(t=0;t<times;t++)
	{
		value += Get_Adc(CHx);
		delay_ms(5);
	}
	return value/times;
}



void ADC_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE ); 
	
	//PB0 - ADC1_IN8   
	//PB1 - ADC1_IN9   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init( GPIOB , &GPIO_InitStructure );
}



//配置ADC的TIM3
//Fre为ADC的采样频率
void TIM3_Config( u32 Fre )
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    uint32_t period, prescaler;
    long clkInt, midInt;
    // 使用84000000.0代替硬编码的值，以提高可维护性
    clkInt = (long)(84000000.0 / Fre + 0.5); // 简化四舍五入逻辑

	midInt = (long)(float)(__sqrtf((float)clkInt) + 0.5f); // 简化四舍五入逻辑

    // 优化循环，减少迭代次数
    for (int i = midInt; i >= 1; i--) {
        if (clkInt % i == 0) { // 如果找到能整除的值
            prescaler = i;
            period = clkInt / i;
            break; // 找到后立即退出循环
        }
    }

	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM3 , ENABLE );			   //开启TIM3时钟
	
	//TIM3配置
	TIM_TimeBaseStructure.TIM_Period = period-1;//自动重装载值
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler-1;//定时器分频
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit( TIM3 , &TIM_TimeBaseStructure );
	
	TIM_SelectOutputTrigger( TIM3 , TIM_TRGOSource_Update );
	TIM_ARRPreloadConfig( TIM3 , ENABLE );
	TIM_Cmd( TIM3 , ENABLE );	
}						                  //使能TIM3


//ADC-DMA配置,在正常模式下启用,使用后会自动关闭
//Size为单次传输的数据量
void ADC_DMA_Trig( u16 Size )                    
{
	DMA2_Stream0->CR &= ~((uint32_t)DMA_SxCR_EN);				//将DMA2的stream0线清零
	DMA2_Stream0->NDTR = Size;                             //设置DMA2的stream0
	DMA_ClearITPendingBit( DMA2_Stream0 ,DMA_IT_TCIF0|DMA_IT_DMEIF0|DMA_IT_TEIF0|DMA_IT_HTIF0|DMA_IT_TCIF0 );
	ADC1->SR = 0;
	DMA2_Stream0->CR |= (uint32_t)DMA_SxCR_EN;//使能DMA2的stream0线
}	

//ADC—DMA相关配置
//ADC、DMA和NVIC中断的初始化
void ADC_Config(void)
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	DMA_InitTypeDef       DMA_InitStructure;
	NVIC_InitTypeDef 	  NVIC_InitStructure;
 
	ADC_DeInit( );
	DMA_DeInit( DMA2_Stream0 );
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA2 , ENABLE ); 
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1 , ENABLE );
 
	
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);     
 
 
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;     //独立模式·1
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;  //ADC时钟频率为APB2的1/4,即84/4=21MHz
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;//DMA失能
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;//两个采样阶段之间的延迟5个时钟
	ADC_CommonInit( &ADC_CommonInitStructure );
 
	
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12位模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;		//非扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//关闭连续转换,外部触发模式下，连续转换失能
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;//选择上升沿触发
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;    //选择TIM3的ADC触发
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//右对齐									
	ADC_InitStructure.ADC_NbrOfConversion = 1;  //转换通道数目                  
	ADC_Init( ADC1, &ADC_InitStructure );//初始化ADC
	
	 
 
 
	ADC_RegularChannelConfig( ADC1 , ADC_Channel_9 , 1, ADC_SampleTime_3Cycles);//配置ADC1的通道9采样时间,为3+12个时钟周期，即为15个时钟周期
	//计算过程:ADC采样时间=15/21M=,ADC采样频率为21M/15=1.4M
 
	ADC_DMARequestAfterLastTransferCmd( ADC1 , ENABLE );//使能ADC的DMA请求
	
	ADC_DMACmd(ADC1, ENABLE);//使能ADC的DMA
	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConv(ADC1);//软件触发ADC转换
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_ADDRESS;//DMA读取的目的地		 
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC1_ConvertedValue; //DMA存放的地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;				//外设到存储器模式
	DMA_InitStructure.DMA_BufferSize =ADC1_DMA_Size;                   //DMA 传输数量
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址不增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//要存储的地址增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度为半字
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//存储数据长度为半字
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;						//正常模式										
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;					//DMA优先级高
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         		//不使用FIFO
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;	//FIFO阈值
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;		
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;	
	DMA_Init( DMA2_Stream0 , &DMA_InitStructure );
	DMA_Cmd( DMA2_Stream0 , DISABLE );//关闭DMA
	
	DMA_ClearITPendingBit( DMA2_Stream0 ,DMA_IT_TCIF0 );//清除DMA中断标志
	DMA_ITConfig( DMA2_Stream0 , DMA_IT_TC , ENABLE );//使能DMA中断
	DMA_Cmd( DMA2_Stream0 , ENABLE );//开启DMA
 
	
	
}
//ADC-DMA初始化,在正常模式下启用,使用后会自动关闭
//ADC1_DMA_Size为单次传输的数据量,在ADC_Config()中调用,在ADC.h中定义
//SAM_FRE为采样频率
void ADC_FFT_Init()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	ADC_GPIO_Init();
	TIM3_Config( SAM_FRE );
	ADC_Config();

}
	



 
//DMA中断配置
void DMA2_Stream0_IRQHandler( void )
{

	if(DMA_GetFlagStatus(DMA2_Stream0,DMA_IT_TCIF0)!=RESET)//传输完成

	{
		ADC1_DMA_Flag=1;
		
	}
	DMA_ClearITPendingBit( DMA2_Stream0 ,DMA_IT_TCIF0|DMA_IT_DMEIF0|DMA_IT_TEIF0|DMA_IT_HTIF0|DMA_IT_TCIF0 );//清除DMA中断标志

}

