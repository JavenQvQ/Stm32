#include "board.h"


#define DA1_Value_Length 24
const uint16_t DA1_Value[DA1_Value_Length] = {
	
0		,
65      ,
262     ,
563     ,
953     ,
1410    ,
1890    ,
2379    ,
2824    ,
3209    ,
3506    ,
3687    ,
3747    ,
3678    ,
3484    ,
3183    ,
2791    ,
2340    ,
1855    ,
1369    ,
920     ,
534     ,
240     ,
57   

};

//DAC输出配置
//PA4和DMA1_Stream5_CH7连接
//TIM4触发DAC,需要配置TIM4
//DAC输出频率为fre/DA1_Value_Lenth
void DAC_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    DAC_InitTypeDef DAC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T4_TRGO;//定时器4触发
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;//不产生波形
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;//不使用LFSR
    DAC_InitStructure.DAC_OutputBuffer=DAC_OutputBuffer_Enable;//输出缓存,
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_SetChannel1Data(DAC_Align_12b_R, 0);//初始化输出值,右对齐
    

    DMA_InitStructure.DMA_Channel = DMA_Channel_7;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//循环模式
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;//存储器到外设
    DMA_InitStructure.DMA_BufferSize = DA1_Value_Length;//DMA缓存大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址不增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器地址增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据大小,16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//存储器数据大小,16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;//中等优先级
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;//不使用FIFO
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;//FIFO阈值

    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//外设突发单次传输
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R1;//外设基地址
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DA1_Value;//存储器基地址
    DMA_Init(DMA1_Stream5, &DMA_InitStructure);
    
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_DMACmd(DAC_Channel_1, ENABLE);
    DMA_Cmd(DMA1_Stream5, ENABLE);
}

void Tim4_Configuration(uint32_t Fre)
{	    
    
    uint32_t period, prescaler;
    long clkInt, midInt;
    // 使用84000000.0代替硬编码的值，以提高可维护性
    clkInt = (long)(84000000.0 / Fre + 0.5); // 简化四舍五入逻辑

    midInt = (long)(float)(__sqrtf((float)clkInt) + 0.5f); // 简化四舍五入逻辑

    // 优化循环，减少迭代次数
    for (int i = midInt; i >= 1; i--) {
        if (clkInt % i == 0) { // 如果找到能整除的值
            prescaler = i;
            period = clkInt / i;
            break; // 找到后立即退出循环
        }
    }

    // 使能TIM4时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    
    TIM_TimeBaseStructure.TIM_Period = period-1;//自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler-1;//预分频
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;//时钟分频,不分频就是84M
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数

    TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update);//触发输出
    TIM_ARRPreloadConfig(TIM4, ENABLE);//使能自动重装载
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM4, ENABLE);
}

    

    


    