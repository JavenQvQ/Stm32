
#include "board.h"
#include "bsp_uart.h"
#include "OLED.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "ADC.h"
#include <stdio.h>

#define FFT_LENGTH		1024 		//FFT长度
float fft_inputbuf[FFT_LENGTH*2];	//FFT输入数组
float fft_outputbuf[FFT_LENGTH];	//FFT输出数组
float fft_outputbufhalf[FFT_LENGTH/2];
extern uint8_t ADC1_DMA_Flag;
extern u16 ADC1_ConvertedValue[ ADC1_DMA_Size ];
u32 i=0;
int newvalue = 0;
int a;


int main(void)
{
	board_init();
	OLED_Init();
	uart1_init(115200U);
	ADC_FFT_Init();
	arm_cfft_radix4_instance_f32 scfft;//FFT结构体
	arm_cfft_radix4_init_f32(&scfft,FFT_LENGTH,0,1);//初始化FFT结构体,FFT长度，FFT运算模式，FFT位倒序
	while(1)
	{
		
		if(ADC1_DMA_Flag==1)
		{	
			printf("%d",ADC1_ConvertedValue[0]);
			ADC1_DMA_Flag=0;
			
			for(i=0;i<1024;i++)
			{
				fft_inputbuf[i*2]=ADC1_ConvertedValue[i]*3.3/4096;//将ADC采样值转换为电压值
				
				fft_inputbuf[i*2+1]=0;//虚部为0
			}
			// arm_cfft_radix4_f32(&scfft,fft_inputbuf);//FFT运算
			// arm_cmplx_mag_f32(fft_inputbuf,fft_outputbuf,FFT_LENGTH);//计算幅值
			ADC_DMA_Trig(1024);  
		}

		
		
		OLED_Refresh();
	}
	

}
