#include "board.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "ADC.h"
#include <stdio.h>




//FFT实数数据处理
//输入数据为1024个实数数据
//输出数据为按照实部，虚拟，实部，虚部，依次排列的1024个数据
void FFT_RealDatedeal(float32_t *fft_inputbuf,float32_t *fft_outputbuf)
{
    arm_rfft_fast_instance_f32 S;//FFT结构体
    arm_rfft_fast_init_f32(&S, FFT_Size);//初始化FFT为1024点
    arm_rfft_fast_f32(&S, fft_inputbuf, fft_outputbuf, 0);//FFT计算
}
//获取FFT输出数据中的最大值
//输入数据为FFT输出数据
//输出数据为最大值和最大值的索引
void FFT_GetMax(float32_t *fft_outputbuf,float32_t *maxValue,uint32_t *maxIndex)
{
    arm_max_f32(fft_outputbuf, FFT_Size/2, maxValue, maxIndex);//获取最大值和最大值的索引
    *maxValue = *maxValue*2/FFT_Size;//归一化
}
    
    



