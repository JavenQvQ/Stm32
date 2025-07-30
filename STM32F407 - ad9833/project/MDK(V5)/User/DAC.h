#ifndef DAC_H
#define DAC_H

#include "stm32f4xx.h"
void DAC_Configuration(uint16_t *DA1_Value, uint32_t DA1_Value_Length, float amplitude);
void Tim4_Configuration(uint32_t Fre, uint32_t DA1_Value_Length);
#endif
