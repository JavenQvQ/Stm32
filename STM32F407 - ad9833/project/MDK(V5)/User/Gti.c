#ifndef PI
#define PI 3.14159265358979323846f
#endif
#include "board.h"
#include "ADC.h"
#include "AD9833.h"
#include "OLED.h"
#include "stm32f4xx.h"
#include "AD9220_DCMI.h"
#include "KEY.h"
#include "arm_math.h" // 引入CMSIS DSP库
#include "RLC.h"     // 引入RLC分析库

// 在文件顶部添加滤波器类型字符串数组
const char* filter_type_strings[] = {
    "Low-Pass",   // 0
    "High-Pass",  // 1
    "Band-Pass",  // 2
    "Band-Stop"   // 3
};


#include <stdio.h>
extern uint8_t KEY0_interrupt_flag ;
extern uint8_t KEY1_interrupt_flag ;
extern uint8_t KEY2_interrupt_flag ;
extern uint8_t Key_WakeUp_interrupt_flag ;
extern uint16_t AD9833_Value[ADC1_DMA_Size];
extern uint16_t RLC_Value[ADC1_DMA_Size];
uint16_t model=0; // 模型选择变量	
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

// 优化的频率分段表 - 降低采样率，提高稳定性
const freq_segment_config_t g_freq_segments[] = {
    // 超低频段: 100Hz - 500Hz - 降低采样率，提高信号质量
    {100,    500,    32768,   ADC_SampleTime_144Cycles, 50, 150, 200, 2.0f},
    
    // 低频段: 500Hz - 1kHz - 适中配置
    {500,    1000,   65536,   ADC_SampleTime_84Cycles,  40, 120, 180, 3.0f},
    
    // 中低频段1: 1kHz - 3kHz - 平衡配置
    {1000,   3000,   65536,   ADC_SampleTime_84Cycles,  30, 100, 150, 4.0f},
    
    // 中低频段2: 3kHz - 10kHz - 保守配置
    {3000,   10000,  131072,  ADC_SampleTime_56Cycles,  20, 80,  120, 3.0f},
    
    // 中频段: 10kHz - 30kHz - 平衡配置
    {10000,  30000,  131072,  ADC_SampleTime_56Cycles,  15, 60,  100, 2.0f},
    
    // 中高频段: 30kHz - 100kHz - 适中采样率
    {30000,  100000, 262144,  ADC_SampleTime_28Cycles,  12, 50,  80,  1.5f},

    // 高频段: 100kHz - 200kHz - 保守配置
    {100000, 200000, 262144,  ADC_SampleTime_28Cycles,  10, 40,  60,  1.0f},
    
    // 超高频段: 200kHz - 250kHz - 最保守配置
    {200000, 250000, 262144,  ADC_SampleTime_15Cycles,  8,  30,  50,  0.8f},
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

// 幅度连续性跟踪变量
static float32_t g_last_valid_magnitude = 0.0f;
static float32_t g_magnitude_history[5] = {0}; // 幅度历史记录
static uint8_t g_magnitude_history_index = 0;
static uint8_t g_magnitude_history_count = 0;

// 测量结果存储数组 - 供RLC分析使用 (180点)
float32_t g_magnitude_array[180];    // 幅值数组
float32_t g_gain_db_array[180];      // 增益数组(dB) - 导出给RLC.c使用
float32_t g_phase_diff_array[180];   // 相位差数组(度) - 导出给RLC.c使用

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
    
    // 数据预处理：直接使用ADC原始值并去除直流分量
    // 对于频率响应分析，只需要相对比较，无需转换为实际电压
    
    float32_t dc_offset = 0.0f;
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        // 直接使用ADC原始值
        fft_input_buffer[i] = (float32_t)adc_data[i];
        dc_offset += fft_input_buffer[i];
    }
    dc_offset /= FFT_SIZE;
    
    // 去除直流分量
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        fft_input_buffer[i] -= dc_offset;
    }
    
    // 数据有效性检查 - 优化版本，提前退出节省计算
    float32_t max_val = -4096.0f, min_val = 4096.0f;
    float32_t rms_val = 0.0f;
    
    // 边计算边检查，提前发现无效数据
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        float32_t val = fft_input_buffer[i];
        if(val > max_val) max_val = val;
        if(val < min_val) min_val = val;
        rms_val += val * val;
        
        // 提前检查：如果前1/4数据就显示信号太弱，直接退出
        if(i == FFT_SIZE/4) {
            float32_t partial_rms = sqrtf(rms_val / (FFT_SIZE/4));
            float32_t partial_pp = max_val - min_val;
            
            if(target_freq > 100000) {
                if(partial_pp < 5.0f || partial_rms < 2.0f) return; // 高频段更严格
            } else {
                if(partial_pp < 25.0f || partial_rms < 10.0f) return; // 低频段检查
            }
        }
    }
    rms_val = sqrtf(rms_val / FFT_SIZE);
    
    // 最终有效性检查 - 动态阈值
    float32_t min_pp_adc = (target_freq > 100000) ? 10.0f : 50.0f;
    float32_t min_rms_adc = (target_freq > 100000) ? 5.0f : 20.0f;
    
    // 超高频段进一步放宽
    if(target_freq > 10000) {
        min_pp_adc *= 0.5f;
        min_rms_adc *= 0.5f;
    }
    
    if((max_val - min_val) < min_pp_adc || rms_val < min_rms_adc) {
        return; // 数据无效或信号太弱
    }
        
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
        
        // 计算幅度 - 基于ADC原始值的相对幅度
        float32_t raw_magnitude = sqrtf(real * real + imag * imag);
        float32_t window_gain = get_window_coherent_gain(window_type);
        
        // FFT幅度校正：
        // 1. 除以FFT_SIZE/2 进行FFT标准化
        // 2. 除以窗函数增益进行窗函数补偿
        // 3. 结果为相对幅度值（ADC码值单位）
        *magnitude = (raw_magnitude * 2.0f) / (window_gain * FFT_SIZE);  // 单边谱幅度，ADC码值单位
        
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
            case ADC_SampleTime_15Cycles:  actual_adc_cycles = 15;  break;
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

// 用于嵌入式C代码的100个对数扫频点数组
const uint32_t g_log_freq_points[180] = {
    100,    104,    109,    114,    119,    124,    130,    136,    142,    148,
    155,    162,    169,    177,    184,    193,    201,    210,    220,    229,
    240,    250,    262,    273,    285,    298,    312,    325,    340,    355,
    371,    388,    405,    423,    442,    462,    482,    504,    526,    550,
    575,    600,    627,    655,    684,    715,    747,    780,    815,    851,
    889,    929,    971,    1014,    1059,    1107,    1156,    1208,    1262,    1318,
    1377,    1439,    1503,    1570,    1640,    1714,    1790,    1870,    1954,    2041,
    2132,    2227,    2327,    2431,    2539,    2653,    2771,    2895,    3025,    3160,
    3301,    3448,    3602,    3763,    3932,    4107,    4291,    4482,    4683,    4892,
    5110,    5339,    5577,    5827,    6087,    6359,    6643,    6940,    7250,    7574,
    7912,    8266,    8635,    9021,    9424,    9845,    10285,    10744,    11224,    11726,
    12250,    12797,    13369,    13966,    14590,    15242,    15923,    16634,    17377,    18154,
    18965,    19812,    20697,    21622,    22588,    23597,    24652,    25753,    26904,    28106,
    29361,    30673,    32044,    33475,    34971,    36534,    38166,    39871,    41652,    43513,
    45458,    47489,    49610,    51827,    54143,    56562,    59089,    61729,    64487,    67368,
    70378,    73522,    76807,    80239,    83824,    87569,    91481,    95569,    99839,    104299,
    108959,    113828,    118913,    124226,    129777,    135575,    141632,    147960,    154571,    161477,
    168692,    176229,    184102,    192328,    200921,    209898,    219276,    229073,    239308,    250000,};

// 动态重新配置ADC采样时间 - 优化版本，减少系统重启
void reconfigure_adc_sample_time(uint32_t adc_sample_time)
{
    // 静态变量记录当前配置，避免重复配置
    static uint32_t current_sample_time = 0xFFFFFFFF;
    if(current_sample_time == adc_sample_time) {
        return; // 配置未变化，跳过重配置
    }
    
    // 快速重配置：只停止转换，不完全重启系统
    ADC_Cmd(ADC1, DISABLE);
    ADC_Cmd(ADC2, DISABLE);
    
    // 等待ADC停止（减少等待时间）
    delay_ms(2);
    
    // 重新配置采样时间
    ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, adc_sample_time);
    ADC_RegularChannelConfig(ADC2, ADC_Channel_8, 1, adc_sample_time);
    
    // 清除ADC状态
    ADC1->SR = 0;
    ADC2->SR = 0;
    
    // 重新启动ADC
    ADC_Cmd(ADC1, ENABLE);
    ADC_Cmd(ADC2, ENABLE);
    
    // 减少稳定等待时间
    delay_ms(5);
    
    // 更新当前配置记录
    current_sample_time = adc_sample_time;
}

// 动态配置ADC采样率和转换速率 - 优化版本，减少重复配置
void configure_adc_for_frequency(uint32_t test_frequency)
{
    const freq_segment_config_t* config = get_freq_segment_config(test_frequency);
    
    // 静态变量缓存当前配置，避免重复设置
    static uint32_t last_sample_rate = 0;
    static uint32_t last_sample_time = 0xFFFFFFFF;
    
    // 计算实际ADC转换时间以确保转换率匹配
    uint16_t actual_cycles = 56; // 默认值
    switch(config->adc_sample_time)
    {
        case ADC_SampleTime_15Cycles:  actual_cycles = 15;  break;
        case ADC_SampleTime_28Cycles:  actual_cycles = 28;  break;
        case ADC_SampleTime_56Cycles:  actual_cycles = 56;  break;
        case ADC_SampleTime_84Cycles:  actual_cycles = 84;  break;
        case ADC_SampleTime_144Cycles: actual_cycles = 144; break;
        default: actual_cycles = 56; break;
    }
    
    // ADC转换总时间 = 采样时间 + 12个转换周期（STM32F407规格）
    uint16_t total_conversion_cycles = actual_cycles + 12;
    
    // ADC时钟频率：84MHz（双ADC模式下，通过APB2_CLK/2获得）
    float32_t adc_clock_freq = 42000000.0f; // 42MHz
    
    // 计算单次转换时间（秒）
    float32_t conversion_time_s = (float32_t)total_conversion_cycles / adc_clock_freq;
    
    // 计算最大可能的采样率（考虑双通道）
    float32_t max_sampling_rate = 1.0f / (conversion_time_s * 2.0f); // 双通道需要乘2
    
    // 确保配置的采样率不超过ADC转换能力
    uint32_t safe_sample_rate = config->adc_sample_rate;
    if(safe_sample_rate > (uint32_t)(max_sampling_rate * 0.8f)) // 留20%余量
    {
        // 降级到安全采样率
        safe_sample_rate = (uint32_t)(max_sampling_rate * 0.8f);
        
        // 四舍五入到最近的2的幂次
        uint32_t power_of_2 = 1;
        while(power_of_2 < safe_sample_rate) power_of_2 <<= 1;
        if((safe_sample_rate - (power_of_2 >> 1)) < (power_of_2 - safe_sample_rate))
            safe_sample_rate = power_of_2 >> 1;
        else
            safe_sample_rate = power_of_2;
    }
    
    // 只在采样率真正改变时重新配置定时器
    if(safe_sample_rate != last_sample_rate) {
        TIM3_Config(safe_sample_rate);
        last_sample_rate = safe_sample_rate;
    }
    
    // 只在采样时间真正改变时重新配置ADC
    if(config->adc_sample_time != last_sample_time) {
        reconfigure_adc_sample_time(config->adc_sample_time);
        last_sample_time = config->adc_sample_time;
    }
    
    // 计算稳定等待时间 - 优化等待策略
    uint16_t stability_delay = config->settle_time_ms;
    
    // 只有在配置发生变化时才需要完整的稳定时间
    if(safe_sample_rate != last_sample_rate || config->adc_sample_time != last_sample_time) {
        if(test_frequency > 100000) // 高频段需要更多时间稳定
        {
            stability_delay += 10; // 减少额外延迟
        }
        delay_ms(stability_delay);
    } else {
        // 配置未变化时只需要短暂延迟
        delay_ms(stability_delay / 3);
    }
}


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
            new_phase = predicted_phase;
        }
    }
    
    // 更新历史记录
    g_phase_history[g_phase_history_index] = new_phase;
    g_phase_history_index = (g_phase_history_index + 1) % 5;
    if(g_phase_history_count < 5) g_phase_history_count++;
    
    return new_phase;
}

// 幅度突变滤波函数 - 基于理想RLC电路幅频特性的连续性
float32_t filter_magnitude_jumps(float32_t new_magnitude, uint32_t current_freq)
{
    // 如果是第一个有效测量，直接返回
    if(g_magnitude_history_count == 0)
    {
        g_magnitude_history[g_magnitude_history_index] = new_magnitude;
        g_magnitude_history_index = (g_magnitude_history_index + 1) % 5;
        g_magnitude_history_count++;
        g_last_valid_magnitude = new_magnitude;
        return new_magnitude;
    }
    
    // 计算幅度变化率（对数域更适合频率响应分析）
    float32_t magnitude_ratio = new_magnitude / g_last_valid_magnitude;
    float32_t magnitude_change_db = 20.0f * log10f(magnitude_ratio);
    
    // 理想RLC电路的幅度变化应该是渐进的，不会有剧烈跳跃
    // 基于频率变化计算预期的最大幅度变化
    float32_t freq_ratio = (float32_t)current_freq / g_last_valid_freq;
    float32_t expected_max_change_db = 6.0f * log10f(freq_ratio); // 基于经验的合理范围
    
    // 检测突变（幅度变化超过预期且频率变化不大）
    if(fabsf(magnitude_change_db) > fabsf(expected_max_change_db) + 10.0f && 
       fabsf(freq_ratio - 1.0f) < 0.2f && g_magnitude_history_count >= 3)
    {
        // 使用历史数据进行中值滤波
        float32_t history_magnitudes[5];
        uint8_t valid_count = 0;
        
        // 收集历史幅度数据
        for(uint8_t i = 0; i < g_magnitude_history_count && i < 5; i++)
        {
            uint8_t idx = (g_magnitude_history_index - 1 - i + 5) % 5;
            history_magnitudes[valid_count++] = g_magnitude_history[idx];
        }
        
        // 简单的中值滤波（排序取中值）
        for(uint8_t i = 0; i < valid_count - 1; i++)
        {
            for(uint8_t j = i + 1; j < valid_count; j++)
            {
                if(history_magnitudes[i] > history_magnitudes[j])
                {
                    float32_t temp = history_magnitudes[i];
                    history_magnitudes[i] = history_magnitudes[j];
                    history_magnitudes[j] = temp;
                }
            }
        }
        
        // 使用中值作为预测值
        float32_t predicted_magnitude = history_magnitudes[valid_count / 2];
        
        // 基于频率变化进行线性插值校正
        if(valid_count >= 2)
        {
            float32_t trend = (history_magnitudes[valid_count-1] - history_magnitudes[0]) / (valid_count - 1);
            predicted_magnitude = g_last_valid_magnitude + trend;
        }
        
        // 比较新测量值与预测值
        float32_t prediction_ratio = new_magnitude / predicted_magnitude;
        float32_t prediction_diff_db = 20.0f * log10f(prediction_ratio);
        
        // 如果与预测值差异仍然很大，使用加权平均
        if(fabsf(prediction_diff_db) > 8.0f)
        {
            // 加权平均：75%预测值 + 25%测量值
            new_magnitude = predicted_magnitude * 0.75f + new_magnitude * 0.25f;
        }
    }
    
    // 更新历史记录
    g_magnitude_history[g_magnitude_history_index] = new_magnitude;
    g_magnitude_history_index = (g_magnitude_history_index + 1) % 5;
    if(g_magnitude_history_count < 5) g_magnitude_history_count++;
    
    g_last_valid_magnitude = new_magnitude;
    return new_magnitude;
}

// 全局幅度平滑处理函数 - 基于理想RLC电路幅频特性的连续性
void smooth_magnitude_response(void)
{
    // 对整个增益数组进行平滑处理，确保理想RLC电路的连续性
    float32_t temp_array[180];
    
    // 复制原始数据
    for(uint16_t i = 0; i < 180; i++) {
        temp_array[i] = g_gain_db_array[i];
    }
    
    // 应用移动平均滤波
    for(uint16_t i = 1; i < 179; i++)
    {
        // 跳过无效数据点
        if(temp_array[i] <= -90.0f) continue;
        
        float32_t sum = 0.0f;
        uint8_t count = 0;
        
        // 收集窗口内的有效数据
        for(int8_t j = -1; j <= 1; j++)
        {
            uint16_t idx = i + j;
            if(idx < 180 && temp_array[idx] > -90.0f)
            {
                sum += temp_array[idx];
                count++;
            }
        }
        
        // 如果有足够的有效邻点，应用平滑
        if(count >= 2)
        {
            float32_t smoothed_value = sum / count;
            
            // 检查是否存在明显的突变
            float32_t deviation = fabsf(temp_array[i] - smoothed_value);
            
            // 如果偏差过大（超过6dB），使用加权平均
            if(deviation > 6.0f)
            {
                g_gain_db_array[i] = smoothed_value * 0.7f + temp_array[i] * 0.3f;
            }
            else if(deviation > 3.0f)
            {
                // 轻微平滑
                g_gain_db_array[i] = smoothed_value * 0.3f + temp_array[i] * 0.7f;
            }
        }
    }
    
    // 第二轮：检查和修复剩余的尖峰
    for(uint16_t i = 1; i < 179; i++)
    {
        if(g_gain_db_array[i] <= -90.0f) continue;
        
        float32_t prev_val = (i > 0 && g_gain_db_array[i-1] > -90.0f) ? g_gain_db_array[i-1] : g_gain_db_array[i];
        float32_t next_val = (i < 179 && g_gain_db_array[i+1] > -90.0f) ? g_gain_db_array[i+1] : g_gain_db_array[i];
        
        // 检查是否为孤立的尖峰
        float32_t expected_val = (prev_val + next_val) / 2.0f;
        float32_t spike_deviation = fabsf(g_gain_db_array[i] - expected_val);
        
        // 如果是明显的尖峰（偏离邻点平均值超过8dB），进行修正
        if(spike_deviation > 8.0f)
        {
            g_gain_db_array[i] = expected_val;
        }
    }
}

// 高精度分段采样函数 - 优化版本
void sample_frequency_point_optimized(uint32_t freq_index)
{
    if(freq_index >= 180) return; // 边界检查 - 更新为180点
    
    uint32_t test_freq = g_log_freq_points[freq_index];
    const freq_segment_config_t* config = get_freq_segment_config(test_freq);
    
    // 静态变量缓存上次设置的频率，避免重复配置AD9833
    static uint32_t last_ad9833_freq = 0;
    
    // 动态配置ADC
    configure_adc_for_frequency(test_freq);
    
    // 只在频率真正改变时设置AD9833
    if(test_freq != last_ad9833_freq) {
        AD9833_WaveSeting(test_freq, 0, SIN_WAVE, 0);
        last_ad9833_freq = test_freq;
        delay_ms(config->settle_time_ms); // 等待信号稳定
    } else {
        // 频率未变化，只需要短暂稳定时间
        delay_ms(config->settle_time_ms / 2);
    }
    
    // 对每个频率点都执行两次FFT采样以减小误差
    uint8_t sample_count = 2;  // 固定为2次采样
    float32_t magnitude_sum = 0, phase_sum = 0, snr_sum = 0;
    uint8_t valid_samples = 0;
    
    for(uint8_t i = 0; i < sample_count; i++)
    {
        // 启动ADC采集
        ADC_DMA_Trig(FFT_SIZE);
        
        // 动态等待时间 - 基于采样率调整
        uint16_t wait_time = config->acquisition_time_ms;
        if(config->adc_sample_rate < 100000) wait_time += 20; // 低采样率需要更多时间
        delay_ms(wait_time);
        
        // 等待DMA完成 - 优化超时处理
        uint32_t timeout = wait_time + 50;
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
            
            // 进一步降低SNR门限，优先获得有效数据
            float32_t snr_threshold_adj = config->snr_threshold * 0.3f; // 降低70%
            
            // 频率段特殊优化
            if(test_freq > 200000) snr_threshold_adj *= 0.5f; // 超高频段再降低50%
            else if(test_freq > 100000) snr_threshold_adj *= 0.6f; // 高频段降低40%
            else if(test_freq > 30000) snr_threshold_adj *= 0.7f; // 中高频段降低30%
            
            // 设置最低门限
            if(snr_threshold_adj < 0.5f) snr_threshold_adj = 0.5f;
            
            if(snr_ad9833 > snr_threshold_adj && snr_rlc > snr_threshold_adj)
            {
                // 计算相位差
                float32_t phase_diff = phase_rlc - phase_ad9833;
                
                // 基本相位解缠绕
                while(phase_diff > 180.0f) phase_diff -= 360.0f;
                while(phase_diff < -180.0f) phase_diff += 360.0f;
                
                // 计算增益
                float32_t magnitude_ratio = 1e-9f;
                
                if(mag_ad9833 > 1e-9f && mag_rlc > 1e-9f) { // 进一步放宽阈值
                    magnitude_ratio = mag_rlc / mag_ad9833;
                }
                
                magnitude_sum += magnitude_ratio;
                phase_sum += phase_diff;
                snr_sum += (snr_ad9833 + snr_rlc) / 2.0f;
                valid_samples++;
            }
            // 备用方案：进一步放宽条件
            else if(mag_ad9833 > 1e-9f && mag_rlc > 1e-9f)
            {
                float32_t phase_diff = phase_rlc - phase_ad9833;
                while(phase_diff > 180.0f) phase_diff -= 360.0f;
                while(phase_diff < -180.0f) phase_diff += 360.0f;
                
                float32_t magnitude_ratio = mag_rlc / mag_ad9833;
                
                magnitude_sum += magnitude_ratio;
                phase_sum += phase_diff;
                snr_sum += (snr_ad9833 + snr_rlc) / 2.0f;
                valid_samples++;
            }
        }
        
        // 两次采样之间的间隔 - 确保信号稳定
        if(i == 0) {
            delay_ms(15); // 第一次和第二次采样之间的间隔
        }
    }
    
    // 输出平均结果
    if(valid_samples > 0)
    {
        float32_t avg_magnitude = magnitude_sum / valid_samples;
        float32_t avg_phase = phase_sum / valid_samples;
        
        // 应用幅度突变滤波 - 确保RLC电路幅频特性的连续性
        avg_magnitude = filter_magnitude_jumps(avg_magnitude, test_freq);
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




// 完整的180点频率扫描函数 - 高效版本，带进度显示
void scan_all_frequency_points(void)
{   
     rlc_analysis_result_t analysis_result;
    for(uint16_t i = 0; i < 180; i++)
    {
        sample_frequency_point_optimized(i);
        
        // 每10个点更新一次进度显示，减少对计算速度的影响
        if(i % 10 == 0 || i == 179)
        {
            uint8_t progress = (uint8_t)((i + 1) * 100 / 180);
            
            // 仅更新进度行，不清屏
            OLED_ShowString(0, 16, "Progress: ", 16, 1);
            OLED_ShowNum(80, 16, progress, 3, 16, 1);
            OLED_ShowString(104, 16, "%", 16, 1);
            
            // 显示当前频率
            if(g_log_freq_points[i] >= 1000) {
                OLED_ShowNum(0, 32, g_log_freq_points[i] / 1000, 3, 16, 1);
                OLED_ShowString(48, 32, "kHz", 16, 1);
            } else {
                OLED_ShowNum(0, 32, g_log_freq_points[i], 3, 16, 1);
                OLED_ShowString(48, 32, "Hz ", 16, 1);
            }
            OLED_Refresh();
        }
        printf("%d,%.2f,%.2f\r\n", 
                   i + 1, g_gain_db_array[i], g_phase_diff_array[i]);
    // 对整个幅度响应进行理想RLC电路的连续性优化
    smooth_magnitude_response();
    // 声明分析结果结构体
    // 在OLED上显示结果
    }
    smooth_magnitude_response();
    analyze_rlc_filter(&analysis_result);
    print_detailed_analysis_report(&analysis_result);
    OLED_ShowString(0, 32, (uint8_t*)filter_type_strings[analysis_result.type], 16, 1);
    OLED_Refresh();


}




// 修改Model_4函数使用新的扫描系统
void Model_4(void)
{
    // 重置相位跟踪
    g_last_valid_phase = 0.0f;
    g_last_valid_freq = 0;
    g_phase_tracking_enabled = 0;
    g_phase_history_index = 0;
    g_phase_history_count = 0;
    
    // 重置幅度跟踪
    g_last_valid_magnitude = 0.0f;
    g_magnitude_history_index = 0;
    g_magnitude_history_count = 0;
    for(uint8_t i = 0; i < 5; i++) {
        g_magnitude_history[i] = 0.0f;
        g_phase_history[i] = 0.0f;
    }
    
    OLED_Clear();
    OLED_ShowString(0, 0, "Model 5: RLC Scan",16, 1);
    OLED_Refresh();
    
    // 初始化FFT实例
    init_fft_instance();

    
    AD9833_AmpSet(115); 
    
    // 初始化AD9833
    AD9833_WaveSeting(100000, 0, SIN_WAVE, 0);
    
    // 使用基础ADC初始化
    ADC_FFT_Init();
    
    delay_ms(10); // 等待系统稳定
    
    while(1)
    {   
        // 显示扫描开始信息
        OLED_Clear();
        OLED_ShowString(0, 0, "Model 5: RLC Scan",16, 1);
        OLED_Refresh();
        
        // 执行纯计算扫描，不包含任何OLED显示操作
        scan_all_frequency_points();
        
        // 扫描完成后显示结果并等待用户操作
        while(1)
        {
            delay_ms(100);
            if(Key_WakeUp_interrupt_flag)
            {
                Key_WakeUp_interrupt_flag = 0;  // 清除标志
                return;  // 退出扫描
            }
            // 其他按键可以添加切换显示等功能
        }
    }
}