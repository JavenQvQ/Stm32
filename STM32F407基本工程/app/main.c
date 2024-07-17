
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
u32 i=0;
int newvalue = 0;
int a;


int main(void)
{
	board_init();
	OLED_Init();
	uart1_init(115200U);
	RCC_ClocksTypeDef rcc;
    RCC_GetClocksFreq(&rcc);//读取系统时钟频率



	arm_cfft_radix4_instance_f32 scfft;//FFT结构体
	int max1,max2 ;
	float max;
	int n=0;
	float A1,A2;
	ADC_FFT_Init();
	arm_cfft_radix4_init_f32(&scfft,FFT_LENGTH,0,1);//初始化FFT结构体,FFT长度，FFT运算模式，FFT位倒序
	while(1)
	{
		if(ADC1_DMA_Flag==1)
		{
			ADC1_DMA_Flag=0;
			DMA_Cmd(DMA2_Stream0, DISABLE);//关闭DMA
			for(i=0;i<FFT_LENGTH;i++)
			{
				fft_inputbuf[i*2]=ADC1_ConvertedValue[i]*3.3/4096;//将ADC采样值转换为电压值
				fft_inputbuf[i*2+1]=0;//虚部为0
			}
			arm_cfft_radix4_f32(&scfft,fft_inputbuf);//FFT运算
			arm_cmplx_mag_f32(fft_inputbuf,fft_outputbuf,FFT_LENGTH);//计算幅值
			arm_max_f32(fft_outputbuf,FFT_LENGTH,&max,&max1);//找出最大值

			DMA_Cmd(DMA2_Stream0, ENABLE);//开启DMA
		}

		
		
		OLED_Refresh();
	}
	

}
