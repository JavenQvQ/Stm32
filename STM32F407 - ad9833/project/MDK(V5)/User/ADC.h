#ifndef ADC_H
#define ADC_H

#include "stm32f4xx.h"
void Adc_Init(void);
uint16_t Get_Adc_Average(uint8_t CHx,uint8_t times);
uint16_t Get_Adc(uint8_t CHx);
extern uint8_t ADC1_DMA_Flag;

#define SAM_FRE        10000//524288//采样频率尽量为2的n次方,2的19次方
#define ADC1_DMA_Size   1024//采样点数尽量为2的n次方



// **采样频率**为TIM生成的信号频率,为SAM_FRE,求倒数可得==采样周期==

// **采样点数**为ADC1_DMA_Size

// **分辨率**为采样频率/采样点数=SAM_FRE/ADC1_DMA_Size(2048)=8Hz


void ADC_GPIO_Init(void);
void TIM3_Config( u32 Fre );
void ADC_Config( void );
void ADC_DMA_Trig( u16 Size );
void ADC_FFT_Init(void);
uint16_t Get_DualADC_Data(uint8_t adc_num, uint16_t index);


#endif

