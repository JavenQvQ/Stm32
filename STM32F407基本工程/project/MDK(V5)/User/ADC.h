#ifndef ADC_H
#define ADC_H

#include "stm32f4xx.h"
void Adc_Init(void);
uint16_t Get_Adc_Average(uint8_t CHx,uint8_t times);
uint16_t Get_Adc(uint8_t CHx);


#define SAM_FRE        1024000//采样频率
#define ADC1_DMA_Size  1024 //采样点数
extern u16 ADC1_ConvertedValue[ ADC1_DMA_Size ]; 
void ADC_GPIO_Init(void);
void TIM3_Config( u32 Fre );
void ADC_Config( void );
void ADC_DMA_Trig( u16 Size );

#endif

