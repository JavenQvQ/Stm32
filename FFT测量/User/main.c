#include "stm32f10x.h" // Device header
#include "delay.h"
#include "key.h"
#include "oled.h"
#include "PWM.h"
#include "IC.h"
#include "table_fft.h"
#define NPT 1024
uint32_t InBufArray[1024];//存放ADC值
uint32_t OutBufArray[512];//存放FFT变换后的值
uint32_t MagBufArray[512];//存放FFT变换后的值

u8 flag;
//FFT算法,计算幅值
void GetPowerMag()
{
    signed short lX,lY;
    float X,Y,Mag;
    unsigned short i;
	u16 temp;
    for(i=0; i<512; i++)
    {
        lX  = (OutBufArray[i] << 16) >> 16;
        lY  = (OutBufArray[i] >> 16);
        
        //除以32768再乘65536是为了符合浮点数计算规律
        X = NPT * ((float)lX) / 32768;
        Y = NPT * ((float)lY) / 32768;
        Mag = sqrt(X * X + Y * Y) / NPT;
        if(i == 0)
            MagBufArray[i] = (unsigned long)(Mag * 32768);
        else
            MagBufArray[i] = (unsigned long)(Mag * 65536);   //MagBufArray[]为计算输出的幅值数组
    }

}


int main()
{
	
	OLED_Init();
	ADC1_Init();
	Key_Init();
	while(1)
	{
		if(flag==1)
		{
			flag=0;
			cr4_fft_1024_stm32(InBufArray,OutBufArray,1024);//FFT算法
			GetPowerMag();//计算幅值
		}

		


	
 	} 
}
  


