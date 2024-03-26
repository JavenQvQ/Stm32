#include "stm32f10x.h"

void ADC1_Init(uint32_t AddrB)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//ADC时钟设置为PCLK2的6分频，即12MHz
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);//

    GPIO_InitTypeDef GPIO_A;
    GPIO_A.GPIO_Mode=GPIO_Mode_AIN;//模拟输入
    GPIO_A.GPIO_Pin=GPIO_Pin_0;
    GPIO_Init(GPIOA,&GPIO_A);

    ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);//ADC1,通道0,采样时间为55.5个周期
    ADC_InitTypeDef ADC_1;
    ADC_1.ADC_Mode=ADC_Mode_Independent;//独立模式
    ADC_1.ADC_DataAlign=ADC_DataAlign_Right;//右对齐
    ADC_1.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//软件触发
    ADC_1.ADC_NbrOfChannel=1;//1个转换通道
    ADC_1.ADC_ScanConvMode=DISABLE;//非扫描模式
    ADC_1.ADC_ContinuousConvMode=ENABLE;//连续转换模式
    ADC_Init(ADC1,&ADC_1);

    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;//要传输的数据的地址,外设
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AddrB;//存放数据的地址,内存
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//数据传输方向,外设到内存就是由A到B
    DMA_InitStructure.DMA_BufferSize = 1;//数据传输量,给传输寄存器的值
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//要传输的地址不自增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//要储存的地址自增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//内存数据长度为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;//优先级高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//非内存到内存模式+
    DMA_Init(DMA1_Channel1,&DMA_InitStructure);//初始化DMA1的通道1
    DMA_Cmd(DMA1_Channel1,ENABLE);//开启DMA1的通道1
    ADC_Cmd(ADC1,ENABLE);
    ADC_DMACmd(ADC1,ENABLE);//使能ADC1的DMA



    ADC_ResetCalibration(ADC1);//复位校准
    while(ADC_GetResetCalibrationStatus(ADC1)==SET);//等待复位校准结束
    ADC_StartCalibration(ADC1);//开始校准
    while(ADC_GetCalibrationStatus(ADC1)==SET);//等待校准结束
    ADC_SoftwareStartConvCmd(ADC1,ENABLE);//开始转换
}

void TIM1_Init(void)
{
	
}