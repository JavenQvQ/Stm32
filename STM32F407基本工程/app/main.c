
#include "board.h"
#include "bsp_uart.h"
#include "OLED.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "ADC.h"
#include "ads8361.h"
#include <stdio.h>

// #define FFT_LENGTH		1024 		//FFT长度
// float fft_inputbuf[FFT_LENGTH*2];	//FFT输入数组
// float fft_outputbuf[FFT_LENGTH];	//FFT输出数组
// float fft_outputbufhalf[FFT_LENGTH/2];
// extern uint8_t ADC1_DMA_Flag;
// extern u16 ADC1_ConvertedValue[ ADC1_DMA_Size ];
// u32 i=0;
// int newvalue = 0;
// int a;


int main(void)
{
	board_init();
	OLED_Init();
	uart1_init(115200U);
	//uart2_init(115200U);
	ADS8361_Init();
	uint8_t i;
	float   Analog_A[2];//模拟量A
	float   Analog_B[2];//模拟量B
	uint16_t DA_data[2];//DA数据
	uint16_t DB_data[2];//DB数据
	// ADC_FFT_Init();
	while(1)
	{
		// if(ADC1_DMA_Flag==1)
		// {	
		// 	ADC1_DMA_Flag=0;
		// 	for(i=0;i<1024;i++)
		// 	{
		// 		fft_inputbuf[i*2]=ADC1_ConvertedValue[i]*3.3/4096;//将ADC采样值转换为电压值
				
		// 		fft_inputbuf[i*2+1]=0;//虚部为0
		// 	}
		// 	// arm_cfft_radix4_f32(&scfft,fft_inputbuf);//FFT运算
		// 	// arm_cmplx_mag_f32(fft_inputbuf,fft_outputbuf,FFT_LENGTH);//计算幅值
		// 	ADC_DMA_Trig(1024);
			
		// }
		ADS8361_Read_Data_Mode3(DA_data, DB_data);
		
		for(i=0; i<2; i++)
		{
			if(DA_data[i] & 0x8000)
			{
				Analog_A[i] = ((DA_data[i] ^ 0x7FFF + 1)*5000.0/65535) - 2500;
			}
			else
			{
				Analog_A[i] = (DA_data[i]*5000.0/65535);
			}
			
			if(DB_data[i] & 0x8000)
			{
				Analog_B[i] = ((DB_data[i] ^ 0x7FFF + 1)*5000.0/65535) - 2500;
			}
			else
			{
				Analog_B[i] = (DB_data[i]*5000.0/65535);
			}
		}
		// printf ("CH_A0: %8.4lfmV    D: %d     ", Analog_A[0], (uint16_t)DA_data[0]);
		// printf ("CH_A1: %8.4lfmV    D: %d     \r\n", Analog_A[1], (uint16_t)DA_data[1]);
		// printf ("CH_B0: %8.4lfmV    D: %d     ", Analog_B[0], (uint16_t)DB_data[0]);
		printf ("%8.4lf\n",Analog_B[1]);
		OLED_Refresh();
	}


	

}
