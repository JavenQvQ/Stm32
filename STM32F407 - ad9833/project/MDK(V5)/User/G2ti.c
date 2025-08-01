#ifndef PI
#define PI 3.14159265358979323846f
#endif
#include "board.h"
#include "ADC.h"
#include "AD9833.h"
#include "OLED.h"
#include "stm32f4xx.h"
#include "AD9220_DCMI.h"
#include "Gti.h"
#include "KEY.h"
#include "arm_math.h" // 引入CMSIS DSP库
#include "RLC.h"     // 引入RLC分析库
#include <stdio.h>


extern uint8_t KEY0_interrupt_flag ;
extern uint8_t KEY1_interrupt_flag ;
extern uint8_t KEY2_interrupt_flag ;
extern uint8_t Key_WakeUp_interrupt_flag ;
extern uint16_t AD9833_Value[ADC1_DMA_Size];
extern uint16_t RLC_Value[ADC1_DMA_Size];
extern uint16_t model; // 模型选择变量
// 窗函数类型枚举定义
