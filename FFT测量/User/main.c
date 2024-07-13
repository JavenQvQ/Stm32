#include "stm32f10x.h" // Device header
#include "delay.h"
#include "key.h"
#include "oled.h"
#include "PWM.h"
#include "IC.h"
#include "stm32_dsp.h"
#include "math.h"
#define NPT 1024
uint32_t InBufArray[1024];//存放ADC值
uint32_t OutBufArray[512];//存放FFT变换后的值
uint32_t MagBufArray[512];//存放FFT变换后的值

u8 flag;
/*
*@brief  获得幅值
*@param  无
*@retval 无
*/
void GetPowerMag()
{
    signed short lX,lY;
    float X,Y,Mag;
    unsigned short i;
    uint32_t MaxMag = 0;//最大幅值
    uint32_t MaxMagIndex = 0;//最大幅值的下标
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
        if(MagBufArray[i] > MaxMag)//找到最大幅值和对应的下标
        {
            MaxMag = MagBufArray[i];
            MaxMagIndex = i;
        }

    }
        OLED_ShowNum(1,1,MaxMagIndex*70.3125,5);//显示最大幅值的下标
}


int main()
{
	
	OLED_Init();
	ADC1_Init();
    PWM_Init();//初始化PWM
    PWM_SetFrequency(1000);//设置PWM频率
	while(1)
	{
		if(flag==1)
		{
            DMA_Cmd(DMA1_Channel1,DISABLE);
			flag=0;
			cr4_fft_1024_stm32(OutBufArray,InBufArray,1024);//FFT算法
			GetPowerMag();//计算幅值
            DMA_Cmd(DMA1_Channel1,ENABLE);//开启DMA1的通道1
		}
 	} 
}
  


