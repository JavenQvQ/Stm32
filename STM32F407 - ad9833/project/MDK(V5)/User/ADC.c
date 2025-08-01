 #include "board.h"
 #include "ADC.h"
 #include "arm_math.h"
#include "board.h"
#include "ADC.h"
#include "arm_math.h"

// 修改为双ADC数据读取地址
#define ADC_CDR_ADDRESS  ((uint32_t)0x40012308)          //双ADC DMA读取地址

// 修改数据存储结构，用于存储双ADC数据
__IO uint32_t ADC1_ConvertedValue[ADC1_DMA_Size];  // 原始32位双ADC数据
__IO uint16_t AD9833_Value[ADC1_DMA_Size];         // AD9833信号数据（ADC1，PB1）
__IO uint16_t RLC_Value[ADC1_DMA_Size];            // RLC电路数据（ADC2，PB0）
uint8_t ADC1_DMA_Flag;
/**********************************************************
 * 函 数 名 称：Get_Adc
 * 函 数 功 能：获得某个通道值（单次采集）
 * 传 入 参 数：CHx：通道数字
 * 函 数 返 回：ADC转换值
 * 作       者：LC
 * 备       注：LP
**********************************************************/
uint16_t Get_Adc(uint8_t CHx) 	 
{
    ADC_RegularChannelConfig(ADC1, CHx, 1, ADC_SampleTime_480Cycles);			    
    ADC_SoftwareStartConv(ADC1);	
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}
 
/**********************************************************
 * 函 数 名 称：Get_Adc_Average
 * 函 数 功 能：计算得出某个通道给定次数采样的平均值 
 * 传 入 参 数：CHx：通道数字，times：采集次数
 * 函 数 返 回：平均值
 * 作       者：LC
 * 备       注：LP
**********************************************************/
uint16_t Get_Adc_Average(uint8_t CHx, uint8_t times)
{
    uint32_t value = 0;
    uint8_t t;
    
    for(t = 0; t < times; t++)
    {
        value += Get_Adc(CHx);
        delay_ms(5);
    }
    return value / times;
}

/**********************************************************
 * 函 数 名 称：Get_DualADC_Data
 * 函 数 功 能：从双ADC数据中提取指定ADC的数据
 * 传 入 参 数：adc_num：ADC编号(1或2)，index：数据索引
 * 函 数 返 回：提取的ADC数据
 * 作       者：LC
 * 备       注：新增函数
**********************************************************/
uint16_t Get_DualADC_Data(uint8_t adc_num, uint16_t index)
{
    if(index >= ADC1_DMA_Size) return 0;
    
    if(adc_num == 1)
    {
        // ADC1数据在低16位
        return (uint16_t)(ADC1_ConvertedValue[index] & 0xFFFF);
    }
    else if(adc_num == 2)
    {
        // ADC2数据在高16位
        return (uint16_t)((ADC1_ConvertedValue[index] >> 16) & 0xFFFF);
    }
    
    return 0;
}
void Split_ADC_Data(void)
{
    for(uint16_t i = 0; i < ADC1_DMA_Size; i++)
    {
        // 提取ADC1数据（低16位）到AD9833_Value数组
        AD9833_Value[i] = (uint16_t)(ADC1_ConvertedValue[i] & 0xFFFF);
        
        // 提取ADC2数据（高16位）到RLC_Value数组
        RLC_Value[i] = (uint16_t)((ADC1_ConvertedValue[i] >> 16) & 0xFFFF);
    }
}

void ADC_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOB时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 
    
    // 配置ADC1通道9 - PB1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 配置ADC2通道8 - PB0（修改为PB0）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//配置ADC的TIM3
//Fre为ADC的采样频率
void TIM3_Config(u32 Fre)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

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

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    //TIM3配置
    TIM_TimeBaseStructure.TIM_Period = period - 1;//自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;//定时器分频
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 配置PWM输出用于触发ADC
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = TIM_TimeBaseStructure.TIM_Period / 2;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);

    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);	
}

//ADC-DMA配置,在正常模式下启用,使用后会自动关闭
//Size为单次传输的数据量
void ADC_DMA_Trig(u16 Size)                    
{
    DMA2_Stream0->CR &= ~((uint32_t)DMA_SxCR_EN);
    DMA2_Stream0->NDTR = Size;
    DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0|DMA_IT_DMEIF0|DMA_IT_TEIF0|DMA_IT_HTIF0|DMA_IT_TCIF0);
    ADC1->SR = 0;
    ADC2->SR = 0;  // 清除ADC2状态寄存器
    DMA2_Stream0->CR |= (uint32_t)DMA_SxCR_EN;
}	

//ADC—DMA相关配置
//ADC、DMA和NVIC中断的初始化（修改为双ADC同步模式）
void ADC_Config(void)
{
    ADC_InitTypeDef       ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    DMA_InitTypeDef       DMA_InitStructure;
    NVIC_InitTypeDef      NVIC_InitStructure;
 
    ADC_DeInit();
    DMA_DeInit(DMA2_Stream0);
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);
 
    // NVIC配置
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);     
 
    // ADC公共配置 - 配置为双ADC同步模式
    ADC_CommonInitStructure.ADC_Mode = ADC_DualMode_RegSimult;  // 双ADC同步规则模式
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2; 
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_2; // 双ADC DMA模式2
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);
 
    // ADC1配置
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_CC1;  // 修改为TIM3 CC1触发
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // ADC2配置
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // ADC2不需要外部触发
    ADC_Init(ADC2, &ADC_InitStructure);
 
    // 配置ADC1通道9（PB1）
    ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, ADC_SampleTime_56Cycles);
    
    // 配置ADC2通道8（PB0）
    ADC_RegularChannelConfig(ADC2, ADC_Channel_8, 1, ADC_SampleTime_56Cycles);
 
    // 使能多ADC模式DMA请求
    ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);
    
    // 使能ADC
    ADC_Cmd(ADC1, ENABLE);
    ADC_Cmd(ADC2, ENABLE);
    
    // 使能ADC1的DMA
    ADC_DMACmd(ADC1, ENABLE);
    
    // DMA配置 - 修改为双ADC数据传输
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC_CDR_ADDRESS;  // 双ADC公共数据寄存器地址
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC1_ConvertedValue;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = ADC1_DMA_Size;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;  // 32位数据
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;          // 32位数据
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;  // 启用FIFO
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;		
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;	
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
    DMA_Cmd(DMA2_Stream0, DISABLE);
    
    DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA2_Stream0, ENABLE);
}


//ADC-DMA初始化,双ADC同步采集模式
void ADC_FFT_Init()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    ADC_GPIO_Init();
    TIM3_Config(SAM_FRE);
    ADC_Config();
}

//DMA中断配置
void DMA2_Stream0_IRQHandler(void)
{
    if(DMA_GetFlagStatus(DMA2_Stream0, DMA_IT_TCIF0) != RESET)
    {
        ADC1_DMA_Flag = 1;
        Split_ADC_Data();        
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0); // 清除DMA传输完成标志
    }
    DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0|DMA_IT_DMEIF0|DMA_IT_TEIF0|DMA_IT_HTIF0|DMA_IT_TCIF0);
}