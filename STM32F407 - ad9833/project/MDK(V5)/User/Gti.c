#ifndef PI
#define PI 3.14159265358979323846f
#endif
#include "board.h"
#include "DAC.h"
#include "ADC.h"
#include "AD9833.h"
#include "OLED.h"
#include "stm32f4xx.h"
#include "AD9220_DCMI.h"
#include "key.h"
#include "arm_math.h" // 引入CMSIS DSP库
#include <stdio.h>

extern uint16_t AD9833_Value[ADC1_DMA_Size];
extern uint16_t RLC_Value[ADC1_DMA_Size];

// 窗函数类型枚举定义
typedef enum {
    WINDOW_RECTANGULAR = 0,  // 矩形窗
    WINDOW_HANNING,          // 汉宁窗
    WINDOW_HAMMING,          // 海明窗
    WINDOW_BLACKMAN,         // 布莱克曼窗
    WINDOW_BLACKMAN_HARRIS,  // 布莱克曼-哈里斯窗
    WINDOW_KAISER            // 凯泽窗
} window_type_t;

// 窗函数相干增益表（用于幅度校正）
const float32_t window_coherent_gains[] = {
    1.0f,      // 矩形窗
    0.5f,      // 汉宁窗
    0.54f,     // 海明窗
    0.42f,     // 布莱克曼窗
    0.35875f,  // 布莱克曼-哈里斯窗
    0.5f       // 凯泽窗（近似值）
};

// FFT相关定义
#ifndef FFT_SIZE
#define FFT_SIZE 1024
#endif

// 增强的频率分段配置结构
typedef struct {
    uint32_t freq_start;        // 频率段起始频率
    uint32_t freq_end;          // 频率段结束频率
    uint32_t adc_sample_rate;   // ADC采样频率（2的n次方）
    uint32_t adc_sample_time;   // ADC采样时间（转换周期）
    uint8_t min_cycles;         // 最少采样周期数
    uint16_t settle_time_ms;    // 信号稳定时间(ms)
    uint16_t acquisition_time_ms; // 数据采集时间(ms)
    float32_t snr_threshold;    // 信噪比门限
} freq_segment_config_t;

// 优化的频率分段表 - 平衡ADC精度、采样率和FFT精度
const freq_segment_config_t g_freq_segments[] = {
    // 超低频段: 100Hz - 500Hz - 高精度慢速采样
    {100,    500,    16384,   ADC_SampleTime_144Cycles, 25, 250, 400, 15.0f},
    
    // 低频段: 500Hz - 1kHz - 保持高精度
    {500,    1000,   32768,   ADC_SampleTime_144Cycles, 20, 200, 300, 20.0f},
    
    // 中低频段: 1kHz - 2kHz - 平衡精度和速度
    {1000,   2000,   65536,   ADC_SampleTime_84Cycles,  15, 150, 250, 25.0f},
    
    // 中频段: 2kHz - 5kHz - 适中配置
    {2000,   5000,   131072,  ADC_SampleTime_84Cycles,  12, 120, 200, 25.0f},
    
    // 中高频段: 5kHz - 15kHz - 提高采样率
    {5000,   15000,  262144,  ADC_SampleTime_56Cycles,  10, 100, 150, 20.0f},
    
    // 高频段: 15kHz - 30kHz - 保证采样定理
    {15000,  30000,  262144,  ADC_SampleTime_56Cycles,  8,  80,  120, 15.0f},
    
    // 超高频段: 30kHz - 60kHz - 更高采样率
    {30000,  60000,  524288,  ADC_SampleTime_28Cycles,  6,  60,  100, 10.0f},
    
    // 极高频段: 60kHz - 100kHz - 最高采样率，较短转换时间
    {60000,  100000, 524288,  ADC_SampleTime_28Cycles,  5,  50,  80,  8.0f},
};

#define FREQ_SEGMENT_COUNT (sizeof(g_freq_segments) / sizeof(g_freq_segments[0]))

// 获取指定频率对应的配置段
const freq_segment_config_t* get_freq_segment_config(uint32_t frequency)
{
    for(uint8_t i = 0; i < FREQ_SEGMENT_COUNT; i++)
    {
        if(frequency >= g_freq_segments[i].freq_start && frequency <= g_freq_segments[i].freq_end)
        {
            return &g_freq_segments[i];
        }
    }
    // 默认返回中频段配置
    return &g_freq_segments[3];
}

// 全局FFT缓冲区
static float32_t fft_input_buffer[FFT_SIZE * 2];  // 复数FFT输入缓冲区
static float32_t fft_output_buffer[FFT_SIZE];     // FFT幅度输出缓冲区
static arm_rfft_fast_instance_f32 fft_instance;   // FFT实例
// 全局变量用于相位连续性跟踪
static float32_t g_last_valid_phase = 0.0f;
static uint32_t g_last_valid_freq = 0;
static uint8_t g_phase_tracking_enabled = 0;
static float32_t g_phase_history[5] = {0}; // 相位历史记录
static uint8_t g_phase_history_index = 0;
static uint8_t g_phase_history_count = 0;

// 测量结果存储数组
static float32_t g_magnitude_array[100];    // 幅值数组
static float32_t g_gain_db_array[100];      // 增益数组(dB)
static float32_t g_phase_diff_array[100];   // 相位差数组(度)

// 获取窗函数相干增益
float32_t get_window_coherent_gain(window_type_t window_type)
{
    if(window_type < sizeof(window_coherent_gains)/sizeof(window_coherent_gains[0]))
    {
        return window_coherent_gains[window_type];
    }
    return 1.0f; // 默认值
}

// 应用窗函数
void apply_window_function(float32_t* data, uint16_t size, window_type_t window_type)
{
    switch(window_type)
    {
        case WINDOW_HANNING:
            for(uint16_t i = 0; i < size; i++)
            {
                float32_t window_val = 0.5f * (1.0f - arm_cos_f32(2.0f * PI * i / (size - 1)));
                data[i] *= window_val;
            }
            break;
            
        case WINDOW_HAMMING:
            for(uint16_t i = 0; i < size; i++)
            {
                float32_t window_val = 0.54f - 0.46f * arm_cos_f32(2.0f * PI * i / (size - 1));
                data[i] *= window_val;
            }
            break;
            
        case WINDOW_BLACKMAN:
            for(uint16_t i = 0; i < size; i++)
            {
                float32_t window_val = 0.42f - 0.5f * arm_cos_f32(2.0f * PI * i / (size - 1)) + 
                                      0.08f * arm_cos_f32(4.0f * PI * i / (size - 1));
                data[i] *= window_val;
            }
            break;
            
        case WINDOW_BLACKMAN_HARRIS:
            for(uint16_t i = 0; i < size; i++)
            {
                float32_t a0 = 0.35875f;
                float32_t a1 = 0.48829f;
                float32_t a2 = 0.14128f;
                float32_t a3 = 0.01168f;
                float32_t window_val = a0 - a1 * arm_cos_f32(2.0f * PI * i / (size - 1)) +
                                      a2 * arm_cos_f32(4.0f * PI * i / (size - 1)) -
                                      a3 * arm_cos_f32(6.0f * PI * i / (size - 1));
                data[i] *= window_val;
            }
            break;
            
        case WINDOW_RECTANGULAR:
        default:
            // 矩形窗不需要处理，数据保持原样
            break;
    }
}

// 初始化FFT实例
void init_fft_instance(void)
{
    arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
}

// 高精度单通道FFT处理函数
void process_single_channel_fft_optimized(uint16_t* adc_data, uint32_t sample_rate, 
                                         uint32_t target_freq, window_type_t window_type,
                                         float32_t* magnitude, float32_t* phase, float32_t* snr)
{
    // 初始化默认返回值
    *magnitude = 0.0f;
    *phase = 0.0f;
    *snr = 0.0f;
    
    // 数据预处理：ADC数据转换为浮点数并去除直流分量
    float32_t dc_offset = 0.0f;
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        fft_input_buffer[i] = (float32_t)adc_data[i];
        dc_offset += fft_input_buffer[i];
    }
    dc_offset /= FFT_SIZE;
    
    // 去除直流分量
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        fft_input_buffer[i] -= dc_offset;
    }
    
        // 数据有效性检查 - 增强稳定性
        float32_t max_val = -65535, min_val = 65535;
        float32_t rms_val = 0.0f;
        for(uint16_t i = 0; i < FFT_SIZE; i++)
        {
            if(fft_input_buffer[i] > max_val) max_val = fft_input_buffer[i];
            if(fft_input_buffer[i] < min_val) min_val = fft_input_buffer[i];
            rms_val += fft_input_buffer[i] * fft_input_buffer[i];
        }
        rms_val = sqrtf(rms_val / FFT_SIZE);
        
        // 更严格的数据有效性检查
        if((max_val - min_val) < 10.0f || rms_val < 5.0f) {
            return; // 数据无效或信号太弱
        }    // 应用窗函数
    apply_window_function(fft_input_buffer, FFT_SIZE, window_type);
    
    // 执行实数FFT
    arm_rfft_fast_f32(&fft_instance, fft_input_buffer, fft_input_buffer, 0);
    
    // 计算幅度谱
    arm_cmplx_mag_f32(fft_input_buffer, fft_output_buffer, FFT_SIZE / 2);
    
    // 计算目标频率对应的FFT bin
    float32_t freq_resolution = (float32_t)sample_rate / FFT_SIZE;
    uint16_t target_bin = (uint16_t)((float32_t)target_freq / freq_resolution + 0.5f);
    
    // 边界检查
    if(target_bin >= FFT_SIZE / 2) {
        target_bin = FFT_SIZE / 2 - 1;
    }
    
    // 获取目标频率的幅度和相位
    if(target_bin > 0 && target_bin < FFT_SIZE / 2)
    {
        float32_t real = fft_input_buffer[target_bin * 2];
        float32_t imag = fft_input_buffer[target_bin * 2 + 1];
        
        // 计算幅度
        float32_t raw_magnitude = sqrtf(real * real + imag * imag);
        float32_t window_gain = get_window_coherent_gain(window_type);
        *magnitude = raw_magnitude / (window_gain * FFT_SIZE / 2.0f);
        
        // 高精度相位计算 - 增强稳定性
        float32_t phase_rad = atan2f(imag, real);
        
        // 检查复数值的有效性
        if(fabsf(real) < 1e-6f && fabsf(imag) < 1e-6f) {
            return; // 信号太弱，无法准确计算相位
        }
        
        // 计算信号的信噪比以评估相位可靠性
        float32_t signal_magnitude = sqrtf(real * real + imag * imag);
        
        // 频率插值校正相位 - 增加稳定性检查
        if(target_bin > 1 && target_bin < FFT_SIZE / 2 - 1 && signal_magnitude > 100.0f)
        {
            // 获取相邻bin的复数值
            float32_t real_prev = fft_input_buffer[(target_bin - 1) * 2];
            float32_t imag_prev = fft_input_buffer[(target_bin - 1) * 2 + 1];
            float32_t real_next = fft_input_buffer[(target_bin + 1) * 2];
            float32_t imag_next = fft_input_buffer[(target_bin + 1) * 2 + 1];
            
            // 计算相邻bin的幅度
            float32_t mag_prev = sqrtf(real_prev * real_prev + imag_prev * imag_prev);
            float32_t mag_curr = signal_magnitude;
            float32_t mag_next = sqrtf(real_next * real_next + imag_next * imag_next);
            
            // 检查是否为有效的峰值（中心bin应该是最大的）
            if(mag_curr > mag_prev && mag_curr > mag_next && mag_curr > mag_prev * 1.2f && mag_curr > mag_next * 1.2f)
            {
                // 使用抛物线插值找到真实峰值位置
                float32_t delta = 0.0f;
                float32_t denom = 2.0f * (2.0f * mag_curr - mag_prev - mag_next);
                if(fabsf(denom) > 1e-6f)
                {
                    delta = (mag_next - mag_prev) / denom;
                    // 限制插值范围
                    if(delta > 0.5f) delta = 0.5f;
                    if(delta < -0.5f) delta = -0.5f;
                }
                
                // 相位插值校正 - 只在delta足够大时进行
                if(fabsf(delta) > 0.05f && fabsf(delta) < 0.5f)
                {
                    float32_t phase_prev = atan2f(imag_prev, real_prev);
                    float32_t phase_next = atan2f(imag_next, real_next);
                    
                    // 处理相位缠绕
                    while(phase_prev - phase_rad > PI) phase_prev -= 2.0f * PI;
                    while(phase_prev - phase_rad < -PI) phase_prev += 2.0f * PI;
                    while(phase_next - phase_rad > PI) phase_next -= 2.0f * PI;
                    while(phase_next - phase_rad < -PI) phase_next += 2.0f * PI;
                    
                    // 检查相位连续性
                    float32_t phase_diff_prev = fabsf(phase_prev - phase_rad);
                    float32_t phase_diff_next = fabsf(phase_next - phase_rad);
                    
                    // 只有在相位变化合理时才进行插值
                    if(phase_diff_prev < PI/2 && phase_diff_next < PI/2)
                    {
                        // 线性插值校正相位
                        if(delta > 0)
                        {
                            phase_rad = phase_rad + delta * (phase_next - phase_rad);
                        }
                        else
                        {
                            phase_rad = phase_rad + delta * (phase_rad - phase_prev);
                        }
                    }
                }
            }
        }
        
        // 基于测量和理论分析的相位延迟补偿
        const freq_segment_config_t* current_config = get_freq_segment_config(target_freq);
        
        // 1. ADC转换延迟（基于STM32F407数据手册）
        uint16_t actual_adc_cycles = 56; // 默认值
        switch(current_config->adc_sample_time)
        {
            case ADC_SampleTime_28Cycles:  actual_adc_cycles = 28;  break;
            case ADC_SampleTime_56Cycles:  actual_adc_cycles = 56;  break;
            case ADC_SampleTime_84Cycles:  actual_adc_cycles = 84;  break;
            case ADC_SampleTime_144Cycles: actual_adc_cycles = 144; break;
            default: actual_adc_cycles = 56; break;
        }
        
        // ADC转换总时间 = 采样时间 + 12个转换周期（STM32F407数据手册）
        float32_t adc_total_cycles = (float32_t)(actual_adc_cycles + 12);
        float32_t adc_clock_freq = 42000000.0f; // 42MHz ADC时钟
        float32_t adc_conversion_time_s = adc_total_cycles / adc_clock_freq;
        
        // 2. 系统固有延迟（基于实际测量或厂商数据）
        float32_t system_delay_s = 0.0f;
        
        // DMA传输延迟（约2-3个系统时钟周期）
        float32_t dma_delay_s = 3.0f / 168000000.0f; // 3个168MHz系统时钟周期
        
        // 双ADC同步偏差（硬件设计决定，通常<1个ADC时钟周期）
        float32_t adc_sync_offset_s = 0.5f / adc_clock_freq;
        
        system_delay_s = dma_delay_s + adc_sync_offset_s;
        
        // 3. 总的相位延迟计算
        float32_t total_delay_s = adc_conversion_time_s + system_delay_s;
        
        // 相位延迟 = -2π × f × delay（负号表示延迟导致的相位滞后）
        float32_t phase_delay_correction = -2.0f * PI * target_freq * total_delay_s;
        
        // 应用相位校正
        phase_rad += phase_delay_correction;
        
        
        // 转换为度并规范化到[-180, 180]
        *phase = phase_rad * 180.0f / PI;
        while(*phase > 180.0f) *phase -= 360.0f;
        while(*phase < -180.0f) *phase += 360.0f;
        
        // 计算信噪比
        float32_t signal_power = raw_magnitude * raw_magnitude;
        float32_t noise_power = 0.0f;
        uint16_t noise_count = 0;
        
        // 估算噪声功率（使用目标频率附近几个bin之外的功率）
        for(uint16_t i = 1; i < FFT_SIZE / 2 - 1; i++)
        {
            if(abs((int)i - (int)target_bin) > 3)  // 远离信号频率
            {
                float32_t noise_real = fft_input_buffer[i * 2];
                float32_t noise_imag = fft_input_buffer[i * 2 + 1];
                noise_power += noise_real * noise_real + noise_imag * noise_imag;
                noise_count++;
            }
        }
        
        if(noise_count > 0)
        {
            noise_power /= noise_count; // 平均噪声功率
            if(noise_power > 1e-12f)
            {
                *snr = 10.0f * log10f(signal_power / noise_power);
            }
            else
            {
                *snr = 100.0f; // 非常高的信噪比
            }
        }
        else
        {
            *snr = 0.0f;
        }
    }
}

const uint32_t g_log_freq_points[100] = {
    100,    107,    115,    123,    132,    142,    152,    163,    175,    187,
    201,    215,    231,    248,    266,    285,    305,    327,    351,    376,
    404,    433,    464,    498,    534,    572,    614,    658,    705,    756,
    811,    870,    933,    1000,    1072,    1150,    1233,    1322,    1417,    1520,
    1630,    1748,    1874,    2009,    2154,    2310,    2477,    2656,    2848,    3054,
    3275,    3511,    3765,    4037,    4329,    4642,    4977,    5337,    5722,    6136,
    6579,    7055,    7565,    8111,    8697,    9326,    10000,    10723,    11498,    12328,
    13219,    14175,    15199,    16298,    17475,    18738,    20092,    21544,    23101,    24771,
    26561,    28480,    30539,    32745,    35112,    37649,    40370,    43288,    46416,    49770,
    53367,    57224,    61359,    65793,    70548,    75646,    81113,    86975,    93260,    100000,};

// 动态重新配置ADC采样时间
void reconfigure_adc_sample_time(uint32_t adc_sample_time)
{
    // 停止定时器触发
    TIM_Cmd(TIM3, DISABLE);
    
    // 停止DMA
    DMA_Cmd(DMA2_Stream0, DISABLE);
    
    // 停止ADC转换
    ADC_Cmd(ADC1, DISABLE);
    ADC_Cmd(ADC2, DISABLE);
    
    // 等待ADC完全停止
    delay_ms(5);
    
    // 重新配置采样时间
    ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, adc_sample_time);
    ADC_RegularChannelConfig(ADC2, ADC_Channel_8, 1, adc_sample_time);
    
    // 清除ADC和DMA状态
    ADC1->SR = 0;
    ADC2->SR = 0;
    DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0|DMA_IT_DMEIF0|DMA_IT_TEIF0|DMA_IT_HTIF0);
    
    // 重新启动ADC
    ADC_Cmd(ADC1, ENABLE);
    ADC_Cmd(ADC2, ENABLE);
    
    // 重新启动DMA
    DMA_Cmd(DMA2_Stream0, ENABLE);
    
    // 重新启动定时器
    TIM_Cmd(TIM3, ENABLE);
    
    // 等待ADC稳定
    delay_ms(10);
}

// 动态配置ADC采样率和转换速率
void configure_adc_for_frequency(uint32_t test_frequency)
{
    const freq_segment_config_t* config = get_freq_segment_config(test_frequency);

    
    // 重新配置定时器3的采样频率
    TIM3_Config(config->adc_sample_rate);
    
    // 动态重新配置ADC转换时间
    reconfigure_adc_sample_time(config->adc_sample_time);
    
    // 等待配置稳定
    // 等待配置稳定
    delay_ms(50);
}

// 相位突变检测和滤波函数
float32_t filter_phase_jumps(float32_t new_phase, uint32_t current_freq)
{
    // 如果是第一个有效测量，直接返回
    if(g_phase_history_count == 0)
    {
        g_phase_history[g_phase_history_index] = new_phase;
        g_phase_history_index = (g_phase_history_index + 1) % 5;
        g_phase_history_count++;
        return new_phase;
    }
    
    // 计算与上一个测量的相位差
    float32_t phase_diff = new_phase - g_last_valid_phase;
    
    // 相位解缠绕
    while(phase_diff > 180.0f) phase_diff -= 360.0f;
    while(phase_diff < -180.0f) phase_diff += 360.0f;
    
    // 检测突变（相位变化超过30度且频率变化不大）
    uint32_t freq_diff = (current_freq > g_last_valid_freq) ? 
                        (current_freq - g_last_valid_freq) : 
                        (g_last_valid_freq - current_freq);
    
    float32_t freq_ratio = (float32_t)freq_diff / g_last_valid_freq;
    
    // 如果相位跳跃过大且频率变化不大，认为是突变
    if(fabsf(phase_diff) > 30.0f && freq_ratio < 0.1f && g_phase_history_count >= 3)
    {
        // 使用历史数据进行中值滤波
        float32_t history_phases[5];
        uint8_t valid_count = 0;
        
        // 收集历史相位数据
        for(uint8_t i = 0; i < g_phase_history_count && i < 5; i++)
        {
            uint8_t idx = (g_phase_history_index - 1 - i + 5) % 5;
            history_phases[valid_count++] = g_phase_history[idx];
        }
        
        // 简单的中值滤波（排序取中值）
        for(uint8_t i = 0; i < valid_count - 1; i++)
        {
            for(uint8_t j = i + 1; j < valid_count; j++)
            {
                if(history_phases[i] > history_phases[j])
                {
                    float32_t temp = history_phases[i];
                    history_phases[i] = history_phases[j];
                    history_phases[j] = temp;
                }
            }
        }
        
        // 使用中值作为预测值
        float32_t predicted_phase = history_phases[valid_count / 2];
        
        // 比较新测量值与预测值
        float32_t prediction_diff = new_phase - predicted_phase;
        while(prediction_diff > 180.0f) prediction_diff -= 360.0f;
        while(prediction_diff < -180.0f) prediction_diff += 360.0f;
        
        // 如果与预测值差异仍然很大，使用预测值代替
        if(fabsf(prediction_diff) > 20.0f)
        {
            printf("相位突变检测: %.2f -> %.2f @%luHz (已滤波)\r\n", 
                   new_phase, predicted_phase, current_freq);
            new_phase = predicted_phase;
        }
    }
    
    // 更新历史记录
    g_phase_history[g_phase_history_index] = new_phase;
    g_phase_history_index = (g_phase_history_index + 1) % 5;
    if(g_phase_history_count < 5) g_phase_history_count++;
    
    return new_phase;
}

// 高精度分段采样函数
void sample_frequency_point_optimized(uint32_t freq_index)
{
    if(freq_index >= 100) return; // 边界检查
    
    uint32_t test_freq = g_log_freq_points[freq_index];
    const freq_segment_config_t* config = get_freq_segment_config(test_freq);
    
    // 动态配置ADC
    configure_adc_for_frequency(test_freq);
    
    // 设置AD9833输出频率
    AD9833_WaveSeting(test_freq, 0, SIN_WAVE, 0);
    delay_ms(config->settle_time_ms); // 等待信号稳定
    
    // 执行多次采样以提高精度
    uint8_t sample_count = 3; // 每个频率点采样3次
    float32_t magnitude_sum = 0, phase_sum = 0, snr_sum = 0;
    uint8_t valid_samples = 0;
    
    for(uint8_t i = 0; i < sample_count; i++)
    {
        // 启动ADC采集
        ADC_DMA_Trig(FFT_SIZE);
        delay_ms(config->acquisition_time_ms);
        
        // 等待DMA完成
        uint32_t timeout = config->acquisition_time_ms + 100;
        while(!ADC1_DMA_Flag && timeout--) {
            delay_ms(1);
        }
        
        if(ADC1_DMA_Flag) {
            ADC1_DMA_Flag = 0;
            
            float32_t mag_ad9833, phase_ad9833, snr_ad9833;
            float32_t mag_rlc, phase_rlc, snr_rlc;
            
            // 处理AD9833通道
            process_single_channel_fft_optimized(AD9833_Value, config->adc_sample_rate, 
                                                test_freq, WINDOW_HANNING,
                                                &mag_ad9833, &phase_ad9833, &snr_ad9833);
            
            // 处理RLC通道
            process_single_channel_fft_optimized(RLC_Value, config->adc_sample_rate, 
                                                test_freq, WINDOW_HANNING,
                                                &mag_rlc, &phase_rlc, &snr_rlc);
            
            // 检查信噪比质量
            if(snr_ad9833 > config->snr_threshold && snr_rlc > config->snr_threshold)
            {
                // 计算相位差
                float32_t phase_diff = phase_rlc - phase_ad9833;
                
                // 基本相位解缠绕
                while(phase_diff > 180.0f) phase_diff -= 360.0f;
                while(phase_diff < -180.0f) phase_diff += 360.0f;
                
                // 计算增益
                float32_t magnitude_ratio = 1e-9f;
                float32_t gain_db = 0.0f;
                
                if(mag_ad9833 > 1e-6f && mag_rlc > 1e-6f) {
                    magnitude_ratio = mag_rlc / mag_ad9833;
                    gain_db = 20.0f * log10f(magnitude_ratio);
                    
                    // 限制增益范围
                    if(gain_db > 100.0f) gain_db = 100.0f;
                    if(gain_db < -100.0f) gain_db = -100.0f;
                }
                
                magnitude_sum += magnitude_ratio;
                phase_sum += phase_diff;
                snr_sum += (snr_ad9833 + snr_rlc) / 2.0f;
                valid_samples++;
            }
        }
    }
    
    // 输出平均结果
    if(valid_samples > 0)
    {
        float32_t avg_magnitude = magnitude_sum / valid_samples;
        float32_t avg_phase = phase_sum / valid_samples;
        float32_t avg_gain_db = 20.0f * log10f(avg_magnitude);
        
        // 应用相位突变滤波
        avg_phase = filter_phase_jumps(avg_phase, test_freq);
        
        // 应用RLC电路专用的相位连续性校正
        if(g_phase_tracking_enabled && g_last_valid_freq > 0)
        {
            float32_t phase_jump = avg_phase - g_last_valid_phase;
            
            // 对于RLC电路，相位应该连续变化
            // 中等跳跃检测阈值（20度），避免过度校正
            if(fabsf(phase_jump) > 20.0f)
            {
                // 尝试不同的360度偏移校正
                float32_t best_phase = avg_phase;
                float32_t min_jump = fabsf(phase_jump);
                
                // 测试±720度校正范围
                for(int offset = -2; offset <= 2; offset++)
                {
                    if(offset == 0) continue;
                    
                    float32_t test_phase = avg_phase + offset * 360.0f;
                    float32_t test_jump = fabsf(test_phase - g_last_valid_phase);
                    
                    if(test_jump < min_jump)
                    {
                        min_jump = test_jump;
                        best_phase = test_phase;
                    }
                }
                
                // 只有在显著改善时才应用校正
                if(min_jump < fabsf(phase_jump) * 0.7f)
                {
                    avg_phase = best_phase;
                }
            }
        }
        
        // 规范化相位到[-180, 180]
        while(avg_phase > 180.0f) avg_phase -= 360.0f;
        while(avg_phase < -180.0f) avg_phase += 360.0f;
        
        // 更新全局相位跟踪
        g_last_valid_phase = avg_phase;
        g_last_valid_freq = test_freq;
        g_phase_tracking_enabled = 1;
        
        // 存储结果到数组
        g_magnitude_array[freq_index] = avg_magnitude;
        g_gain_db_array[freq_index] = avg_gain_db;
        g_phase_diff_array[freq_index] = avg_phase;
    }
    else
    {
        // 无效测量时设置默认值
        g_magnitude_array[freq_index] = 0.0f;
        g_gain_db_array[freq_index] = -100.0f;  // 极低增益表示无效
        g_phase_diff_array[freq_index] = 0.0f;
    }
}



// 完整的100点频率扫描函数
void scan_all_frequency_points(void)
{
    for(uint16_t i = 0; i < 100; i++)
    {
        sample_frequency_point_optimized(i);
    }
}

// 修改Model_5函数使用新的扫描系统
void Model_5(void)
{
    // 重置相位跟踪
    g_last_valid_phase = 0.0f;
    g_last_valid_freq = 0;
    g_phase_tracking_enabled = 0;
    g_phase_history_index = 0;
    g_phase_history_count = 0;
    
    // 初始化FFT实例
    init_fft_instance();
    
    // 初始化AD9833
    AD9833_WaveSeting(100000, 0, SIN_WAVE, 0);
    AD9833_AmpSet(115); 
    
    // 使用基础ADC初始化
    ADC_FFT_Init();
    
    delay_ms(1000); // 等待系统稳定
    
    uint32_t scan_count = 0;
    
    while(1)
    {
        scan_count++;
        
        // 每次扫描前重置相位跟踪
        g_last_valid_phase = 0.0f;
        g_last_valid_freq = 0;
        g_phase_tracking_enabled = 0;
        g_phase_history_index = 0;
        g_phase_history_count = 0;
        
        scan_all_frequency_points();
        
        delay_ms(3000);
    }
}