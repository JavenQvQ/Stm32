
#include "board.h"
#include "bsp_uart.h"
#include "OLED.h"//使用PB14 PB15
#include "arm_math.h"
#include "arm_const_structs.h"
#include "ADC.h"//使用PB01和定时器3以及DMA2_Stream0
#include "ads8361.h"//使用TIM6和PD8-PD15
#include "DAC.h"//使用PA04和TIM4以及DMA1_Stream5
#include <stdio.h>


// #define FFT_LENGTH		1024 		//FFT长度
// float fft_inputbuf[FFT_LENGTH*2];	//FFT输入数组
// float fft_outputbuf[FFT_LENGTH];	//FFT输出数组
// float fft_outputbufhalf[FFT_LENGTH/2];

// u32 i=0;
// int newvalue = 0;
// int a;


extern uint8_t ADC1_DMA_Flag;//ADC采集完成1024个数据标志
extern u16 ADC1_ConvertedValue[ ADC1_DMA_Size ];//ADC数据

extern uint16_t ADS8361_DB_data1[1024];//DB数据
extern uint8_t  ADS8361_Recive_Flag;//ADS8361采集完成1024个数据标志



//ADS8361数据处理函数,通过定时器中断采集数据,采集1024个数据后进入数据处理函数
//放在主循环中调用
void ADS8361_DATAHandle(void)
{
	if(ADS8361_Recive_Flag==1)
		{
			ADS8361_Recive_Flag=0;
			for(int i=0;i<1024;i++)
			{
				printf("%d\n",ADS8361_DB_data1[i]);
				
			}
			//TIM6_Config(1000000);//定时器100khz的频率采集数据
			
			printf("done\n");
	    }
}


//ADC数据处理函数,通过DMA采集数据,采集1024个数据后进入数据处理函数
//放在主循环中调用
void ADC_DATAHandle(void)
{
	if(ADC1_DMA_Flag==1)
	{	
		ADC1_DMA_Flag=0;
		//数据处理
		for(int i=0;i<1024;i++)
		{
			printf("%d\n",ADC1_ConvertedValue[i]);
		}
		ADC_DMA_Trig(1024);//重新开始采集数据
	}
}


int main(void)
{
	board_init();
	OLED_Init();
	uart1_init(115200U);
	uart2_init(115200U);
	ADC_FFT_Init();//ADC初始化

	
	DAC_Configuration();//DAC初始化
	Tim4_Configuration(1000);//DAC输出频率10khz
	while(1)
	{

		ADC_DATAHandle();//ADC数据处理函数





	}
}
