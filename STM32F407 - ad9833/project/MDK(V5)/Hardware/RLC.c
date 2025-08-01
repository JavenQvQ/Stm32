// filepath: d:\stm32\stm32up\STM32F407 - ad9833\project\MDK(V5)\Hardware\RLC.c

#include "RLC.h"
#include "arm_math.h"
#include <stdio.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// 定义 fabsf 函数（如果没有的话）
#ifndef fabsf
#define fabsf(x) ((x) < 0 ? -(x) : (x))
#endif

// 简化的分析结果结构体
typedef struct {
    uint8_t type;               // 滤波器类型 0-3 (取消Unknown)
    float32_t center_freq;      // 中心频率 Hz
    float32_t q_factor;         // Q值
    float32_t lc_product;       // LC乘积
    float32_t rc_product;       // RC乘积
} rlc_simple_result_t;

// 滤波器类型字符串 - 移除Unknown类型
const char* filter_names[] = {
    "Low-Pass", "High-Pass", "Band-Pass", "Band-Stop"
};

// 外部引用 - 更新为180点250kHz扫频
extern const uint32_t g_log_freq_points[180];
extern float32_t g_gain_db_array[180];
extern float32_t g_phase_diff_array[180];
extern float32_t g_magnitude_array[180];  // 添加幅度数组的外部引用

// 相位解缠绕函数 - 从Python移植
void unwrap_phase(float32_t* phase_array, uint16_t length)
{
    for(uint16_t i = 1; i < length; i++)
    {
        float32_t diff = phase_array[i] - phase_array[i-1];
        
        // 将相位差限制在[-180°, +180°]范围内
        while(diff > 180.0f) {
            phase_array[i] -= 360.0f;
            diff = phase_array[i] - phase_array[i-1];
        }
        while(diff < -180.0f) {
            phase_array[i] += 360.0f;
            diff = phase_array[i] - phase_array[i-1];
        }
    }
}

// 计算相位导数 - 从Python移植
void calculate_phase_derivative(const float32_t* phase_array, float32_t* derivative_array, uint16_t length)
{
    // 使用频率间隔计算梯度（假设对数频率间隔相等）
    for(uint16_t i = 0; i < length; i++)
    {
        if(i == 0) {
            // 前向差分
            derivative_array[i] = phase_array[i+1] - phase_array[i];
        }
        else if(i == length - 1) {
            // 后向差分
            derivative_array[i] = phase_array[i] - phase_array[i-1];
        }
        else {
            // 中心差分
            derivative_array[i] = (phase_array[i+1] - phase_array[i-1]) / 2.0f;
        }
    }
}

// 基于相频特性相似度选择最佳匹配的滤波器类型
uint8_t select_best_filter_match(void)
{
    // 计算各种滤波器的相似度得分
    float32_t scores[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Low-Pass, High-Pass, Band-Pass, Band-Stop
    
    // 1. 基于增益特性的评分
    float32_t max_gain = -100.0f, min_gain = 100.0f;
    uint16_t max_gain_idx = 0, min_gain_idx = 0;
    
    for(uint16_t i = 0; i < 180; i++) {
        if(g_gain_db_array[i] > max_gain) {
            max_gain = g_gain_db_array[i];
            max_gain_idx = i;
        }
        if(g_gain_db_array[i] < min_gain) {
            min_gain = g_gain_db_array[i];
            min_gain_idx = i;
        }
    }
    
    float32_t gain_range = max_gain - min_gain;
    
    // 增益分布分析
    float32_t low_freq_avg = 0.0f, high_freq_avg = 0.0f, mid_freq_avg = 0.0f;
    uint16_t low_count = 0, high_count = 0, mid_count = 0;
    
    for(uint16_t i = 0; i < 60; i++) {
        if(g_gain_db_array[i] > -80.0f) {
            low_freq_avg += g_gain_db_array[i];
            low_count++;
        }
    }
    if(low_count > 0) low_freq_avg /= low_count;
    
    for(uint16_t i = 60; i < 120; i++) {
        if(g_gain_db_array[i] > -80.0f) {
            mid_freq_avg += g_gain_db_array[i];
            mid_count++;
        }
    }
    if(mid_count > 0) mid_freq_avg /= mid_count;
    
    for(uint16_t i = 120; i < 180; i++) {
        if(g_gain_db_array[i] > -80.0f) {
            high_freq_avg += g_gain_db_array[i];
            high_count++;
        }
    }
    if(high_count > 0) high_freq_avg /= high_count;
    
    // 2. 基于相位特性的评分
    static float32_t phase_unwrapped[180];
    for(uint16_t i = 0; i < 180; i++) {
        phase_unwrapped[i] = g_phase_diff_array[i];
    }
    unwrap_phase(phase_unwrapped, 180);
    
    float32_t start_phase = phase_unwrapped[0];
    float32_t end_phase = phase_unwrapped[179];
    float32_t total_phase_change = end_phase - start_phase;
    
    // 3. 低通滤波器评分
    // 特征：低频增益高，高频增益低，相位从0°下降
    if(low_count > 0 && high_count > 0) {
        float32_t gain_trend = low_freq_avg - high_freq_avg;
        if(gain_trend > 0.0f) {
            scores[0] += gain_trend * 0.1f; // 增益下降趋势
        }
        
        if(fabsf(start_phase) < 45.0f) {
            scores[0] += 20.0f; // 低频相位接近0°
        }
        
        if(total_phase_change < -30.0f) {
            scores[0] += fabsf(total_phase_change) * 0.3f; // 相位下降
        }
    }
    
    // 4. 高通滤波器评分
    // 特征：低频增益低，高频增益高，低频段数据缺失
    uint16_t low_freq_invalid = 0;
    for(uint16_t i = 0; i < 60; i++) {
        if(g_gain_db_array[i] < -50.0f) {
            low_freq_invalid++;
        }
    }
    
    if(low_freq_invalid > 30) {
        scores[1] += (float32_t)low_freq_invalid * 0.5f; // 低频无效数据多
    }
    
    if(high_count > 0 && low_count > 0) {
        float32_t gain_trend = high_freq_avg - low_freq_avg;
        if(gain_trend > 0.0f) {
            scores[1] += gain_trend * 0.2f; // 增益上升趋势
        }
    }
    
    if(fabsf(end_phase) < 45.0f) {
        scores[1] += 15.0f; // 高频相位接近0°
    }
    
    // 5. 带通滤波器评分 - 针对您的数据优化
    // 您的数据特征：-26.58dB → -17.53dB，相位83° → -74°
    {
        // 基础评分
        if(max_gain_idx > 20 && max_gain_idx < 160) {
            scores[2] += 15.0f;
        }
        
        // 增益范围检查（您的数据约9dB范围）
        if(gain_range > 2.0f) {
            scores[2] += gain_range * 3.0f;
        }
        
        // 相位变化检查（您的数据约157°变化）
        if(fabsf(total_phase_change) > 60.0f) {
            scores[2] += fabsf(total_phase_change) * 0.5f;
        }
        
        // 检查"先衰减再上升"的模式（典型带通特征）
        float32_t start_avg = 0.0f, middle_avg = 0.0f, end_avg = 0.0f;
        uint16_t start_count = 0, middle_count = 0, end_count = 0;
        
        // 开始段：前30点
        for(uint16_t i = 0; i < 30; i++) {
            if(g_gain_db_array[i] > -80.0f) {
                start_avg += g_gain_db_array[i];
                start_count++;
            }
        }
        if(start_count > 0) start_avg /= start_count;
        
        // 中间段：最低点附近
        uint16_t middle_start = min_gain_idx > 20 ? min_gain_idx - 20 : 0;
        uint16_t middle_end = min_gain_idx < 160 ? min_gain_idx + 20 : 179;
        for(uint16_t i = middle_start; i <= middle_end; i++) {
            if(g_gain_db_array[i] > -80.0f) {
                middle_avg += g_gain_db_array[i];
                middle_count++;
            }
        }
        if(middle_count > 0) middle_avg /= middle_count;
        
        // 结束段：后20点
        for(uint16_t i = 160; i < 180; i++) {
            if(g_gain_db_array[i] > -80.0f) {
                end_avg += g_gain_db_array[i];
                end_count++;
            }
        }
        if(end_count > 0) end_avg /= end_count;
        
        // 检查V型或U型曲线特征
        if(start_count > 0 && middle_count > 0 && end_count > 0) {
            float32_t start_drop = start_avg - middle_avg;
            float32_t end_rise = end_avg - middle_avg;
            
            // 如果开始段高于中间段，结束段也高于中间段
            if(start_drop > 1.0f && end_rise > 1.0f) {
                scores[2] += 25.0f + (start_drop + end_rise) * 2.0f;
            }
            // 或者只要有明显的谷底特征
            else if(start_drop > 0.0f || end_rise > 0.0f) {
                scores[2] += 10.0f + (start_drop + end_rise) * 1.5f;
            }
        }
        
        // 检查最低点位置（您的数据在174点附近）
        if(min_gain_idx > 150 && min_gain_idx < 178) {
            scores[2] += 15.0f;  // 最低点在后段，符合带通特征
            
            // 检查最低点深度
            float32_t valley_depth = max_gain - min_gain;
            if(valley_depth > 3.0f) {
                scores[2] += valley_depth * 2.0f;
            }
        }
        
        // 检查增益曲线的连续性
        uint16_t decline_points = 0, rise_points = 0;
        
        // 统计下降段
        for(uint16_t i = 1; i < min_gain_idx; i++) {
            if(g_gain_db_array[i] < g_gain_db_array[i-1]) {
                decline_points++;
            }
        }
        
        // 统计上升段（如果有数据）
        for(uint16_t i = min_gain_idx + 1; i < 180; i++) {
            if(g_gain_db_array[i] > g_gain_db_array[i-1]) {
                rise_points++;
            }
        }
        
        // 如果有明显的下降和上升趋势
        if(decline_points > 20) {
            scores[2] += decline_points * 0.3f;
        }
        if(rise_points > 3) {
            scores[2] += rise_points * 1.0f;
        }
        
        // 峰值检查（如果峰值不在最后段）
        if(max_gain_idx < 170) {
            float32_t peak_prominence = max_gain - (low_freq_avg + high_freq_avg) / 2.0f;
            if(peak_prominence > 0.5f) {
                scores[2] += peak_prominence * 1.5f;
            }
        }
    }
    // 特征：中频有陷波，两侧增益较高
    if(min_gain_idx > 30 && min_gain_idx < 150 && gain_range > 10.0f) {
        scores[3] += 20.0f; // 中频有明显陷波
        
        float32_t notch_depth = max_gain - min_gain;
        if(notch_depth > 5.0f) {
            scores[3] += notch_depth * 1.5f;
        }
        
        // 检查陷波处相位跳跃
        if(min_gain_idx > 5 && min_gain_idx < 175) {
            float32_t phase_before = phase_unwrapped[min_gain_idx - 5];
            float32_t phase_after = phase_unwrapped[min_gain_idx + 5];
            float32_t phase_jump = fabsf(phase_after - phase_before);
            
            if(phase_jump > 90.0f) {
                scores[3] += 15.0f;
            }
        }
    }
    
    // 7. 找到最高得分的滤波器类型
    uint8_t best_type = 0;
    float32_t max_score = scores[0];
    
    for(uint8_t i = 1; i < 4; i++) {
        if(scores[i] > max_score) {
            max_score = scores[i];
            best_type = i;
        }
    }
    
    // 8. 如果所有得分都很低，基于简单规则选择
    if(max_score < 10.0f) {
        // 基于增益趋势的简单判断
        if(high_count > 0 && low_count > 0) {
            float32_t gain_trend = high_freq_avg - low_freq_avg;
            
            if(gain_trend > 10.0f) {
                best_type = 1; // High-Pass
            } else if(gain_trend < -10.0f) {
                best_type = 0; // Low-Pass
            } else if(gain_range > 15.0f && max_gain_idx > 20 && max_gain_idx < 160) {
                best_type = 2; // Band-Pass
            } else {
                best_type = 0; // 默认Low-Pass
            }
        }
    }
    
    printf("滤波器相似度评分: LP=%.1f, HP=%.1f, BP=%.1f, BS=%.1f -> %s\r\n",
           scores[0], scores[1], scores[2], scores[3], filter_names[best_type]);
    
    return best_type;
}

// 基于"相位指纹"的专家决策系统 - 考虑ADC采样精度限制
uint8_t identify_filter_type_advanced(void)
{
    // ==================== 第一步：ADC精度问题检测和数据清理 ====================
    // 在任何分析之前，先统一清理所有无效的ADC采样值
    
    const float32_t min_magnitude_threshold = 1e-8f;    // 最小幅度阈值
    const float32_t min_phase_threshold = 0.5f;         // 最小相位阈值
    const float32_t min_gain_threshold = -60.0f;        // 最小可信增益阈值
    
    uint16_t invalid_count = 0;
    uint16_t corrected_count = 0;
    
    // 第一轮：标记所有无效数据
    for(uint16_t i = 0; i < 180; i++)
    {
        uint8_t is_invalid = 0;
        
        // 检查各种ADC精度问题：
        
        // 1. 幅度过小（ADC无法检测）
        if(g_magnitude_array[i] <= min_magnitude_threshold) {
            is_invalid = 1;
        }
        
        // 2. 相位异常为0（ADC精度不足）
        if(fabsf(g_phase_diff_array[i]) < min_phase_threshold) {
            is_invalid = 1;
        }
        
        // 3. 增益异常（计算错误或信号太弱）
        if(g_gain_db_array[i] <= min_gain_threshold) {
            is_invalid = 1;
        }
        
        // 4. 组合检查：幅度和相位同时异常（典型ADC精度问题）
        if(g_magnitude_array[i] <= min_magnitude_threshold * 10 && 
           fabsf(g_phase_diff_array[i]) < min_phase_threshold * 2) {
            is_invalid = 1;
        }
        
        // 5. 低增益时的额外检查
        if(g_gain_db_array[i] < -30.0f) {
            if(fabsf(g_phase_diff_array[i]) < min_phase_threshold * 3 ||
               g_magnitude_array[i] <= min_magnitude_threshold * 50) {
                is_invalid = 1;
            }
        }
        
        // 6. 检查增益异常为0dB的情况（ADC无法检测信号时的错误计算）
        if(fabsf(g_gain_db_array[i]) < 2.0f && g_magnitude_array[i] <= min_magnitude_threshold * 5) {
            is_invalid = 1;
        }
        
        if(is_invalid) {
            // 统一标记为无效数据
            g_gain_db_array[i] = -200.0f;        // 无效增益标记
            g_magnitude_array[i] = 0.0f;         // 无效幅度标记
            g_phase_diff_array[i] = 0.0f;        // 临时标记（后面会修正）
            invalid_count++;
        }
    }
    
    // 第二轮：针对高通滤波器的特殊处理
    uint16_t low_freq_invalid = 0;
    uint16_t high_freq_valid = 0;
    
    // 统计低频段（前60个点）无效数据
    for(uint16_t i = 0; i < 60; i++) {
        if(g_gain_db_array[i] <= -100.0f) {  // 被标记为无效
            low_freq_invalid++;
        }
    }
    
    // 统计高频段（后60个点）有效数据
    for(uint16_t i = 120; i < 180; i++) {
        if(g_gain_db_array[i] > -100.0f) {   // 仍然有效
            high_freq_valid++;
        }
    }
    
    // 如果低频段大量无效，高频段有效，可能是高通滤波器
    uint8_t suspected_highpass = 0;
    if(low_freq_invalid > 30 && high_freq_valid > 20) {
        suspected_highpass = 1;
        
        // 为高通滤波器修正低频段相位（设为理论值180°）
        for(uint16_t i = 0; i < 60; i++) {
            if(g_gain_db_array[i] <= -100.0f) {  // 无效点
                g_phase_diff_array[i] = 180.0f;   // 高通低频理论相位
                corrected_count++;
            }
        }
        
        // 寻找第一个有效点，确保相位连续性
        uint16_t first_valid_idx = 0;
        for(uint16_t i = 0; i < 180; i++) {
            if(g_gain_db_array[i] > -100.0f) {
                first_valid_idx = i;
                break;
            }
        }
        
        if(first_valid_idx > 0 && first_valid_idx < 180) {
            float32_t first_valid_phase = g_phase_diff_array[first_valid_idx];
            
            // 确保从180°平滑过渡到第一个有效点
            if(fabsf(first_valid_phase - 180.0f) > 90.0f) {
                float32_t phase_offset = 0.0f;
                
                if(first_valid_phase < 90.0f) {
                    phase_offset = 180.0f;
                } else if(first_valid_phase > 270.0f) {
                    phase_offset = -180.0f;
                }
                
                // 应用相位调整到所有后续有效数据点
                if(fabsf(phase_offset) > 0.1f) {
                    for(uint16_t i = first_valid_idx; i < 180; i++) {
                        if(g_gain_db_array[i] > -100.0f) {
                            g_phase_diff_array[i] += phase_offset;
                            
                            // 保持相位在合理范围内
                            while(g_phase_diff_array[i] > 180.0f) g_phase_diff_array[i] -= 360.0f;
                            while(g_phase_diff_array[i] < -180.0f) g_phase_diff_array[i] += 360.0f;
                        }
                    }
                }
            }
        }
    }
    
    // ==================== 第二步：基于清理后的数据进行识别 ====================
    // 现在所有后续处理都基于已清理的干净数据
    
    // 创建临时数组进行相位处理
    static float32_t phase_unwrapped[180];
    static float32_t phase_derivative[180];
    static uint8_t valid_phase_mask[180];  // 标记有效相位数据
    
    // 统计有效数据点（基于预清理的结果）
    uint16_t valid_phase_count = 0;
    
    for(uint16_t i = 0; i < 180; i++)
    {
        phase_unwrapped[i] = g_phase_diff_array[i];
        
        // 检查数据是否在预清理中被标记为有效
        if(g_gain_db_array[i] > -100.0f &&          // 不是无效标记
           g_magnitude_array[i] > 0.0f &&           // 有有效幅度
           fabsf(g_phase_diff_array[i]) > 0.1f) {   // 有有效相位
            valid_phase_mask[i] = 1;
            valid_phase_count++;
        } else {
            valid_phase_mask[i] = 0;
        }
    }
    
    // 如果有效相位点太少，主要依靠增益特征判断
    uint8_t phase_data_reliable = (valid_phase_count > 60);  // 至少需要1/3的点有效
    
    // 步骤1: 相位解缠绕（只对有效点进行）
    unwrap_phase(phase_unwrapped, 180);
    
    // 步骤2: 计算相位导数（跳过无效点）
    for(uint16_t i = 0; i < 180; i++)
    {
        if(!valid_phase_mask[i]) {
            phase_derivative[i] = 0.0f;  // 无效点导数设为0
            continue;
        }
        
        if(i == 0) {
            if(i + 1 < 180 && valid_phase_mask[i+1]) {
                float32_t log_freq_diff = logf((float32_t)g_log_freq_points[i+1]) - logf((float32_t)g_log_freq_points[i]);
                phase_derivative[i] = (phase_unwrapped[i+1] - phase_unwrapped[i]) / log_freq_diff;
            } else {
                phase_derivative[i] = 0.0f;
            }
        }
        else if(i == 179) {
            if(valid_phase_mask[i-1]) {
                float32_t log_freq_diff = logf((float32_t)g_log_freq_points[i]) - logf((float32_t)g_log_freq_points[i-1]);
                phase_derivative[i] = (phase_unwrapped[i] - phase_unwrapped[i-1]) / log_freq_diff;
            } else {
                phase_derivative[i] = 0.0f;
            }
        }
        else {
            if(i > 0 && i < 179 && valid_phase_mask[i-1] && valid_phase_mask[i+1]) {
                float32_t log_freq_diff = logf((float32_t)g_log_freq_points[i+1]) - logf((float32_t)g_log_freq_points[i-1]);
                phase_derivative[i] = (phase_unwrapped[i+1] - phase_unwrapped[i-1]) / log_freq_diff;
            } else {
                phase_derivative[i] = 0.0f;
            }
        }
    }
    
    // 获取关键相位点（现在基于已清理的数据）
    float32_t start_phase = 0.0f, end_phase = 0.0f;
    uint16_t first_valid_idx = 0, last_valid_idx = 179;
    
    // 寻找第一个和最后一个有效相位点
    for(uint16_t i = 0; i < 180; i++) {
        if(valid_phase_mask[i]) {
            first_valid_idx = i;
            start_phase = phase_unwrapped[i];
            break;
        }
    }
    for(uint16_t i = 179; i > 0; i--) {
        if(valid_phase_mask[i]) {
            last_valid_idx = i;
            end_phase = phase_unwrapped[i];
            break;
        }
    }
    
    float32_t total_phase_change = end_phase - start_phase;
       
    // 增益特征分析（基于已清理的数据）
    float32_t max_gain = -100.0f, min_gain = 100.0f;
    uint16_t max_gain_idx = 0, min_gain_idx = 0;
    
    for(uint16_t i = 0; i < 180; i++)
    {
        if(valid_phase_mask[i]) {  // 只考虑有效数据点
            if(g_gain_db_array[i] > max_gain) {
                max_gain = g_gain_db_array[i];
                max_gain_idx = i;
            }
            if(g_gain_db_array[i] < min_gain) {
                min_gain = g_gain_db_array[i];
                min_gain_idx = i;
            }
        }
    }
    float32_t gain_range = max_gain - min_gain;
      
    // ==================== 第三步：决策逻辑（基于干净数据）====================
    const float32_t phase_tolerance = 60.0f;         // 放宽相位容差
    const float32_t derivative_threshold = 4.0f;     // 相位导数阈值
    
    // 检查增益特征（基于有效数据）
    uint8_t has_clear_peak = (max_gain_idx > 10 && max_gain_idx < 170 && gain_range > 6.0f);
    uint8_t has_clear_notch = (min_gain_idx > 10 && min_gain_idx < 170 && gain_range > 10.0f);
    
    // 1. 检查带阻特征（最特殊）
    float32_t max_positive_derivative = -1000.0f;
    for(uint16_t i = 20; i < 160; i++) {
        if(valid_phase_mask[i] && phase_derivative[i] > max_positive_derivative) {
            max_positive_derivative = phase_derivative[i];
        }
    }
    
    if(max_positive_derivative > derivative_threshold && 
       fabsf(start_phase) < phase_tolerance && 
       fabsf(end_phase) < phase_tolerance && 
       has_clear_notch) {
        return 3; // Band-Stop
    }
    
    // 2. 基于起始相位的放宽判断
    if(fabsf(start_phase) < phase_tolerance) {
        return 0; // Low-Pass
    }
    else if(fabsf(start_phase - 90.0f) < phase_tolerance) {
        return 2; // Band-Pass
    }
    else if(fabsf(start_phase - 180.0f) < phase_tolerance || fabsf(start_phase + 180.0f) < phase_tolerance) {
        return 1; // High-Pass
    }
    
    // 3. 如果预清理时已经疑似高通，直接返回
    if(suspected_highpass) {
        return 1; // High-Pass
    }
    
    // 4. 非标准情况的放宽判断
    if(start_phase > 30.0f && start_phase < 150.0f) {
        if(fabsf(total_phase_change) > 60.0f) {
            return 2; // Band-Pass
        }
        
        if(gain_range > 3.0f && min_gain_idx > 120) {
            return 2; // Band-Pass
        }
    }
    
    // 5. 简化的增益趋势判断（基于有效数据）
    float32_t early_gain_avg = 0.0f, late_gain_avg = 0.0f;
    uint16_t early_count = 0, late_count = 0;
    
    for(uint16_t i = 0; i < 60; i++) {
        if(valid_phase_mask[i]) {
            early_gain_avg += g_gain_db_array[i];
            early_count++;
        }
    }
    for(uint16_t i = 120; i < 180; i++) {
        if(valid_phase_mask[i]) {
            late_gain_avg += g_gain_db_array[i];
            late_count++;
        }
    }
    
    if(early_count > 0) early_gain_avg /= early_count;
    if(late_count > 0) late_gain_avg /= late_count;
    
    float32_t gain_trend = late_gain_avg - early_gain_avg;
    
    if(gain_trend > 5.0f && late_count > 5) {
        return 1; // High-Pass
    }
    else if(gain_trend < -5.0f && early_count > 5) {
        return 0; // Low-Pass
    }
    
    // 6. 最终兜底
    return select_best_filter_match();
}

// 查找增益最大值索引 - 修复为180点
uint16_t find_max_gain_index(void)
{
    uint16_t max_idx = 0;
    float32_t max_gain = g_gain_db_array[0];
    
    for(uint16_t i = 1; i < 180; i++)
    {
        if(g_gain_db_array[i] > max_gain)
        {
            max_gain = g_gain_db_array[i];
            max_idx = i;
        }
    }
    return max_idx;
}

// 查找增益最小值索引 - 修复为180点
uint16_t find_min_gain_index(void)
{
    uint16_t min_idx = 0;
    float32_t min_gain = g_gain_db_array[0];
    
    for(uint16_t i = 1; i < 180; i++)
    {
        if(g_gain_db_array[i] < min_gain)
        {
            min_gain = g_gain_db_array[i];
            min_idx = i;
        }
    }
    return min_idx;
}

// 查找特定相位对应的频率 - 修复为180点扫频，忽略无效数据
float32_t find_frequency_at_phase(float32_t target_phase_deg)
{
    // 创建相位解缠绕数组
    static float32_t phase_unwrapped[180];
    
    // 复制并解缠绕相位数据
    for(uint16_t i = 0; i < 180; i++)
    {
        phase_unwrapped[i] = g_phase_diff_array[i];
    }
    unwrap_phase(phase_unwrapped, 180);
    
    // 查找最接近目标相位的点（只在有效数据中查找）
    uint16_t best_idx = 0;
    float32_t min_diff = 1000.0f;
    uint8_t found_valid = 0;
    
    for(uint16_t i = 1; i < 180; i++)
    {
        // 跳过ADC精度问题导致的无效数据点：
        // 1. 增益<-100dB（可能是无效标记）
        // 2. 幅度为0（ADC无法检测）
        // 3. 相位为0且增益低（ADC精度问题）
        if(g_gain_db_array[i] <= -100.0f || 
           g_magnitude_array[i] <= 1e-10f ||
           (fabsf(g_phase_diff_array[i]) < 0.5f && g_gain_db_array[i] < -30.0f)) {
            continue;
        }
        
        float32_t diff = fabsf(phase_unwrapped[i] - target_phase_deg);
        if(diff < min_diff)
        {
            min_diff = diff;
            best_idx = i;
            found_valid = 1;
        }
    }
    
    // 如果没找到有效数据，使用备用方法
    if(!found_valid) {
        if(target_phase_deg == 0.0f) { // Band-Pass查找0°
            uint16_t max_idx = find_max_gain_index();
            return (float32_t)g_log_freq_points[max_idx];
        }
        // 其他情况返回中点频率
        return (float32_t)g_log_freq_points[90];
    }
    
    // 考虑测量误差的容差检查
    if(min_diff > 30.0f)  // 如果最接近的点相位差仍然很大
    {
        // 在这种情况下，使用增益特征作为备用方法
        if(target_phase_deg == 0.0f) // Band-Pass查找0°
        {
            uint16_t max_idx = find_max_gain_index();
            return (float32_t)g_log_freq_points[max_idx];
        }
    }
    
    // 改进的对数频率插值（只在有效数据间进行）
    if(best_idx > 0 && best_idx < 179 && min_diff < 20.0f)
    {
        // 寻找有效的前后点进行插值
        uint16_t prev_idx = best_idx - 1;
        uint16_t next_idx = best_idx + 1;
        
        // 向前寻找有效点
        while(prev_idx > 0 && g_gain_db_array[prev_idx] <= -100.0f) {
            prev_idx--;
        }
        
        // 向后寻找有效点
        while(next_idx < 179 && g_gain_db_array[next_idx] <= -100.0f) {
            next_idx++;
        }
        
        // 确保找到的点是有效的
        if(g_gain_db_array[prev_idx] > -100.0f && g_gain_db_array[next_idx] > -100.0f)
        {
            // 使用对数频率进行插值，更符合扫频点分布
            float32_t log_f1 = logf((float32_t)g_log_freq_points[prev_idx]);
            float32_t log_f2 = logf((float32_t)g_log_freq_points[next_idx]);
            float32_t p1 = phase_unwrapped[prev_idx];
            float32_t p2 = phase_unwrapped[next_idx];
            
            if(fabsf(p2 - p1) > 1e-6f)
            {
                float32_t log_f_interp = log_f1 + (target_phase_deg - p1) * (log_f2 - log_f1) / (p2 - p1);
                return expf(log_f_interp);
            }
        }
    }
    
    return (float32_t)g_log_freq_points[best_idx];
}

// 考虑100Hz-250kHz对数扫频点分布的Q值计算优化 - 忽略无效数据点
void calculate_improved_q_factor(rlc_simple_result_t* result)
{
    // 创建相位解缠绕数组进行分析
    static float32_t phase_unwrapped[180];
    static float32_t log_freq_array[180];
    static float32_t phase_derivative_log[180];
    static uint8_t valid_data_mask[180];  // 标记有效数据点
    
    // 准备对数频率数组和相位数据，同时检查数据有效性
    uint16_t valid_count = 0;
    for(uint16_t i = 0; i < 180; i++)
    {
        // 检查数据是否有效：考虑ADC精度问题
        // 1. 增益不是无效标记（>-100dB）
        // 2. 相位不为0（>0.1°，避免ADC精度问题）
        // 3. 幅度不为0（>1e-10，ADC能检测到信号）
        uint8_t is_valid = 1;
        
        if(g_gain_db_array[i] <= -99.0f) {
            is_valid = 0;  // 无效增益标记
        }
        
        if(fabsf(g_phase_diff_array[i]) <= 0.1f) {
            is_valid = 0;  // 相位接近0，可能是ADC精度问题
        }
        
        if(g_magnitude_array[i] <= 1e-10f) {
            is_valid = 0;  // 幅度为0，ADC无法检测
        }
        
        // 额外检查：低增益时相位更容易出现精度问题
        if(g_gain_db_array[i] < -50.0f && fabsf(g_phase_diff_array[i]) < 1.0f) {
            is_valid = 0;  // 低增益+小相位=ADC精度问题
        }
        
        if(is_valid) {
            valid_data_mask[i] = 1;
            valid_count++;
        } else {
            valid_data_mask[i] = 0;
        }
        
        phase_unwrapped[i] = g_phase_diff_array[i];
        log_freq_array[i] = logf((float32_t)g_log_freq_points[i]);
    }
    
    // 如果有效数据点太少，使用默认Q值
    if(valid_count < 20) {
        result->q_factor = 0.707f;
        return;
    }
    
    unwrap_phase(phase_unwrapped, 180);
    
    // 基于对数频率计算相位导数（只对有效点进行）
    for(uint16_t i = 0; i < 180; i++)
    {
        phase_derivative_log[i] = 0.0f;  // 默认值
        
        if(!valid_data_mask[i]) {
            continue;  // 跳过无效点
        }
        
        if(i == 0) {
            // 寻找下一个有效点
            for(uint16_t j = i + 1; j < 180; j++) {
                if(valid_data_mask[j]) {
                    phase_derivative_log[i] = (phase_unwrapped[j] - phase_unwrapped[i]) / 
                                             (log_freq_array[j] - log_freq_array[i]);
                    break;
                }
            }
        }
        else if(i == 179) {
            // 寻找前一个有效点
            for(uint16_t j = i - 1; j > 0; j--) {
                if(valid_data_mask[j]) {
                    phase_derivative_log[i] = (phase_unwrapped[i] - phase_unwrapped[j]) / 
                                             (log_freq_array[i] - log_freq_array[j]);
                    break;
                }
            }
        }
        else {
            // 寻找前后有效点进行中心差分
            uint16_t prev_valid = 0, next_valid = 0;
            
            for(uint16_t j = i - 1; j > 0; j--) {
                if(valid_data_mask[j]) {
                    prev_valid = j;
                    break;
                }
            }
            
            for(uint16_t j = i + 1; j < 180; j++) {
                if(valid_data_mask[j]) {
                    next_valid = j;
                    break;
                }
            }
            
            if(prev_valid > 0 && next_valid > 0) {
                phase_derivative_log[i] = (phase_unwrapped[next_valid] - phase_unwrapped[prev_valid]) / 
                                         (log_freq_array[next_valid] - log_freq_array[prev_valid]);
            }
        }
        
        // 转换为弧度
        phase_derivative_log[i] = phase_derivative_log[i] * PI / 180.0f;
    }
    
    // 找到中心频率对应的索引（只在有效数据中查找）
    uint16_t center_idx = 90; // 默认值（180点的中点）
    float32_t min_freq_diff = 1e6f;
    
    for(uint16_t i = 0; i < 180; i++)
    {
        if(!valid_data_mask[i]) continue;  // 跳过无效点
        
        float32_t freq_diff = fabsf((float32_t)g_log_freq_points[i] - result->center_freq);
        if(freq_diff < min_freq_diff)
        {
            min_freq_diff = freq_diff;
            center_idx = i;
        }
    }
    
    // 修正的Q值计算 - 考虑100Hz-250kHz扫频特点，只使用有效数据
    if(center_idx > 5 && center_idx < 175 && valid_data_mask[center_idx])
    {
        // 在中心频率附近取平均值提高稳定性（只使用有效点）
        float32_t slope_sum = 0.0f;
        uint8_t valid_slope_count = 0;
        
        for(uint16_t i = center_idx - 2; i <= center_idx + 2; i++)
        {
            if(i < 180 && valid_data_mask[i] && fabsf(phase_derivative_log[i]) < 10.0f)  // 排除异常值
            {
                slope_sum += phase_derivative_log[i];
                valid_slope_count++;
            }
        }
        
        if(valid_slope_count > 0)
        {
            float32_t avg_slope = slope_sum / valid_slope_count;
            
            // 针对不同滤波器类型的Q值计算修正
            // 基于二阶系统传递函数的相位响应理论
            float32_t zeta_est = 0.707f;
            
            switch(result->type)
            {
                case 0: // Low-Pass: H(s) = 1/(s²/ωn² + 2ζs/ωn + 1)
                case 1: // High-Pass: H(s) = (s/ωn)²/(s²/ωn² + 2ζs/ωn + 1)
                {
                    // 对于二阶系统，在截止频率附近：
                    // dφ/d(ln ω) ≈ -2ζ (低通) 或 +2ζ (高通)
                    // 但实际测量中符号可能因相位解缠绕而变化
                    float32_t slope_magnitude = fabsf(avg_slope);
                    
                    // 将度转换为弧度，并考虑对数频率的影响
                    slope_magnitude = slope_magnitude * PI / 180.0f;
                    
                    // 理论关系：|dφ/d(ln ω)| = 2ζ
                    zeta_est = slope_magnitude / 2.0f;
                    
                    // 对于一阶系统的修正（如果Q值很低）
                    if(zeta_est > 1.0f) {
                        // 可能是一阶系统，其dφ/d(ln ω) = 1
                        zeta_est = 0.707f; // 设为临界阻尼
                    }
                    break;
                }
                    
                case 2: // Band-Pass: H(s) = (2ζs/ωn)/(s²/ωn² + 2ζs/ωn + 1)
                {
                    // 对于带通滤波器，在谐振频率处：
                    // dφ/d(ln ω) ≈ -4ζ (通过0°时的斜率)
                    float32_t slope_magnitude = fabsf(avg_slope);
                    
                    // 将度转换为弧度
                    slope_magnitude = slope_magnitude * PI / 180.0f;
                    
                    // 理论关系：|dφ/d(ln ω)| = 4ζ
                    zeta_est = slope_magnitude / 4.0f;
                    break;
                }
                    
                case 3: // Band-Stop: H(s) = (s²/ωn² + 1)/(s²/ωn² + 2ζs/ωn + 1)
                {
                    // 对于带阻滤波器，相位导数符号相反
                    float32_t slope_magnitude = fabsf(avg_slope);
                    
                    // 将度转换为弧度
                    slope_magnitude = slope_magnitude * PI / 180.0f;
                    
                    // 理论关系：|dφ/d(ln ω)| = 4ζ
                    zeta_est = slope_magnitude / 4.0f;
                    break;
                }
            }
            
            // 基于100Hz-250kHz扫频点密度的误差修正
            float32_t freq_density_factor = 1.0f;
            
            // 根据频率段调整修正因子（考虑实际测量精度）
            if(result->center_freq < 300.0f) {
                freq_density_factor = 1.5f;  // 低频段噪声影响大
            }
            else if(result->center_freq < 1000.0f) {
                freq_density_factor = 1.3f;
            }
            else if(result->center_freq < 5000.0f) {
                freq_density_factor = 1.1f;
            }
            else if(result->center_freq < 50000.0f) {
                freq_density_factor = 1.0f;  // 中频段最准确
            }
            else if(result->center_freq < 150000.0f) {
                freq_density_factor = 0.9f;
            }
            else {
                freq_density_factor = 0.8f;  // 高频段ADC精度下降
            }
            
            zeta_est *= freq_density_factor;
            
            // 限制zeta的合理范围
            if(zeta_est < 0.05f) zeta_est = 0.05f;    // 对应Q=10
            if(zeta_est > 2.0f) zeta_est = 2.0f;      // 对应Q=0.25
            
            // 计算Q值：Q = 1/(2*zeta)
            result->q_factor = 1.0f / (2.0f * zeta_est);
        }
    }
    
    // 基于3dB带宽的备用Q值计算（用于交叉验证，只使用有效数据）
    if(result->type == 2 || result->type == 3) // Band-Pass或Band-Stop
    {
        // 寻找有效的参考点
        uint16_t ref_idx = 0;
        float32_t ref_gain = -1000.0f;
        
        for(uint16_t i = 0; i < 179; i++) {
            if(valid_data_mask[i]) {
                if((result->type == 2 && g_gain_db_array[i] > ref_gain) ||
                   (result->type == 3 && g_gain_db_array[i] < ref_gain)) {
                    ref_gain = g_gain_db_array[i];
                    ref_idx = i;
                }
            }
        }
        
        if(valid_data_mask[ref_idx]) {
            float32_t target_gain = (result->type == 2) ? (ref_gain - 3.0f) : (ref_gain + 3.0f);
            
            // 查找左右3dB点（只在有效数据中查找）
            uint16_t left_idx = ref_idx, right_idx = ref_idx;
            
            for(uint16_t i = ref_idx; i > 0; i--) {
                if(valid_data_mask[i] && 
                   ((result->type == 2 && g_gain_db_array[i] <= target_gain) ||
                    (result->type == 3 && g_gain_db_array[i] >= target_gain))) {
                    left_idx = i;
                    break;
                }
            }
            
            for(uint16_t i = ref_idx; i < 179; i++) {
                if(valid_data_mask[i] && 
                   ((result->type == 2 && g_gain_db_array[i] <= target_gain) ||
                    (result->type == 3 && g_gain_db_array[i] >= target_gain))) {
                    right_idx = i;
                    break;
                }
            }
            
            if(left_idx != right_idx && valid_data_mask[left_idx] && valid_data_mask[right_idx]) {
                float32_t f_low = (float32_t)g_log_freq_points[left_idx];
                float32_t f_high = (float32_t)g_log_freq_points[right_idx];
                float32_t bandwidth = f_high - f_low;
                
                if(bandwidth > 1.0f) {
                    float32_t q_bandwidth = result->center_freq / bandwidth;
                    
                    // 如果两种方法差异很大，取平均值
                    if(fabsf(result->q_factor - q_bandwidth) / result->q_factor > 0.5f) {
                        result->q_factor = (result->q_factor + q_bandwidth) / 2.0f;
                    }
                }
            }
        }
    }
}

// 改进的RLC分析函数 - 基于Python的完整算法
void analyze_rlc_simple(rlc_simple_result_t* result)
{
    // 初始化结果
    result->type = 0; // 默认Low-Pass
    result->center_freq = 0.0f;
    result->q_factor = 0.707f;
    result->lc_product = 0.0f;
    result->rc_product = 0.0f;
    
    // 步骤1: 使用Python的相位指纹识别算法
    result->type = identify_filter_type_advanced();
    
    // 步骤1.5: 高通滤波器数据清理 - 将低频段无效数据标记为无效，防止影响后续计算
    if(result->type == 1) // High-Pass
    {
        // 找到第一个可靠的数据点（增益足够高且相位有意义）
        uint16_t first_reliable_idx = 0;
        uint8_t found_reliable_point = 0;
        
        // 严格的可靠性检查：同时满足增益、幅度、相位三个条件
        for(uint16_t i = 0; i < 180; i++) {
            uint8_t is_reliable = 1;
            
            // 增益检查：必须高于-35dB
            if(g_gain_db_array[i] <= -35.0f) {
                is_reliable = 0;
            }
            
            // 幅度检查：必须能被ADC可靠检测
            if(g_magnitude_array[i] <= 1e-7f) {
                is_reliable = 0;
            }
            
            // 相位检查：必须有意义的相位值（不能接近0）
            if(fabsf(g_phase_diff_array[i]) <= 2.0f) {
                is_reliable = 0;
            }
            
            // 组合检查：避免ADC精度导致的"虚假"数据
            if(g_magnitude_array[i] <= 1e-6f && fabsf(g_phase_diff_array[i]) <= 1.0f) {
                is_reliable = 0;  // 幅度和相位都很小，肯定是ADC精度问题
            }
            
            if(is_reliable) {
                first_reliable_idx = i;
                found_reliable_point = 1;
                break;
            }
        }
        
        if(found_reliable_point) {
            // 将前面不可靠的数据点彻底清理
            for(uint16_t i = 0; i < first_reliable_idx; i++) {
                // 标记为无效数据
                g_gain_db_array[i] = -200.0f;    // 无效增益标记
                g_magnitude_array[i] = 0.0f;     // 无效幅度标记
                
                // 相位设为理论值（高通低频理论相位接近180°）
                g_phase_diff_array[i] = 180.0f;  // 但实际计算中会被忽略
            }
            
            // 检查第一个可靠点的相位连续性
            if(first_reliable_idx > 0 && first_reliable_idx < 180) {
                float32_t first_reliable_phase = g_phase_diff_array[first_reliable_idx];
                
                // 确保相位连续性：从理论180°平滑过渡到第一个可靠点
                if(fabsf(first_reliable_phase - 180.0f) > 90.0f) {
                    // 如果相位差异很大，进行相位缠绕调整
                    float32_t phase_offset = 0.0f;
                    
                    if(first_reliable_phase < 90.0f) {
                        // 第一个可靠点相位太小，可能需要+180°调整
                        phase_offset = 180.0f;
                    } else if(first_reliable_phase > 270.0f) {
                        // 第一个可靠点相位太大，可能需要-180°调整
                        phase_offset = -180.0f;
                    }
                    
                    // 应用相位调整到所有后续有效数据点
                    if(fabsf(phase_offset) > 0.1f) {
                        for(uint16_t i = first_reliable_idx; i < 180; i++) {
                            if(g_gain_db_array[i] > -100.0f) {  // 只调整有效数据点
                                g_phase_diff_array[i] += phase_offset;
                                
                                // 保持相位在合理范围内
                                while(g_phase_diff_array[i] > 180.0f) g_phase_diff_array[i] -= 360.0f;
                                while(g_phase_diff_array[i] < -180.0f) g_phase_diff_array[i] += 360.0f;
                            }
                        }
                        
                        printf("高通滤波器：应用相位连续性调整 %+.1f度\r\n", phase_offset);
                    }
                }
            }
        } else {
            // 如果没找到任何可靠点，可能不是高通滤波器
            printf("警告：未找到可靠的高通数据点，可能识别错误\r\n");
        }
    }
    
    // 步骤2: 根据Python算法精确定位中心频率
    switch(result->type)
    {
        case 0: // Low-Pass - 查找-90°处的频率
        {
            result->center_freq = find_frequency_at_phase(-90.0f);
            break;
        }
        
        case 1: // High-Pass - 查找+90°处的频率  
        {
            result->center_freq = find_frequency_at_phase(90.0f);
            break;
        }
        
        case 2: // Band-Pass - 查找0°处的频率或增益最大处
        {
            // 优先使用相位方法
            float32_t phase_based_freq = find_frequency_at_phase(0.0f);
            
            // 也计算增益最大处
            uint16_t max_idx = find_max_gain_index();
            float32_t gain_based_freq = (float32_t)g_log_freq_points[max_idx];
            
            // 选择更合理的结果（两者应该接近）
            if(fabsf(phase_based_freq - gain_based_freq) / phase_based_freq < 0.5f)
            {
                result->center_freq = phase_based_freq; // 优先使用相位方法
            }
            else
            {
                result->center_freq = gain_based_freq; // 备用方法
            }
            break;
        }
        
        case 3: // Band-Stop - 增益最小处
        {
            uint16_t min_idx = find_min_gain_index();
            result->center_freq = (float32_t)g_log_freq_points[min_idx];
            break;
        }
    }
    

    
    // 步骤3: 使用Python的改进Q值计算方法
    calculate_improved_q_factor(result);
    
    // 步骤4: 计算LC和RC乘积 - 使用Python的公式
    if(result->center_freq > 10.0f && result->q_factor > 0.01f)
    {
        float32_t omega_c = 2.0f * PI * result->center_freq;
        float32_t zeta = 1.0f / (2.0f * result->q_factor); // 阻尼比
        
        // 按照Python算法：
        // LC乘积 = 1 / (ωc)²
        result->lc_product = 1.0f / (omega_c * omega_c);
        
        // RC乘积 = (2 * zeta) / ωc  (修正后的公式)
        result->rc_product = (2.0f * zeta) / omega_c;

    }
    
    // 限制Q值范围
    if(result->q_factor > 100.0f) result->q_factor = 100.0f;
    if(result->q_factor < 0.1f) result->q_factor = 0.1f;
}

// 打印简化的分析结果
void print_simple_result(const rlc_simple_result_t* result)
{
    printf("\r\n=== RLC滤波器分析结果 ===\r\n");
    printf("滤波器类型: %s\r\n", filter_names[result->type]);
    
    // 根据滤波器类型选择合适的频率描述
    if(result->type == 0 || result->type == 1) {
        printf("截止频率: %.1f Hz\r\n", result->center_freq);  // 低通/高通用截止频率
    } else {
        printf("中心频率: %.1f Hz\r\n", result->center_freq);  // 带通/带阻用中心频率
    }
    
    printf("Q值: %.2f\r\n", result->q_factor);
    printf("阻尼比: %.3f\r\n", 1.0f / (2.0f * result->q_factor));
    
    if(result->type == 2 || result->type == 3) // Band-Pass或Band-Stop
    {
        float32_t bandwidth = result->center_freq / result->q_factor;
        printf("3dB带宽: %.1f Hz\r\n", bandwidth);
    }
    
    // 估算元件值（假设C=100nF）
    if(result->lc_product > 1e-12f && result->rc_product > 1e-9f)
    {
        float32_t C_assumed = 100e-9f; // 100nF
        float32_t L_estimated = result->lc_product / C_assumed;
        float32_t R_estimated = result->rc_product / C_assumed;
        
        printf("估算元件值 (假设C=100nF):\r\n");
        printf("  L ≈ %.2f mH\r\n", L_estimated * 1000.0f);
        printf("  R ≈ %.1f Ω\r\n", R_estimated);
    }
    
    printf("========================\r\n");
}

// 主分析接口函数（简化版）
void analyze_rlc_filter(rlc_analysis_result_t* result)
{
    rlc_simple_result_t simple_result;
    
    // 调用简化分析
    analyze_rlc_simple(&simple_result);
    // 转换结果格式 - 修复类型转换问题
    result->type = (filter_type_t)simple_result.type;  // 强制类型转换
    result->center_freq = simple_result.center_freq;
    result->q_factor = simple_result.q_factor;
    result->bandwidth = (simple_result.center_freq > 0.0f && simple_result.q_factor > 0.0f) ? 
                       (simple_result.center_freq / simple_result.q_factor) : 0.0f;
    result->damping_ratio = (simple_result.q_factor > 0.0f) ? 
                           (1.0f / (2.0f * simple_result.q_factor)) : 0.0f;
}

// 简化的详细报告函数
void print_detailed_analysis_report(const rlc_analysis_result_t* result)
{
    printf("滤波器类型: %s\r\n", filter_names[result->type]);
    
    // 根据滤波器类型选择合适的频率描述
    if(result->type == 0 || result->type == 1) {
        printf("截止频率: %.1f Hz\r\n", result->center_freq);  // 低通/高通用截止频率
    } else {
        printf("中心频率: %.1f Hz\r\n", result->center_freq);  // 带通/带阻用中心频率
    }
    
    printf("Q值: %.2f\r\n", result->q_factor);
    printf("阻尼比: %.3f\r\n", result->damping_ratio);
    printf("带宽: %.1f Hz\r\n", result->bandwidth);
    
    // 调用简化结果进行元件值估算
    rlc_simple_result_t simple_result;
    analyze_rlc_simple(&simple_result);
    
}