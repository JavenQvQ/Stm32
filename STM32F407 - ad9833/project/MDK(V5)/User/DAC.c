#include "board.h"




//DAC输出配置
//PA4和DMA1_Stream5_CH7连接
//TIM4触发DAC,需要配置TIM4
//DAC输出频率为fre/DA1_Value_Lenth
void DAC_Configuration(uint16_t *DA1_Value, uint32_t DA1_Value_Length, float amplitude)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    DAC_InitTypeDef DAC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    uint32_t i;
    float amplitude_digital_f; // 使用浮点数变量
    
    // 将幅值从0-3.3V转换为0-4095数字值（使用浮点数计算）
    if(amplitude > 3.3f) amplitude = 3.3f;
    if(amplitude < 0.0f) amplitude = 0.0f;
    amplitude_digital_f = amplitude * 4095.0f / 3.3f;
    
    // 根据幅值调整DA1_Value数组（使用浮点数计算提高精度）
    for(i = 0; i < DA1_Value_Length; i++)
    {
        // 使用浮点数计算，提高精度
        float temp_f = (float)DA1_Value[i] * amplitude_digital_f / 4095.0f;
        DA1_Value[i] = (uint16_t)(temp_f + 0.5f); // 四舍五入
        
        // 确保不超出DAC范围
        if(DA1_Value[i] > 4095) DA1_Value[i] = 4095;
    }
    
    // 使能时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    
    // 配置PA5为模拟输出 (DAC Channel 2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // DMA配置需要更改为Channel 2对应的Stream
    // DAC Channel 2 使用 DMA1_Stream6_CH7
    DMA_Cmd(DMA1_Stream6, DISABLE);
    while(DMA_GetCmdStatus(DMA1_Stream6) != DISABLE);
    
    // 配置DMA
    DMA_DeInit(DMA1_Stream6);
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
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R2;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DA1_Value;  // 修正变量名
    DMA_Init(DMA1_Stream6, &DMA_InitStructure);

    // 配置DAC Channel 2
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T4_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    
   // 设置初始值（根据幅值调整，使用浮点数计算）
    uint16_t initial_value = (uint16_t)(amplitude_digital_f / 2.0f + 0.5f);  // 中间值，四舍五入
    DAC_SetChannel2Data(DAC_Align_12b_R, initial_value);
    
    // 启用DAC和DMA
    DAC_Cmd(DAC_Channel_2, ENABLE);
    DAC_DMACmd(DAC_Channel_2, ENABLE);
    DMA_Cmd(DMA1_Stream6, ENABLE);
}

void Tim4_Configuration(uint32_t Fre, uint32_t DA1_Value_Length)
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


    

    


    