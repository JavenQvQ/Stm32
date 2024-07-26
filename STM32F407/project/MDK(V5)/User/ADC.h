#ifndef ADC_H
#define ADC_H

#include "stm32f4xx.h"
void Adc_Init(void);
uint16_t Get_Adc_Average(uint8_t CHx,uint8_t times);
uint16_t Get_Adc(uint8_t CHx);
extern uint8_t ADC1_DMA_Flag;

#define SAM_FRE        1048576//采样频率尽量为2的n次方
#define ADC1_DMA_Size   4096//采样点数尽量为2的n次方
#define FFT_Size       ADC1_DMA_Size//FFT变换长度,当FFT长度小于输入波形长度时，裁剪输入可以减少FFT需要处理的数据量，从而提高计算速度。但这样做可能会丢失波形中的一些信息。
                                    //当FFT长度大于输入波形长度时，通过补零来增加数据点的数量。这样做通常不会增加频谱分辨率（因为实际的信号信息没有增加），但可以使频
                                    //谱看起来更加平滑，并可能有助于在图形化展示时更清楚地识别出峰值。



// **采样频率**为TIM生成的信号频率,为SAM_FRE,求倒数可得==采样周期==

// **采样点数**为ADC1_DMA_Size

// **分辨率**为采样频率/采样点数=SAM_FRE/ADC1_DMA_Size(2048)=8Hz


void ADC_GPIO_Init(void);
void TIM3_Config( u32 Fre );
void ADC_Config( void );
void ADC_DMA_Trig( u16 Size );
void ADC_FFT_Init(void);


#endif

