#ifndef ADC_H
#define ADC_H

#include "stm32f4xx.h"
void Adc_Init(void);
uint16_t Get_Adc_Average(uint8_t CHx,uint8_t times);
uint16_t Get_Adc(uint8_t CHx);
extern uint8_t ADC1_DMA_Flag;

#define SAM_FRE        20000//采样频率
#define ADC1_DMA_Size  1024 //采样点数

void ADC_GPIO_Init(void);
void TIM3_Config( u32 Fre );
void ADC_Config( void );
void ADC_DMA_Trig( u16 Size );
void ADC_FFT_Init(void);


#endif

