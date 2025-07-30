#include "board.h"


#define DA1_Value_Length 64  // 增加采样点数提高波形质量
// 标准正弦波数据 (12位DAC，0-4095范围，中心值2048)
const uint16_t DA1_Value[DA1_Value_Length] = {
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346, 3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
    4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630, 3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
    2048, 1847, 1648, 1453, 1264, 1082, 910, 749, 600, 465, 345, 242, 156, 88, 39, 10,
    0, 10, 39, 88, 156, 242, 345, 465, 600, 749, 910, 1082, 1264, 1453, 1648, 1847
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
    
    // 使能时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    
    // 配置PA5为模拟输出 (DAC Channel 2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  // 改为Pin_5
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // DMA配置需要更改为Channel 2对应的Stream
    // DAC Channel 2 使用 DMA1_Stream6_CH7
    DMA_Cmd(DMA1_Stream6, DISABLE);  // 改为Stream6
    while(DMA_GetCmdStatus(DMA1_Stream6) != DISABLE);
    
    // 配置DMA
    DMA_DeInit(DMA1_Stream6);  // 改为Stream6
    DMA_InitStructure.DMA_Channel = DMA_Channel_7;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize = DA1_Value_Length;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R2;  // 改为DHR12R2
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DA1_Value;
    DMA_Init(DMA1_Stream6, &DMA_InitStructure);  // 改为Stream6

    // 配置DAC Channel 2
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T4_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);  // 改为Channel_2
    
    // 设置初始值
    DAC_SetChannel2Data(DAC_Align_12b_R, 2048);  // 改为SetChannel2Data
    
    // 启用DAC和DMA
    DAC_Cmd(DAC_Channel_2, ENABLE);  // 改为Channel_2
    DAC_DMACmd(DAC_Channel_2, ENABLE);  // 改为Channel_2
    DMA_Cmd(DMA1_Stream6, ENABLE);  // 改为Stream6
}

void Tim4_Configuration(uint32_t Fre)
{	    
    uint32_t period = 0, prescaler = 1;
    
    // APB1时钟频率为42MHz，但定时器时钟为84MHz（因为APB1分频系数不为1时，定时器时钟x2）
    uint32_t timer_clock = 84000000;
    uint32_t target_freq = Fre * DA1_Value_Length; // 实际需要的定时器频率
    uint32_t total_count = timer_clock / target_freq;
    
    // 寻找最佳的分频值组合
    for (prescaler = 1; prescaler <= 65536; prescaler++) {
        period = total_count / prescaler;
        if (period <= 65536 && period > 0) {
            break;
        }
    }
    
    // 边界检查和错误处理
    if (period == 0) period = 1;
    if (period > 65536) period = 65536;
    if (prescaler > 65536) prescaler = 65536;

    // 使能TIM4时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = period - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update);
    TIM_ARRPreloadConfig(TIM4, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}


    

    


    