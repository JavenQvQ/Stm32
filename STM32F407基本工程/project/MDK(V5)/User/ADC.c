 #include "board.h"



/**********************************************************
 * 函 数 名 称：Adc_Init
 * 函 数 功 能：初始化ADC
 * 传 入 参 数：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：LP
**********************************************************/
void Adc_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStruct;
	ADC_InitTypeDef ADC_InitStruct;

	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit();//ADC复位

	ADC_CommonInitStruct.ADC_DMAAccessMode=ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStruct.ADC_Mode=ADC_Mode_Independent;
	ADC_CommonInitStruct.ADC_Prescaler=ADC_Prescaler_Div4;
	ADC_CommonInitStruct.ADC_TwoSamplingDelay=ADC_TwoSamplingDelay_5Cycles;

	ADC_CommonInit(&ADC_CommonInitStruct);


	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStruct.ADC_DataAlign=ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ExternalTrigConvEdge=ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_NbrOfConversion=1;
	ADC_InitStruct.ADC_Resolution=ADC_Resolution_12b;
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;

	ADC_Init(ADC1, &ADC_InitStruct);

	ADC_Cmd(ADC1, ENABLE);
}
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
void ADC_DMA_Init(void)
{
    
}