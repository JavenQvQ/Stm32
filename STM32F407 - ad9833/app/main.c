
#include "board.h"
#include "bsp_uart.h"//使用PA9和PA10
#include "arm_math.h"
#include "OLED.h"//使用PB14SCL PB15SDA
#include "FFT.h"//使用arm_math.h
#include "ADC.h"//使用PB01和定时器3以及DMA2_Stream0
#include "DAC.h"//使用PA05和TIM4以及DMA1_Stream5
#include "ad9220_dcmi.h"//使用DCMI接口和DMA2_Stream1
						//GPIOA: 3个引脚（PA2, PA4, PA6）
						//GPIOB: 3个引脚（PB5, PB6, PB7）
						//GPIOC: 7个引脚（PC6, PC7, PC8, PC9, PC10, PC12）
						//GPIOD: 1个引脚（PD2）
						//GPIOE: 4个引脚（PE4, PE5, PE6, PE14, PE15）
#include "ad9833.h"//使用PB12 PB13 PB14,
#include "key.h"//使用PA0 PE2 PE3 PE4
#include "Gti.h"
#include <stdio.h>
#include "board.h"

extern uint32_t adc_convert_value[ADC_DMA_DATA_LENGTH]; //存储从 DCMI 接收来的数据
extern uint8_t KEY0_interrupt_flag ;
extern uint8_t KEY1_interrupt_flag ;
extern uint8_t KEY2_interrupt_flag ;
extern uint8_t Key_WakeUp_interrupt_flag ;
void Model_0(void); // 模型0函数声明
void Model_1(void); // 模型1函数声明
void Model_2(void); // 模型2函数声明
void Model_3(void); // 模型3函数声明
void Model_4(void); // 模型4函数声明

#define SIN_Value_Length 128  // 增加采样点数提高波形质量
// 标准正弦波数据 (12位DAC，0-4095范围，中心值2048)
uint16_t SIN_Value[SIN_Value_Length];


// 添加DSP优化的常量定义
#define PI_F32                 3.14159265358979f
#define TWO_PI_F32            (2.0f * PI_F32)
#define FILTER_GAIN_F32       5.13f
#define FILTER_C1_F32         1e-8f
#define FILTER_C2_F32         3e-4f
#define TARGET_OUTPUT_V_F32   2.0f
#define MAX_COMPENSATION_F32  3.2f
#define MIN_COMPENSATION_F32  0.1f

// 使用DSP库优化的幅值补偿计算函数
float calculate_amplitude_compensation_precise(uint32_t frequency)
{
    // 使用单精度浮点数，DSP库针对单精度优化
    float32_t omega = TWO_PI_F32 * (float32_t)frequency;
    float32_t omega2;
    
    // 使用DSP库的乘法函数计算ω²
    arm_mult_f32(&omega, &omega, &omega2, 1);
    
    // 计算实部：real_part = 1.0 - 10^-8*ω²
    float32_t real_part_temp = FILTER_C1_F32 * omega2;
    float32_t real_part = 1.0f - real_part_temp;
    
    // 计算虚部：imag_part = 3*10^-4*ω
    float32_t imag_part = FILTER_C2_F32 * omega;
    
    // 计算实部和虚部的平方
    float32_t real_squared, imag_squared;
    arm_mult_f32(&real_part, &real_part, &real_squared, 1);
    arm_mult_f32(&imag_part, &imag_part, &imag_squared, 1);
    
    // 计算平方和
    float32_t sum_of_squares;
    arm_add_f32(&real_squared, &imag_squared, &sum_of_squares, 1);
    
    // 使用DSP库的快速平方根函数
    float32_t magnitude_denominator;
    arm_sqrt_f32(sum_of_squares, &magnitude_denominator);
    
    // 计算传递函数幅值：|H(jω)| = 5.13 / sqrt(...)
    float32_t magnitude = FILTER_GAIN_F32 / magnitude_denominator;
    
    // 计算所需的输入幅值：required_input = 2.0 / magnitude
    float32_t required_input = TARGET_OUTPUT_V_F32 / magnitude;
    
    // 使用DSP库的限幅函数
    float32_t result;
    if(required_input > MAX_COMPENSATION_F32) 
        result = MAX_COMPENSATION_F32;
    else if(required_input < MIN_COMPENSATION_F32) 
        result = MIN_COMPENSATION_F32;
    else 
        result = required_input;
    
    return result;
}

// 优化的正弦波表生成函数
void generate_sin_table(float amp)
{
    // 限制幅值范围
    if(amp > 3.3f) amp = 3.3f;
    if(amp < 0.0f) amp = 0.0f;
    
    // 使用DSP库的常量和函数
    const float32_t dac_scale = 4095.0f / 3.3f;
    const float32_t angle_step = TWO_PI_F32 / (float32_t)SIN_Value_Length;
    const float32_t center_voltage = 1.65f;
    const float32_t peak_voltage = amp * 0.5f;
    
    float32_t angles[SIN_Value_Length];
    float32_t sin_values[SIN_Value_Length];
    
    // 生成角度数组
    for(int i = 0; i < SIN_Value_Length; i++)
    {
        angles[i] = angle_step * (float32_t)i;
    }
    
    // 使用DSP库批量计算正弦值（如果可用）
    // 注意：某些版本的CMSIS-DSP库可能没有arm_sin_f32批量函数
    // 这里使用循环调用单个sin函数
    for(int i = 0; i < SIN_Value_Length; i++)
    {
        // 使用标准库的sinf函数（单精度）
        sin_values[i] = sinf(angles[i]);
        
        // 计算电压值
        float32_t voltage = center_voltage + sin_values[i] * peak_voltage;
        
        // 限制电压范围
        if(voltage > 3.3f) voltage = 3.3f;
        if(voltage < 0.0f) voltage = 0.0f;
        
        // 转换为DAC值
        float32_t dac_float = voltage * dac_scale + 0.5f;
        uint16_t dac_value = (uint16_t)dac_float;
        
        // 确保值在有效范围内
        if(dac_value > 4095) dac_value = 4095;
        
        SIN_Value[i] = dac_value;
    }
}



extern uint16_t model; // 模型选择变量	


//主函数
int main(void)
{
	board_init();
	OLED_Init();
	uart1_init(115200U);
	EXTI_Key_Init();//按键初始化
	OLED_Init(); // OLED初始化
	OLED_Clear(); // 清屏
    //AD9220_DCMIDMA_Config();//AD9220初始化
	AD9833_Init_GPIO();//AD9833初始化GPIO

	
	while(1)
	{

		switch (model)
		{
			case 0:
				OLED_Clear(); // 清屏
				OLED_ShowString(16, 0, "Model: 0", 16, 1); // 显示当前模型
				OLED_Refresh(); // 刷新OLED显示
				Model_0(); // 调用模型0函数
				break;
			case 1:
				Model_1(); // 调用模型1函数
				break;
			case 2:
				Model_2(); // 调用模型2函数
				break;
			case 3:
				Model_3(); // 调用模型3函数
				break;
            case 4:
                Model_4(); // 调用模型4函数
                break;
			default:
				break;
		}

	}
}





void Model_0(void)
{
	while(1)
	{
		delay_ms(100); // 延时100毫秒
		if(Key_WakeUp_interrupt_flag)
		{
			Key_WakeUp_interrupt_flag = 0; // 清除中断标志
			printf("Exit Model_0\r\n");
			model = 1; // 切换到模型1
			return; // 退出函数
		}
	}
}



void Model_1(void)
{
    static uint32_t current_frequency = 3000;  // 当前频率，初始3kHz
    static uint32_t frequency_step = 100;      // 频率步长，初始100Hz
    static uint32_t max_frequency = 5000000;   // 最大频率5MHz
    static uint32_t min_frequency = 100;       // 最小频率100Hz
    // 步长选择（0=100Hz, 1=1kHz, 2=10kHz）
    static uint8_t step_index = 0;
    static uint32_t step_values[3] = {100, 1000, 10000};
    static uint8_t initialized = 0;            // 初始化标志
    static uint8_t display_updated = 0;        // 显示更新标志
    
    // 首次运行时初始化AD9833
    if (!initialized)
    {
        AD9833_WaveSeting(current_frequency, 0, SIN_WAVE, 0);
        AD9833_AmpSet(150);
  
        // 初始化显示，只在第一次进入时清屏
        OLED_Clear();
        OLED_ShowString(0, 0, "=== Model 1 ===", 16, 1);
        OLED_ShowString(0, 16, "Freq:", 16, 1);
        OLED_ShowString(0, 32, "Step:", 16, 1);
        OLED_ShowString(0, 48, "K2:+ K0:- K1:Step", 12, 1);
        display_updated = 1; // 标记需要更新显示
        initialized = 1;
    }
    
    while(1)
    {
        // 只有在需要更新时才重新显示频率和步长
        if(display_updated)
        {
            // 扩大清除频率显示区域，确保完全清除
            OLED_ShowString(40, 16, "                ", 16, 1); // 16个空格，足够清除所有内容
		// 重新设计频率显示逻辑，统一使用固定起始位置
			if(current_frequency >= 1000000) {
			// MHz显示 - 1MHz及以上
			uint32_t mhz = current_frequency / 1000000;
			uint32_t khz_remainder = (current_frequency % 1000000) / 1000;
			
			if(khz_remainder == 0) {
				// 整数MHz显示，例如：1MHz, 2MHz
				OLED_ShowNum(56, 16, mhz, 1, 16, 1);
				OLED_ShowString(64, 16, "MHz", 16, 1);
			} else {
				// 带小数的MHz显示，例如：1.500MHz
				OLED_ShowNum(40, 16, mhz, 1, 16, 1);
				OLED_ShowString(48, 16, ".", 16, 1);
				OLED_ShowNum(56, 16, khz_remainder, 3, 16, 1);
				OLED_ShowString(80, 16, "MHz", 16, 1);
			}
		} else if(current_frequency >= 1000) {
			// kHz显示 - 1kHz 到 999kHz
			uint32_t khz = current_frequency / 1000;
			uint32_t hz_remainder = current_frequency % 1000;
			
			if(hz_remainder == 0) {
				// 整数kHz显示，例如：1kHz, 10kHz, 100kHz
				if(khz >= 100) {
					// 100-999 kHz：使用3位数字，起始位置40
					OLED_ShowNum(40, 16, khz, 3, 16, 1);
					OLED_ShowString(64, 16, "kHz", 16, 1);
				} else if(khz >= 10) {
					// 10-99 kHz：使用2位数字，起始位置48（居中对齐）
					OLED_ShowNum(48, 16, khz, 2, 16, 1);
					OLED_ShowString(64, 16, "kHz", 16, 1);
				} else {
					// 1-9 kHz：使用1位数字，起始位置56（居中对齐）
					OLED_ShowNum(56, 16, khz, 1, 16, 1);
					OLED_ShowString(64, 16, "kHz", 16, 1);
				}
			} else {
				// 带小数的kHz显示，例如：1.500kHz
				if(khz >= 100) {
					// 100+ kHz 带小数
					OLED_ShowNum(40, 16, khz, 3, 16, 1);
					OLED_ShowString(64, 16, ".", 16, 1);
					OLED_ShowNum(72, 16, hz_remainder, 3, 16, 1);
					OLED_ShowString(96, 16, "kHz", 16, 1);
				} else if(khz >= 10) {
					// 10+ kHz 带小数
					OLED_ShowNum(48, 16, khz, 2, 16, 1);
					OLED_ShowString(64, 16, ".", 16, 1);
					OLED_ShowNum(72, 16, hz_remainder, 3, 16, 1);
					OLED_ShowString(96, 16, "kHz", 16, 1);
				} else {
					// 1-9 kHz 带小数
					OLED_ShowNum(56, 16, khz, 1, 16, 1);
					OLED_ShowString(64, 16, ".", 16, 1);
					OLED_ShowNum(72, 16, hz_remainder, 3, 16, 1);
					OLED_ShowString(96, 16, "kHz", 16, 1);
				}
			}
			} else {
				// Hz显示（100-999Hz）
				OLED_ShowNum(40, 16, current_frequency, 3, 16, 1);
				OLED_ShowString(64, 16, "Hz ", 16, 1);
			}
            // 扩大清除步长显示区域
            OLED_ShowString(40, 32, "                ", 16, 1); // 16个空格
            
            // 显示步长信息 - 简化逻辑
            if(frequency_step >= 1000) {
                uint32_t step_khz = frequency_step / 1000;
                if(step_khz >= 10) {
                    OLED_ShowNum(40, 32, step_khz, 2, 16, 1);
                    OLED_ShowString(56, 32, "kHz", 16, 1);
                } else {
                    OLED_ShowNum(40, 32, step_khz, 1, 16, 1);
                    OLED_ShowString(48, 32, "kHz", 16, 1);
                }
            } else {
                OLED_ShowNum(40, 32, frequency_step, 3, 16, 1);
                OLED_ShowString(64, 32, "Hz ", 16, 1);
            }
            
            OLED_Refresh(); // 只在更新后刷新一次
            display_updated = 0; // 重置更新标志
        }
        
        // KEY2: 频率增加
        if(KEY2_interrupt_flag)
        {
            KEY2_interrupt_flag = 0; // 清除中断标志
            
            if(current_frequency + frequency_step <= max_frequency)
            {
                current_frequency += frequency_step;
                AD9833_WaveSeting(current_frequency, 0, SIN_WAVE, 0);
                display_updated = 1; // 标记需要更新显示
                printf("Frequency increased to: %lu Hz, Step: %lu Hz\r\n", 
                       current_frequency, frequency_step);
            }
            else
            {
                printf("Maximum frequency reached: %lu Hz (5MHz)\r\n", max_frequency);
            }
        }
        
        // KEY0: 频率减少
        if(KEY0_interrupt_flag)
        {
            KEY0_interrupt_flag = 0; // 清除中断标志
            
            if(current_frequency >= min_frequency + frequency_step)
            {
                current_frequency -= frequency_step;
                AD9833_WaveSeting(current_frequency, 0, SIN_WAVE, 0);
                display_updated = 1; // 标记需要更新显示
                printf("Frequency decreased to: %lu Hz, Step: %lu Hz\r\n", 
                       current_frequency, frequency_step);
            }
            else
            {
                printf("Minimum frequency reached: %lu Hz\r\n", min_frequency);
            }
        }
        
        // KEY1: 切换步长
        if(KEY1_interrupt_flag)
        {
            KEY1_interrupt_flag = 0; // 清除中断标志
            
            step_index = (step_index + 1) % 3; // 循环切换0,1,2
            frequency_step = step_values[step_index];
            display_updated = 1; // 标记需要更新显示
            
            const char* step_names[3] = {"100Hz", "1kHz", "10kHz"};
            printf("Step changed to: %s (%lu Hz)\r\n", 
                   step_names[step_index], frequency_step);
        }
        
        // 退出条件：使用WakeUp按键切换模式
        if(Key_WakeUp_interrupt_flag)
        {
            Key_WakeUp_interrupt_flag = 0; // 清除中断标志
            printf("Exit Model_1\r\n");
            model = 2; // 切换到模型2
			initialized=0;
            return; // 退出函数
        }

    }
}

void Model_2(void)
{
	generate_sin_table(0.775f); // 生成正弦波表，幅值为0.775V
	DAC_Configuration(SIN_Value, SIN_Value_Length);//DAC配置，使用正弦波数据
	Tim4_Configuration(1000, SIN_Value_Length);//配置定时器4，频率1000Hz，采样点数为SIN_Value_Length
	OLED_Clear(); // 清屏
	OLED_ShowString(0, 0, "Model: 2", 16, 1); // 显示当前模型
	OLED_ShowString(0, 16, "Freq: 1000Hz", 16, 1); // 显示频率
	OLED_ShowString(0, 32, "Amp: 2.0V", 16, 1); // 显示幅度
	OLED_Refresh(); // 刷新OLED显示
	while(1)
	{
		delay_ms(100); // 延时100ms，避免过快刷新
		if(Key_WakeUp_interrupt_flag)
		{
			Key_WakeUp_interrupt_flag = 0; // 清除中断标志
			printf("Exit Model_2\r\n");
			model = 3; // 切换到模型3
			return; // 退出函数
		}
	}
	
}



// 在Model_3函数中，修改switch (control_mode)部分：

void Model_3(void)
{
    static uint32_t dac_frequency = 100;      // DAC输出频率，初始100Hz
    static uint32_t frequency_step = 100;     // 频率步长，固定100Hz
    static uint32_t max_frequency = 3000;     // 最大频率3kHz
    static uint32_t min_frequency = 100;      // 最小频率100Hz
    
    // 电压控制变量
    static float target_voltage = 2.0f;      // 目标输出电压峰峰值，初始2V
    static float voltage_step = 0.1f;        // 电压步长0.1V
    static float max_voltage = 2.0f;         // 最大电压2V
    static float min_voltage = 1.0f;         // 最小电压1V
    
    static uint8_t control_mode = 0;         // 控制模式：0=电压控制，1=频率控制
    static uint8_t initialized = 0;         // 初始化标志
    
    OLED_Clear(); // 清屏
    
    // 首次运行时初始化DAC
    if (!initialized)
    {
        // 使用高精度计算函数
        double amplitude_compensation = calculate_amplitude_compensation_precise(dac_frequency);
        
        // 使用高精度正弦波表生成
        generate_sin_table(amplitude_compensation);
        
        DAC_Configuration(SIN_Value, SIN_Value_Length);
        Tim4_Configuration(dac_frequency * SIN_Value_Length, 1);
        
        printf("Model_3 initialized (High Precision). Mode: VOLTAGE control\r\n");
        printf("Freq: %lu Hz, Target: %.1fV, Input: %.6fV\r\n", 
               dac_frequency, target_voltage, amplitude_compensation);
        printf("KEY1: switch mode, KEY0/KEY2: adjust value, WakeUp: exit\r\n");
        initialized = 1;
    }
    
    OLED_ShowString(0, 0, "Model: 3", 16, 1); // 显示当前模型
    
    while(1)
    {
        // 清除显示区域，避免字符重叠
        OLED_ShowString(0, 0, "Model: 3        ", 16, 1); // 显示当前模型
        
        // KEY1: 切换控制模式（电压/频率）
        if(KEY1_interrupt_flag)
        {
            KEY1_interrupt_flag = 0; // 清除中断标志
            control_mode = !control_mode; // 切换模式
        }
        
        if(control_mode == 0)
        {
            OLED_ShowString(0, 16, "Mode: Voltage   ", 16, 1);
        }
        else
        {
            OLED_ShowString(0, 16, "Mode: Frequency ", 16, 1);
        }
        
        // 根据控制模式处理按键
        switch (control_mode)
        {
            case 0: // 电压控制模式
            {
                // KEY2: 增加电压
                if(KEY2_interrupt_flag)
                {
                    KEY2_interrupt_flag = 0;
                    
                    if(target_voltage + voltage_step <= max_voltage + 0.001f) // 浮点数比较容错
                    {
                        target_voltage += voltage_step;
                        
                        // 使用高精度计算函数，基于新的目标电压重新计算
                        double amplitude_compensation = ((double)target_voltage / 2.0) * calculate_amplitude_compensation_precise(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2) amplitude_compensation = 3.2;
                        if(amplitude_compensation < 0.05) amplitude_compensation = 0.05;
                        
                        // 重新生成高精度正弦波表
                        generate_sin_table(amplitude_compensation);
                        
                        printf("Voltage+: %.1fV, Freq: %lu Hz, Input: %.6fV\r\n", 
                               target_voltage, dac_frequency, amplitude_compensation);
                    }
                    else
                    {
                        printf("Maximum voltage reached: %.1fV\r\n", max_voltage);
                    }
                }
                
                // KEY0: 减少电压
                if(KEY0_interrupt_flag)
                {
                    KEY0_interrupt_flag = 0;
                    
                    if(target_voltage >= min_voltage + voltage_step - 0.001f) // 浮点数比较容错
                    {
                        target_voltage -= voltage_step;
                        
                        // 使用高精度计算函数，基于新的目标电压重新计算
                        double amplitude_compensation = ((double)target_voltage / 2.0) * calculate_amplitude_compensation_precise(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2) amplitude_compensation = 3.2;
                        if(amplitude_compensation < 0.05) amplitude_compensation = 0.05;
                        
                        // 重新生成高精度正弦波表
                        generate_sin_table(amplitude_compensation);
                        
                        printf("Voltage-: %.1fV, Freq: %lu Hz, Input: %.6fV\r\n", 
                               target_voltage, dac_frequency, amplitude_compensation);
                    }
                    else
                    {
                        printf("Minimum voltage reached: %.1fV\r\n", min_voltage);
                    }
                }
                break;
            }
            
            case 1: // 频率控制模式
            {
                // KEY2: 增加频率
                if(KEY2_interrupt_flag)
                {
                    KEY2_interrupt_flag = 0;
                    
                    if(dac_frequency + frequency_step <= max_frequency)
                    {
                        dac_frequency += frequency_step;
                        
                        // 使用高精度计算函数
                        double amplitude_compensation = ((double)target_voltage / 2.0) * calculate_amplitude_compensation_precise(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2) amplitude_compensation = 3.2;
                        if(amplitude_compensation < 0.05) amplitude_compensation = 0.05;
                        
                        // 重新生成高精度正弦波表和配置定时器
                        generate_sin_table(amplitude_compensation);
                        Tim4_Configuration(dac_frequency * SIN_Value_Length, 1);
                        
                        printf("Freq+: %lu Hz, Target: %.1fV, Input: %.6fV\r\n", 
                               dac_frequency, target_voltage, amplitude_compensation);
                    }
                    else
                    {
                        printf("Maximum frequency reached: %lu Hz\r\n", max_frequency);
                    }
                }
                
                // KEY0: 减少频率
                if(KEY0_interrupt_flag)
                {
                    KEY0_interrupt_flag = 0;
                    
                    if(dac_frequency >= min_frequency + frequency_step)
                    {
                        dac_frequency -= frequency_step;
                        
                        // 使用高精度计算函数
                        double amplitude_compensation = ((double)target_voltage / 2.0) * calculate_amplitude_compensation_precise(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2) amplitude_compensation = 3.2;
                        if(amplitude_compensation < 0.05) amplitude_compensation = 0.05;
                        
                        // 重新生成高精度正弦波表和配置定时器
                        generate_sin_table(amplitude_compensation);
                        Tim4_Configuration(dac_frequency * SIN_Value_Length, 1);
                        
                        printf("Freq-: %lu Hz, Target: %.1fV, Input: %.6fV\r\n", 
                               dac_frequency, target_voltage, amplitude_compensation);
                    }
                    else
                    {
                        printf("Minimum frequency reached: %lu Hz\r\n", min_frequency);
                    }
                }
                break;
            }
            
            default:
                break;
        }
        
        // 显示频率 - 修正显示位置和格式
        OLED_ShowString(0, 32, "Freq:", 16, 1);
        OLED_ShowNum(64, 32, dac_frequency, 4, 16, 1); // 显示频率数值
        OLED_ShowString(96, 32, "Hz  ", 16, 1); // 显示单位
        
        // 显示目标电压 - 修正小数显示计算
        OLED_ShowString(0, 48, "Volt:", 16, 1);
        // 修正浮点数转换逻辑，避免精度问题
        uint16_t voltage_display = (uint16_t)((target_voltage * 10.0f) + 0.5f); // 四舍五入
        uint16_t voltage_integer = voltage_display / 10;
        uint16_t voltage_decimal = voltage_display % 10;
        
        OLED_ShowNum(64, 48, voltage_integer, 1, 16, 1); // 整数部分
        OLED_ShowString(80, 48, ".", 16, 1);             // 小数点
        OLED_ShowNum(88, 48, voltage_decimal, 1, 16, 1); // 小数部分
        OLED_ShowString(104, 48, "V  ", 16, 1);          // 单位        // 单位
        OLED_Refresh(); // 刷新OLED显示

        // 退出条件：使用WakeUp按键
        if(Key_WakeUp_interrupt_flag)
        {
            Key_WakeUp_interrupt_flag = 0; // 清除中断标志
            printf("Exit Model_3\r\n");
            model = 0; // 切换到模型0
            
            // 停止DAC输出
            TIM_Cmd(TIM4, DISABLE);
            DAC_Cmd(DAC_Channel_2, DISABLE);
            return; // 退出函数
        }
    }
}

