#ifndef ADC_H
#define ADC_H

#include "stm32f4xx.h"
void Adc_Init(void);
uint16_t Get_Adc_Average(uint8_t CHx,uint8_t times);
uint16_t Get_Adc(uint8_t CHx);

#endif

