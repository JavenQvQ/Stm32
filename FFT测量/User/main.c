#include "stm32f10x.h" // Device header
#include "delay.h"
#include "key.h"
#include "oled.h"
#include "Timer.h"
#include "PWM.h"
#include "IC.h"
uint16_t frequency=1000;
uint16_t frequency_Set=1;
uint8_t KeyCode;
uint16_t ADC_Value;
int main()
{
	OLED_Init();
	PWM_Init();
	ADC1_Init((uint32_t)&ADC_Value);
	Key_Init();
	
	while(1)
	{
		OLED_ShowNum(1,1,ADC_Value,4);
		KeyCode=KEY_FIFO_Get();

	
 	} 
}
  


