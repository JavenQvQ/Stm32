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
int main()
{
	OLED_Init();
	PWM_Init();
	IC_Init();
	Key_Init();
	
	while(1)
	{
		KeyCode=KEY_FIFO_Get();
		switch (KeyCode)
		{
		case KEY_1_DOWN:
		{
			frequency_Set=frequency_Set+1;
			if(frequency_Set>=10000)
			frequency_Set=1;
			
			OLED_ShowNum(2,1,frequency_Set,7);
		}break;
		case KEY_1_LONG:
		{
			frequency_Set=frequency_Set+100;
			if(frequency_Set>=10000)
			frequency_Set=1;
			OLED_ShowNum(2,1,frequency_Set,7);
		}
		case KEY_2_DOWN:
		{
			frequency_Set=frequency_Set-1;
			if(frequency_Set==1)
			frequency_Set=10000;
			OLED_ShowNum(2,1,frequency_Set,7);
		}break;
		case KEY_2_LONG:
		{
			frequency_Set=frequency_Set-1;
			if(frequency_Set==1)
			frequency_Set=10000;
			OLED_ShowNum(2,1,frequency_Set,7);		
		}break;
		case KEY_3_LONG:
		{
			frequency_Set=frequency_Set-100;
			if(frequency_Set<=100)
			frequency_Set=10000;
			OLED_ShowNum(2,1,frequency_Set,7);
		}break;
		}
		if(frequency_Set!=frequency)
		{
		frequency=frequency_Set;
		PWM_SetFrequency(frequency);			
		}//显示频率
		
		OLED_ShowNum(1,1,IC_GetFrequency(),7);
	}
	
} 

  


