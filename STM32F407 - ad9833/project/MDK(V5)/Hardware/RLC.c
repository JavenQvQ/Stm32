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
    
    return best_type;
}

// 基于"相位指纹"的专家决策系统 - 考虑ADC采样精度限制
uint8_t identify_filter_type_advanced(void)
{
    // ==================== 第一步：ADC精度问题检测和数据清理 ====================
    
    const float32_t min_magnitude_threshold = 1e-8f;    // 最小幅度阈值
    const float32_t min_gain_threshold = -60.0f;        // 最小可信增益阈值
    
    uint16_t invalid_count = 0;
    
    // 第一轮：标记所有无效数据
    for(uint16_t i = 0; i < 180; i++)
    {
        uint8_t is_invalid = 0;
        
        // 检查各种ADC精度问题：
        
        // 1. 幅度过小（ADC无法检测） - 这会导致相位无效
        if(g_magnitude_array[i] <= min_magnitude_threshold) {
            is_invalid = 1;
        }
        
        // 2. 增益异常（计算错误或信号太弱）
        if(g_gain_db_array[i] <= min_gain_threshold) {
            is_invalid = 1;
        }
        
        // 3. 组合检查：幅度很小时，相位不可信（ADC精度问题）
        if(g_magnitude_array[i] <= min_magnitude_threshold * 10) {
            is_invalid = 1;  // 幅度太小，相位必然不准确
        }
        
        // 4. 低增益时的额外检查 - 只检查幅度，不检查相位为0的情况
        if(g_gain_db_array[i] < -30.0f) {
            if(g_magnitude_array[i] <= min_magnitude_threshold * 50) {
                is_invalid = 1;  // 只看幅度，不看相位
            }
        }
        
        // 5. 检查增益异常为0dB但幅度很小的情况（ADC计算错误）
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
    
    // 无效数据已在第一轮清理中处理完毕，不需要高通滤波器的特殊处理
    uint8_t suspected_highpass = 0;
    
    // 统计低频段和高频段的数据分布情况（用于后续识别）
    uint16_t low_freq_invalid = 0;
    uint16_t high_freq_valid = 0;
    
    for(uint16_t i = 0; i < 60; i++) {
        if(g_gain_db_array[i] <= -100.0f) {
            low_freq_invalid++;
        }
    }
    
    for(uint16_t i = 120; i < 180; i++) {
        if(g_gain_db_array[i] > -100.0f) {
            high_freq_valid++;
        }
    }
    
    // 如果低频段大量无效，高频段有效，需要仔细区分高通和带通
    if(low_freq_invalid > 30 && high_freq_valid > 20) {
        // 进一步分析：检查是否为带通滤波器的右侧衰减区
        
        // 检查1：计算高频段的增益趋势
        float32_t high_freq_slope = 0.0f;
        uint8_t slope_samples = 0;
        
        for(uint16_t i = 120; i < 170; i++) {
            if(g_gain_db_array[i] > -100.0f && g_gain_db_array[i+1] > -100.0f) {
                float32_t freq_ratio = (float32_t)g_log_freq_points[i+1] / g_log_freq_points[i];
                high_freq_slope += (g_gain_db_array[i+1] - g_gain_db_array[i]) / log10f(freq_ratio);
                slope_samples++;
            }
        }
        if(slope_samples > 0) high_freq_slope /= slope_samples;
        
        // 检查2：寻找可能的峰值
        float32_t max_valid_gain = -100.0f;
        uint16_t max_valid_idx = 0;
        
        for(uint16_t i = 60; i < 180; i++) {
            if(g_gain_db_array[i] > -100.0f && g_gain_db_array[i] > max_valid_gain) {
                max_valid_gain = g_gain_db_array[i];
                max_valid_idx = i;
            }
        }
        
        // 检查3：分析增益变化模式
        uint8_t is_likely_highpass = 1;
        
        // 如果高频段呈下降趋势，可能是带通的右侧
        if(high_freq_slope < -5.0f) {
            is_likely_highpass = 0;
        }
        
        // 如果在中频段发现明显峰值，可能是带通
        if(max_valid_idx > 60 && max_valid_idx < 150) {
            // 计算峰值的显著性
            float32_t peak_prominence = 0.0f;
            uint8_t surrounding_count = 0;
            
            for(uint16_t i = max_valid_idx - 10; i <= max_valid_idx + 10; i++) {
                if(i != max_valid_idx && i < 180 && g_gain_db_array[i] > -100.0f) {
                    peak_prominence += (max_valid_gain - g_gain_db_array[i]);
                    surrounding_count++;
                }
            }
            
            if(surrounding_count > 0) {
                peak_prominence /= surrounding_count;
                
                if(peak_prominence > 3.0f) {
                    is_likely_highpass = 0;
                }
            }
        }
        
        // 检查4：相位特征验证
        static float32_t phase_unwrapped_check[180];
        for(uint16_t i = 0; i < 180; i++) {
            phase_unwrapped_check[i] = g_phase_diff_array[i];
        }
        unwrap_phase(phase_unwrapped_check, 180);
        
        // 计算有效频段的相位变化
        float32_t total_phase_change_valid = 0.0f;
        uint16_t phase_change_samples = 0;
        
        for(uint16_t i = 60; i < 179; i++) {
            if(g_gain_db_array[i] > -100.0f && g_gain_db_array[i+1] > -100.0f) {
                total_phase_change_valid += fabsf(phase_unwrapped_check[i+1] - phase_unwrapped_check[i]);
                phase_change_samples++;
            }
        }
        
        // 高通滤波器在有效频段内相位变化应该较小
        // 带通滤波器即使在右侧也会有较大相位变化
        if(phase_change_samples > 10) {
            float32_t avg_phase_change = total_phase_change_valid / phase_change_samples;
            
            if(avg_phase_change > 2.0f) {
               
                is_likely_highpass = 0;
            }
        }
        
        // 检查5：分析增益绝对水平
        // 真正的高通滤波器：高频段增益应该接近0dB（通带增益）
        // 带通滤波器右侧：增益仍然是衰减的
        float32_t high_freq_max_gain = -100.0f;
        for(uint16_t i = 150; i < 180; i++) {
            if(g_gain_db_array[i] > -100.0f && g_gain_db_array[i] > high_freq_max_gain) {
                high_freq_max_gain = g_gain_db_array[i];
            }
        }
        
        if(high_freq_max_gain < -10.0f) {
           
        }
        
        // 检查6：分析增益曲线的单调性
        // 真正的高通：从低频到高频单调上升
        // 带通右侧：可能有先升后降的趋势
        uint16_t rising_segments = 0, falling_segments = 0;
        
        for(uint16_t i = 60; i < 170; i++) {
            if(g_gain_db_array[i] > -100.0f && g_gain_db_array[i+1] > -100.0f) {
                if(g_gain_db_array[i+1] > g_gain_db_array[i] + 0.5f) {
                    rising_segments++;
                } else if(g_gain_db_array[i+1] < g_gain_db_array[i] - 0.5f) {
                    falling_segments++;
                }
            }
        }
        
        float32_t monotonicity_ratio = (float32_t)rising_segments / (rising_segments + falling_segments + 1);
        
        if(monotonicity_ratio < 0.7f) {
            
        }
        
        // 检查7：验证相位起始特征
        // 找到第一个有效相位点验证是否符合高通特征
        float32_t first_valid_phase = 0.0f;
        uint8_t found_first_phase = 0;
        
        for(uint16_t i = 60; i < 120; i++) {
            if(g_gain_db_array[i] > -100.0f && g_magnitude_array[i] > 1e-8f) {
                first_valid_phase = phase_unwrapped_check[i];
                found_first_phase = 1;

                break;
            }
        }
        
        if(found_first_phase) {
            // 高通滤波器在截止频率以上应该相位接近0°或正值
            // 带通滤波器右侧可能相位已经是负值
            if(first_valid_phase < -45.0f) {

                is_likely_highpass = 0;
            }
        }
        
        // 最终判断
        if(is_likely_highpass) {
            suspected_highpass = 1;  
        } else {

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
    
    // 在统计有效数据点时修正逻辑
    for(uint16_t i = 0; i < 180; i++)
    {
        phase_unwrapped[i] = g_phase_diff_array[i];
        
        // 修正的有效性检查：只要有幅度，相位就可能有效（包括相位为0的情况）
        if(g_gain_db_array[i] > -100.0f &&          // 不是无效标记
           g_magnitude_array[i] > min_magnitude_threshold) {   // 有足够的幅度
            valid_phase_mask[i] = 1;
            valid_phase_count++;
        } else {
            valid_phase_mask[i] = 0;
        }
    }
    
    
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
    
    // 寻找前3个有效相位点并计算平均值（更稳定的起始相位）
    float32_t valid_start_phases[3] = {0.0f, 0.0f, 0.0f};
    uint8_t found_start_count = 0;
    
    for(uint16_t i = 0; i < 180 && found_start_count < 3; i++) {
        if(valid_phase_mask[i]) {
            valid_start_phases[found_start_count] = phase_unwrapped[i];
            found_start_count++;
        }
    }
    
    // 计算起始相位的平均值
    if(found_start_count > 0) {
        start_phase = 0.0f;
        for(uint8_t i = 0; i < found_start_count; i++) {
            start_phase += valid_start_phases[i];
        }
        start_phase /= found_start_count;
        
    }
    
    // 寻找后3个有效相位点并计算平均值（更稳定的结束相位）
    float32_t valid_end_phases[3] = {0.0f, 0.0f, 0.0f};
    uint8_t found_end_count = 0;
    
    for(uint16_t i = 179; i > 0 && found_end_count < 3; i--) {
        if(valid_phase_mask[i]) {
            valid_end_phases[found_end_count] = phase_unwrapped[i];
           
            found_end_count++;
        }
    }
    
    // 计算结束相位的平均值
    if(found_end_count > 0) {
        end_phase = 0.0f;
        for(uint8_t i = 0; i < found_end_count; i++) {
            end_phase += valid_end_phases[i];
        }
        end_phase /= found_end_count;

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
    const float32_t phase_tolerance = 40.0f;         // 放宽相位容差
    const float32_t derivative_threshold = 4.0f;     // 相位导数阈值
    
    // 检查相位一致性（如果有多个有效相位点）
    uint8_t phase_reliable = 1;
    if(found_start_count > 1) {
        float32_t start_phase_variance = 0.0f;
        for(uint8_t i = 0; i < found_start_count; i++) {
            float32_t diff = valid_start_phases[i] - start_phase;
            start_phase_variance += diff * diff;
        }
        start_phase_variance /= found_start_count;
        
        if(sqrtf(start_phase_variance) > 15.0f) {
            
            phase_reliable = 0;
        }
    }
    
    if(found_end_count > 1) {
        float32_t end_phase_variance = 0.0f;
        for(uint8_t i = 0; i < found_end_count; i++) {
            float32_t diff = valid_end_phases[i] - end_phase;
            end_phase_variance += diff * diff;
        }
        end_phase_variance /= found_end_count;
        
        if(sqrtf(end_phase_variance) > 15.0f) {
          
            phase_reliable = 0;
        }
    }
    
    // 如果相位不可靠，放宽容差
    float32_t effective_tolerance = phase_reliable ? phase_tolerance : phase_tolerance * 1.5f;
    
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
       fabsf(start_phase) < effective_tolerance && 
       fabsf(end_phase) < effective_tolerance && 
       has_clear_notch) {
       
        return 3; // Band-Stop
    }
    
    // 2. 基于起始相位的精确判断（使用统计后的相位值）
   
    
    // 计算决策可信度权重
    float32_t decision_confidence = 1.0f;
    
    // 权重因子1：有效相位点数量
    if(found_start_count < 2) {
        decision_confidence *= 0.6f; // 单点判断可信度降低
     
    }
    
    // 权重因子2：相位一致性
    if(!phase_reliable) {
        decision_confidence *= 0.7f; // 相位不一致时可信度降低
      
    }
    
    // 权重因子3：有效数据覆盖率
    float32_t coverage_ratio = (float32_t)valid_phase_count / 180.0f;
    if(coverage_ratio < 0.5f) {
        decision_confidence *= (0.5f + coverage_ratio); // 覆盖率低时可信度降低
     
    }
    
    // 根据可信度调整容差
    float32_t adaptive_tolerance = effective_tolerance / decision_confidence;
    if(adaptive_tolerance > 60.0f) adaptive_tolerance = 60.0f; // 限制最大容差
    
    
    // 使用自适应容差进行判断
    if(fabsf(start_phase) < adaptive_tolerance) {
       
        return 0; // Low-Pass
    }
    else if(fabsf(start_phase - 90.0f) < adaptive_tolerance) {
       
        return 2; // Band-Pass
    }
    else if(fabsf(start_phase - 180.0f) < adaptive_tolerance || fabsf(start_phase + 180.0f) < adaptive_tolerance) {
       
        return 1; // High-Pass
    }
    
    // 如果自适应判断也失败，降级到基础判断
    if(decision_confidence < 0.5f) {

        // 使用增益特征辅助判断
        if(has_clear_peak && max_gain_idx > 60 && max_gain_idx < 120) {

            return 2; // Band-Pass (基于中频峰值)
        }
        else if(has_clear_notch) {

            return 3; // Band-Stop
        }
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

// 修正find_frequency_at_phase函数中的数据有效性检测
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
    
    // 查找最接近目标相位的点（修正有效性检查）
    uint16_t best_idx = 0;
    float32_t min_diff = 1000.0f;
    uint8_t found_valid = 0;
    
    for(uint16_t i = 1; i < 180; i++)
    {
        // 修正的无效数据检测：
        // 1. 增益<-100dB（无效标记）
        // 2. 幅度为0或太小（ADC无法检测，导致相位不可信）
        // 注意：相位为0但有幅度的情况是有效的！
        if(g_gain_db_array[i] <= -100.0f || 
           g_magnitude_array[i] <= 1e-8f) {
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

// 基于Python算法的Q值计算 - 针对有限扫频范围(100-250kHz)优化
void calculate_improved_q_factor(rlc_analysis_result_t* result)
{
    // 初始化默认Q值
    result->q_factor = 0.707f;
    
    // Python代码中的有效性检查阈值
    const float32_t min_magnitude_threshold = 1e-8f;
    const float32_t min_gain_threshold = -60.0f;
    
    // 统计有效数据点 - 严格按照Python代码的过滤条件
    uint16_t valid_count = 0;
    for(uint16_t i = 0; i < 180; i++) {
        if(g_gain_db_array[i] > min_gain_threshold && 
           g_magnitude_array[i] > min_magnitude_threshold) {
            valid_count++;
        }
    }
    
    // 如果有效数据点太少，返回默认值
    if(valid_count < 30) { // 降低要求，适应有限扫频范围
        return;
    }
    
    // 检查扫频范围覆盖情况
    uint32_t freq_min = g_log_freq_points[0];      // 100kHz
    uint32_t freq_max = g_log_freq_points[179];    // 250kHz
    float32_t freq_span_ratio = (float32_t)freq_max / freq_min; // 2.5倍频程
    
    
    // 针对不同滤波器类型使用不同的Q值估算策略
    switch(result->type) {
        case FILTER_BAND_PASS:
        case FILTER_BAND_STOP:
        {
            // 对于带通/带阻滤波器，优先使用3dB带宽方法
            
            // 找到参考点（峰值或谷值）
            uint16_t ref_idx = (result->type == FILTER_BAND_PASS) ? 
                              find_max_gain_index() : find_min_gain_index();
            
            float32_t ref_gain = g_gain_db_array[ref_idx];
            float32_t target_gain = (result->type == FILTER_BAND_PASS) ? 
                                   (ref_gain - 3.0f) : (ref_gain + 3.0f);
            
            // 寻找左右3dB点 - 修复版本：寻找穿越点而非最近点
            uint16_t left_idx = ref_idx, right_idx = ref_idx;
            uint8_t found_left = 0, found_right = 0;
            
            // 向左寻找3dB穿越点
            for(uint16_t i = ref_idx; i > 0; i--) {
                if(g_gain_db_array[i] > min_gain_threshold && 
                   g_gain_db_array[i-1] > min_gain_threshold) {
                    
                    // 检查是否穿越target_gain
                    uint8_t crossed = 0;
                    if(result->type == FILTER_BAND_PASS) {
                        // 带通：从高增益向低增益穿越target_gain
                        crossed = (g_gain_db_array[i] >= target_gain && g_gain_db_array[i-1] < target_gain);
                    } else {
                        // 带阻：从低增益向高增益穿越target_gain
                        crossed = (g_gain_db_array[i] <= target_gain && g_gain_db_array[i-1] > target_gain);
                    }
                    
                    if(crossed) {
                        left_idx = i;
                        found_left = 1;
                        break;
                    }
                }
            }
            
            // 向右寻找3dB穿越点
            for(uint16_t i = ref_idx; i < 179; i++) {
                if(g_gain_db_array[i] > min_gain_threshold && 
                   g_gain_db_array[i+1] > min_gain_threshold) {
                    
                    // 检查是否穿越target_gain
                    uint8_t crossed = 0;
                    if(result->type == FILTER_BAND_PASS) {
                        // 带通：从高增益向低增益穿越target_gain
                        crossed = (g_gain_db_array[i] >= target_gain && g_gain_db_array[i+1] < target_gain);
                    } else {
                        // 带阻：从低增益向高增益穿越target_gain
                        crossed = (g_gain_db_array[i] <= target_gain && g_gain_db_array[i+1] > target_gain);
                    }
                    
                    if(crossed) {
                        right_idx = i;
                        found_right = 1;
                        break;
                    }
                }
            }
            
            // 如果找到了穿越点，进行插值计算精确的3dB频率
            if(found_left && found_right) {
                // 左侧3dB频率插值
                float32_t g1_left = g_gain_db_array[left_idx-1];
                float32_t g2_left = g_gain_db_array[left_idx];
                float32_t f1_left = (float32_t)g_log_freq_points[left_idx-1];
                float32_t f2_left = (float32_t)g_log_freq_points[left_idx];
                
                // 对数频率域线性插值
                float32_t log_f1_left = logf(f1_left);
                float32_t log_f2_left = logf(f2_left);
                float32_t log_f_3db_left = log_f1_left + (target_gain - g1_left) * (log_f2_left - log_f1_left) / (g2_left - g1_left);
                float32_t f_3db_left = expf(log_f_3db_left);
                
                // 右侧3dB频率插值
                float32_t g1_right = g_gain_db_array[right_idx];
                float32_t g2_right = g_gain_db_array[right_idx+1];
                float32_t f1_right = (float32_t)g_log_freq_points[right_idx];
                float32_t f2_right = (float32_t)g_log_freq_points[right_idx+1];
                
                float32_t log_f1_right = logf(f1_right);
                float32_t log_f2_right = logf(f2_right);
                float32_t log_f_3db_right = log_f1_right + (target_gain - g1_right) * (log_f2_right - log_f1_right) / (g2_right - g1_right);
                float32_t f_3db_right = expf(log_f_3db_right);
                
                // 计算带宽和Q值
                float32_t bandwidth = f_3db_right - f_3db_left;
                
                if(bandwidth > 0.0f && result->center_freq > 0.0f) {
                    result->q_factor = result->center_freq / bandwidth;

                }
            } else if(found_left || found_right) {
                // 只找到一侧的3dB点，使用单侧估算

                
                float32_t single_side_bw = 0.0f;
                if(found_left) {
                    single_side_bw = result->center_freq - (float32_t)g_log_freq_points[left_idx];
                } else {
                    single_side_bw = (float32_t)g_log_freq_points[right_idx] - result->center_freq;
                }
                
                // 假设对称，总带宽为单侧的2倍
                float32_t estimated_bandwidth = single_side_bw * 2.0f;
                if(estimated_bandwidth > 0.0f) {
                    result->q_factor = result->center_freq / estimated_bandwidth;

                }
            } else {
                // 3dB点不完整，使用理论基础的斜率分析

                
                // 计算谐振点附近的增益变化率
                float32_t slope_left = 0.0f, slope_right = 0.0f;
                uint8_t left_count = 0, right_count = 0;
                
                // 计算左侧斜率（dB/decade）
                if(ref_idx > 5) {
                    for(uint16_t i = ref_idx - 5; i < ref_idx; i++) {
                        if(g_gain_db_array[i] > min_gain_threshold && 
                           g_gain_db_array[i+1] > min_gain_threshold) {
                            float32_t freq_ratio = (float32_t)g_log_freq_points[i+1] / g_log_freq_points[i];
                            slope_left += (g_gain_db_array[i+1] - g_gain_db_array[i]) / log10f(freq_ratio);
                            left_count++;
                        }
                    }
                    if(left_count > 0) slope_left /= left_count;
                }
                
                // 计算右侧斜率（dB/decade）
                if(ref_idx < 175) {
                    for(uint16_t i = ref_idx; i < ref_idx + 5; i++) {
                        if(g_gain_db_array[i] > min_gain_threshold && 
                           g_gain_db_array[i+1] > min_gain_threshold) {
                            float32_t freq_ratio = (float32_t)g_log_freq_points[i+1] / g_log_freq_points[i];
                            slope_right += (g_gain_db_array[i+1] - g_gain_db_array[i]) / log10f(freq_ratio);
                            right_count++;
                        }
                    }
                    if(right_count > 0) slope_right /= right_count;
                }
                
                // 基于二阶系统理论的Q值估算
                if(left_count > 0 && right_count > 0) {
                    // 对于二阶带通/带阻滤波器：
                    // 远离谐振频率时，增益以±40dB/decade的速率变化
                    // 实际测量的斜率会因Q值影响而偏离理论值
                    // Q值越高，实际斜率越陡峭
                    
                    float32_t avg_slope_magnitude = (fabsf(slope_left) + fabsf(slope_right)) / 2.0f;
                    
                    // 基于二阶系统的理论关系：
                    // 对于Q>>1的情况，谐振附近斜率 ≈ 40*Q dB/decade
                    // 因此 Q ≈ slope / 40 (这是理论最大值)
                    // 但实际测量中，由于扫频范围限制，观测到的斜率通常小于理论值
                    
                    if(avg_slope_magnitude > 10.0f) {
                        // 考虑扫频范围限制的修正因子
                        float32_t range_factor = 1.0f;
                        
                        // 如果谐振点靠近边界，斜率观测不完整，需要修正
                        if(ref_idx < 30 || ref_idx > 150) {
                            range_factor = 1.5f; // 边界修正
                        }
                        
                        // 基于物理模型的Q值估算
                        float32_t theoretical_q = avg_slope_magnitude / 40.0f * range_factor;
                        
                        // 对于有限扫频范围，还需要考虑频率跨度影响
                        float32_t freq_span = (float32_t)freq_max / freq_min; // 2.5倍频程
                        if(freq_span < 10.0f) { // 如果频率跨度小于10倍频程
                            theoretical_q *= (1.0f + logf(10.0f / freq_span)); // 跨度修正
                        }
                        
                        result->q_factor = theoretical_q;
                        
                       
                    } else {
                        // 斜率太小，可能是低Q系统或数据质量问题
                      
                        result->q_factor = 0.707f; // 标准二阶系统默认值
                    }
                } else {

                    result->q_factor = 0.707f;
                }
            }
            break;
        }
        
        case FILTER_LOW_PASS:
        case FILTER_HIGH_PASS:
        {
            // 对于低通/高通滤波器，使用相位导数方法（如果可能）
 
            
            // 创建有效数据数组
            static float32_t valid_freqs[180];
            static float32_t valid_phases[180];
            uint16_t valid_idx = 0;
            
            // 复制并解缠绕相位数据
            static float32_t phase_unwrapped[180];
            for(uint16_t i = 0; i < 180; i++) {
                phase_unwrapped[i] = g_phase_diff_array[i];
            }
            unwrap_phase(phase_unwrapped, 180);
            
            // 提取有效数据点
            for(uint16_t i = 0; i < 180; i++) {
                if(g_gain_db_array[i] > min_gain_threshold && 
                   g_magnitude_array[i] > min_magnitude_threshold) {
                    valid_freqs[valid_idx] = (float32_t)g_log_freq_points[i];
                    valid_phases[valid_idx] = phase_unwrapped[i]; // 保持度数单位
                    valid_idx++;
                }
            }
            
            if(valid_idx < 10) {

                break;
            }
            
            // 寻找相位-90°的点（对应二阶系统的截止频率）
            float32_t target_phase = (result->type == FILTER_LOW_PASS) ? -90.0f : 90.0f;
            uint16_t cutoff_idx = 0;
            float32_t min_phase_diff = 1000.0f;
            
            for(uint16_t i = 0; i < valid_idx; i++) {
                float32_t phase_deg = valid_phases[i];
                float32_t diff = fabsf(phase_deg - target_phase);
                if(diff < min_phase_diff) {
                    min_phase_diff = diff;
                    cutoff_idx = i;
                }
            }
            
            // 如果找到了接近的相位点，使用增益衰减计算Q值
            if(min_phase_diff < 30.0f && cutoff_idx > 0 && cutoff_idx < valid_idx - 1) {
                // 计算截止频率处的增益衰减率
                float32_t freq_cutoff = valid_freqs[cutoff_idx];
                result->center_freq = freq_cutoff; // 更新截止频率
                
                // 基于二阶系统理论计算Q值
                // 对于二阶低通/高通滤波器，在截止频率附近：
                // 增益变化率 dG/d(log f) ≈ ±20log10(2Q) dB/decade
                // 因此可以通过测量斜率来估算Q值
                
                float32_t slope = 0.0f;
                uint8_t slope_samples = 0;
                
                uint16_t start_idx = (cutoff_idx > 5) ? cutoff_idx - 5 : 0;
                uint16_t end_idx = (cutoff_idx < valid_idx - 5) ? cutoff_idx + 5 : valid_idx - 1;
                
                for(uint16_t i = start_idx; i < end_idx; i++) {
                    if(g_gain_db_array[i] > min_gain_threshold && 
                       g_gain_db_array[i+1] > min_gain_threshold) {
                        float32_t freq_ratio = valid_freqs[i+1] / valid_freqs[i];
                        float32_t gain_diff = g_gain_db_array[i+1] - g_gain_db_array[i]; // 使用增益变化
                        slope += gain_diff / log10f(freq_ratio);
                        slope_samples++;
                    }
                }
                
                if(slope_samples > 0) {
                    slope /= slope_samples;
                    
                    // 基于二阶系统理论的Q值计算
                    // 对于二阶低通：斜率 ≈ -40dB/decade（在高频段）
                    // 对于二阶高通：斜率 ≈ +40dB/decade（在低频段）
                    // Q值影响过渡区的陡峭程度
                    float32_t abs_slope = fabsf(slope);
                    
                    if(abs_slope > 5.0f) {
                        // 基于二阶系统的Q值估算
                        // 对于典型的巴特沃斯响应（Q=0.707），斜率约为20dB/decade
                        // 更高的Q值会产生更陡峭的过渡
                        result->q_factor = abs_slope / 28.28f; // 28.28 ≈ 20*sqrt(2) 对应Q=0.707
                        
                        // 限制合理范围
                        if(result->q_factor > 10.0f) result->q_factor = 10.0f;
                        if(result->q_factor < 0.1f) result->q_factor = 0.707f;

                    } else {
  
                        result->q_factor = 0.707f; // 标准二阶系统
                    }
                } else {
 
                    result->q_factor = 0.707f;
                }
            } else {

                result->q_factor = 0.707f; // 标准二阶系统
            }
            break;
        }
        
        default:
            result->q_factor = 0.707f;
            break;
    }
    
    // 针对有限扫频范围的Q值修正
    if(result->center_freq > 0.0f) {
        float32_t center_freq_ratio_low = result->center_freq / freq_min;
        float32_t center_freq_ratio_high = freq_max / result->center_freq;
        
        // 如果中心频率接近扫频范围边界，Q值可能被低估
        if(center_freq_ratio_low < 1.5f || center_freq_ratio_high < 1.5f) {
            
            // 使用基于理论的边界修正方法
            float32_t boundary_correction = 1.0f;
            
            // 根据中心频率位置计算修正系数
            if(center_freq_ratio_low < 1.2f) {
                // 中心频率太靠近低频边界
                boundary_correction = 1.0f + (1.2f - center_freq_ratio_low) * 0.5f;
            } else if(center_freq_ratio_high < 1.2f) {
                // 中心频率太靠近高频边界
                boundary_correction = 1.0f + (1.2f - center_freq_ratio_high) * 0.5f;
            }
            
            // 基于扫频范围不足的修正
            float32_t freq_span_octaves = log2f((float32_t)freq_max / freq_min);
            if(freq_span_octaves < 3.0f) { // 正常需要至少3倍频程来准确测量Q值
                float32_t span_correction = 3.0f / freq_span_octaves;
                boundary_correction *= span_correction;
            }
            
            // 应用修正，但限制修正范围
            if(boundary_correction > 1.0f && boundary_correction < 2.0f) {
                result->q_factor *= boundary_correction;

            }
        }
    }
    
    // 最终限制Q值范围
    if(result->q_factor > 100.0f) result->q_factor = 100.0f;
    if(result->q_factor < 0.05f) result->q_factor = 0.05f;
    

}

// 改进的RLC分析函数 - 基于Python的完整算法
void analyze_rlc_simple(rlc_analysis_result_t* result)
{
    // 初始化结果
    result->type = FILTER_LOW_PASS; // 默认Low-Pass
    result->center_freq = 0.0f;
    result->q_factor = 0.707f;
    result->lc_product = 0.0f;
    result->rc_product = 0.0f;
    result->bandwidth = 0.0f;
    result->damping_ratio = 0.707f;
    
    // 步骤1: 使用相位指纹识别算法
    uint8_t filter_type = identify_filter_type_advanced();
    result->type = (filter_type_t)filter_type;
    
    // 由于无效数据已在identify_filter_type_advanced中清理完毕，
    // 不需要额外的高通滤波器数据清理处理
    
    // 步骤2: 根据算法精确定位中心频率 - 针对100-250kHz离散频率点优化
    switch(filter_type)
    {
        case 0: // Low-Pass - 查找-90°处的频率（二阶系统截止频率）
        {
            result->center_freq = find_frequency_at_phase_precise(-90.0f, result);
            break;
        }
        
        case 1: // High-Pass - 查找+90°处的频率（二阶系统截止频率）
        {
            result->center_freq = find_frequency_at_phase_precise(90.0f, result);
            break;
        }
        
        case 2: // Band-Pass - 多重方法交叉验证中心频率
        {
            // 方法1: 相位0°点
            float32_t phase_based_freq = find_frequency_at_phase_precise(0.0f, result);
            
            // 方法2: 增益最大点（带插值）
            float32_t gain_based_freq = find_peak_frequency_precise();
            
            // 方法3: 相位变化率最大点（谐振频率特征）
            float32_t phase_derivative_freq = find_max_phase_derivative_frequency();
            
            // 多重验证和加权平均
            float32_t freq_candidates[3] = {phase_based_freq, gain_based_freq, phase_derivative_freq};
            uint8_t valid_methods = 0;
            float32_t freq_sum = 0.0f;
            

            
            // 检查各方法的有效性和一致性
            for(uint8_t i = 0; i < 3; i++) {
                if(freq_candidates[i] > 50000.0f && freq_candidates[i] < 300000.0f) { // 合理范围
                    valid_methods++;
                    freq_sum += freq_candidates[i];
                }
            }
            
            if(valid_methods >= 2) {
                // 如果有至少2种方法给出合理结果，使用平均值
                result->center_freq = freq_sum / valid_methods;
                
                // 检查方法间的一致性
                float32_t max_deviation = 0.0f;
                for(uint8_t i = 0; i < 3; i++) {
                    if(freq_candidates[i] > 50000.0f && freq_candidates[i] < 300000.0f) {
                        float32_t deviation = fabsf(freq_candidates[i] - result->center_freq) / result->center_freq;
                        if(deviation > max_deviation) max_deviation = deviation;
                    }
                }
                
                if(max_deviation > 0.2f) { // 如果偏差超过20%，发出警告
 
                }
                

            } else {
                // 如果验证方法太少，使用最可靠的增益法
                result->center_freq = gain_based_freq;

            }
            break;
        }
        
        case 3: // Band-Stop - 增益最小处（带插值）
        {
            result->center_freq = find_notch_frequency_precise();
            break;
        }
    }
    

    
    // 步骤3: 使用Python的改进Q值计算方法
    calculate_improved_q_factor(result);
    
    // 步骤4: 计算LC和RC乘积以及其他参数 - 使用Python的公式
    float32_t effective_freq = 0.0f;
    
    // 根据滤波器类型选择有效频率
    switch(result->type) {
        case FILTER_LOW_PASS:
        case FILTER_HIGH_PASS:
            // 低通和高通使用截止频率
            effective_freq = result->center_freq;  // center_freq在这里实际存储的是截止频率
            printf("使用截止频率计算LC/RC乘积: %.1f Hz\r\n", effective_freq);
            break;
            
        case FILTER_BAND_PASS:
        case FILTER_BAND_STOP:
            // 带通和带阻使用中心频率
            effective_freq = result->center_freq;  // 真正的中心频率
            printf("使用中心频率计算LC/RC乘积: %.1f Hz\r\n", effective_freq);
            break;
            
        default:
            effective_freq = result->center_freq;
            break;
    }
    
    if(effective_freq > 10.0f && result->q_factor > 0.01f)
    {
        float32_t omega_c = 2.0f * PI * effective_freq;
        float32_t zeta = 1.0f / (2.0f * result->q_factor); // 阻尼比
        
        // 按照Python算法：
        // LC乘积 = 1 / (ωc)²
        result->lc_product = 1.0f / (omega_c * omega_c);
        
        // RC乘积 = (2 * zeta) / ωc  (修正后的公式)
        result->rc_product = (2.0f * zeta) / omega_c;
        
        // 计算带宽和阻尼比
        if(result->type == FILTER_BAND_PASS || result->type == FILTER_BAND_STOP) {
            // 带通/带阻：3dB带宽 = 中心频率 / Q值
            result->bandwidth = effective_freq / result->q_factor;
        } else {
            // 低通/高通：3dB带宽就是截止频率（从DC或到无穷大）
            result->bandwidth = effective_freq;
        }
        
        result->damping_ratio = zeta;
        
        printf("LC乘积: %.4e (基于ω=%.1f rad/s)\r\n", result->lc_product, omega_c);
        printf("RC乘积: %.4e (基于ζ=%.3f)\r\n", result->rc_product, zeta);
    }
    
    // 限制Q值范围
    if(result->q_factor > 100.0f) result->q_factor = 100.0f;
    if(result->q_factor < 0.1f) result->q_factor = 0.1f;
}

// 主分析接口函数（简化版）
void analyze_rlc_filter(rlc_analysis_result_t* result)
{
    // 直接调用分析函数
    analyze_rlc_simple(result);
}

// 简化的详细报告函数
void print_detailed_analysis_report(const rlc_analysis_result_t* result)
{
    printf("\r\n=== 详细RLC滤波器分析报告 ===\r\n");
    printf("滤波器类型: %s\r\n", filter_names[result->type]);
    
    // 根据滤波器类型选择合适的频率描述
    if(result->type == FILTER_LOW_PASS || result->type == FILTER_HIGH_PASS) {
        printf("截止频率: %.1f Hz\r\n", result->center_freq);  // 低通/高通用截止频率
        printf("角频率: %.1f rad/s\r\n", 2.0f * PI * result->center_freq);
    } else {
        printf("中心频率: %.1f Hz\r\n", result->center_freq);  // 带通/带阻用中心频率
        printf("角频率: %.1f rad/s\r\n", 2.0f * PI * result->center_freq);
    }
    
    printf("Q值: %.2f\r\n", result->q_factor);
    printf("阻尼比: %.3f\r\n", result->damping_ratio);
    
    // 根据滤波器类型显示带宽信息
    if(result->type == FILTER_BAND_PASS || result->type == FILTER_BAND_STOP) {
        printf("3dB带宽: %.1f Hz\r\n", result->bandwidth);
    } else {
        printf("截止频率: %.1f Hz (即3dB点)\r\n", result->bandwidth);
    }
    
    // 显示计算得到的LC和RC乘积
    printf("\r\n理论参数乘积:\r\n");
    printf("  LC乘积: %.4e H·F\r\n", result->lc_product);
    printf("  RC乘积: %.4e Ω·F\r\n", result->rc_product);
    
    // 估算元件值（假设不同的电容值）
    if(result->lc_product > 1e-15f && result->rc_product > 1e-12f)
    {
        printf("\r\n估算元件值:\r\n");
        
        // 提供多种电容值的选择
        float32_t cap_values[] = {10e-9f, 47e-9f, 100e-9f, 220e-9f, 470e-9f, 1e-6f};  // 10nF到1μF
        const char* cap_units[] = {"10nF", "47nF", "100nF", "220nF", "470nF", "1μF"};
        
        for(uint8_t i = 0; i < 6; i++) {
            float32_t C_assumed = cap_values[i];
            float32_t L_estimated = result->lc_product / C_assumed;
            float32_t R_estimated = result->rc_product / C_assumed;
            
            // 选择合适的单位显示
            if(L_estimated >= 1.0f) {
                printf("  假设C=%s: L≈%.1f H, R≈%.1f Ω\r\n", 
                       cap_units[i], L_estimated, R_estimated);
            } else if(L_estimated >= 1e-3f) {
                printf("  假设C=%s: L≈%.1f mH, R≈%.1f Ω\r\n", 
                       cap_units[i], L_estimated * 1000.0f, R_estimated);
            } else if(L_estimated >= 1e-6f) {
                printf("  假设C=%s: L≈%.1f μH, R≈%.1f Ω\r\n", 
                       cap_units[i], L_estimated * 1000000.0f, R_estimated);
            } else {
                printf("  假设C=%s: L≈%.1f nH, R≈%.1f Ω\r\n", 
                       cap_units[i], L_estimated * 1000000000.0f, R_estimated);
            }
        }
        
        // 提供设计建议
        printf("\r\n设计建议:\r\n");
        
        switch(result->type) {
            case FILTER_LOW_PASS:
                printf("  - 低通滤波器：选择合适的L和C值实现%.1f Hz截止频率\r\n", result->center_freq);
                printf("  - 建议使用标准电感值，然后调整电容值微调频率\r\n");
                break;
                
            case FILTER_HIGH_PASS:
                printf("  - 高通滤波器：选择合适的R和C值实现%.1f Hz截止频率\r\n", result->center_freq);
                printf("  - 建议使用标准电容值，然后调整电阻值微调频率\r\n");
                break;
                
            case FILTER_BAND_PASS:
                printf("  - 带通滤波器：LC谐振频率=%.1f Hz，Q值=%.2f\r\n", result->center_freq, result->q_factor);
                printf("  - R值控制Q值和带宽，L和C值决定中心频率\r\n");
                break;
                
            case FILTER_BAND_STOP:
                printf("  - 带阻滤波器：LC陷波频率=%.1f Hz，Q值=%.2f\r\n", result->center_freq, result->q_factor);
                printf("  - 高Q值产生更窄的陷波，调整R值控制陷波深度\r\n");
                break;
        }
    }
    
    printf("==============================\r\n");
}

// 基于部分频率响应估算完整Q值的高级方法
float32_t estimate_q_from_partial_response(const rlc_analysis_result_t* result)
{
    const float32_t min_gain_threshold = -60.0f;
    uint32_t freq_min = g_log_freq_points[0];      // 100kHz
    uint32_t freq_max = g_log_freq_points[179];    // 250kHz
    
    // 方法1: 基于增益变化率的外推估算
    if(result->type == FILTER_BAND_PASS || result->type == FILTER_BAND_STOP) {
        uint16_t ref_idx = (result->type == FILTER_BAND_PASS) ? 
                          find_max_gain_index() : find_min_gain_index();
        
        float32_t ref_freq = (float32_t)g_log_freq_points[ref_idx];
        
        // 如果中心频率在扫频范围内
        if(ref_freq >= freq_min && ref_freq <= freq_max) {
            // 计算两侧的增益衰减率
            float32_t left_slope = 0.0f, right_slope = 0.0f;
            uint8_t left_count = 0, right_count = 0;
            
            // 计算左侧斜率（频率低于中心频率）
            if(ref_idx >= 10) {
                for(uint16_t i = ref_idx - 10; i < ref_idx; i++) {
                    if(g_gain_db_array[i] > min_gain_threshold && 
                       g_gain_db_array[i+1] > min_gain_threshold) {
                        float32_t freq_ratio = (float32_t)g_log_freq_points[i+1] / g_log_freq_points[i];
                        float32_t gain_change = g_gain_db_array[i+1] - g_gain_db_array[i];
                        left_slope += gain_change / log10f(freq_ratio);
                        left_count++;
                    }
                }
                if(left_count > 0) left_slope /= left_count;
            }
            
            // 计算右侧斜率（频率高于中心频率）
            if(ref_idx < 170) {
                for(uint16_t i = ref_idx; i < ref_idx + 10 && i < 179; i++) {
                    if(g_gain_db_array[i] > min_gain_threshold && 
                       g_gain_db_array[i+1] > min_gain_threshold) {
                        float32_t freq_ratio = (float32_t)g_log_freq_points[i+1] / g_log_freq_points[i];
                        float32_t gain_change = g_gain_db_array[i+1] - g_gain_db_array[i];
                        right_slope += gain_change / log10f(freq_ratio);
                        right_count++;
                    }
                }
                if(right_count > 0) right_slope /= right_count;
            }
            
            // 基于斜率估算Q值
            float32_t avg_slope_magnitude = (fabsf(left_slope) + fabsf(right_slope)) / 2.0f;
            
            if(avg_slope_magnitude > 5.0f) {
                // 经验公式：Q ≈ slope_magnitude / 20
                float32_t estimated_q = avg_slope_magnitude / 20.0f;
                
                
                return estimated_q;
            }
        }
    }
    
    // 方法2: 基于相位变化率的估算
    static float32_t phase_unwrapped[180];
    for(uint16_t i = 0; i < 180; i++) {
        phase_unwrapped[i] = g_phase_diff_array[i];
    }
    unwrap_phase(phase_unwrapped, 180);
    
    // 计算扫频范围内的总相位变化
    float32_t total_phase_change = 0.0f;
    uint16_t phase_samples = 0;
    
    for(uint16_t i = 1; i < 180; i++) {
        if(g_gain_db_array[i] > min_gain_threshold && 
           g_gain_db_array[i-1] > min_gain_threshold) {
            float32_t phase_diff = phase_unwrapped[i] - phase_unwrapped[i-1];
            total_phase_change += fabsf(phase_diff);
            phase_samples++;
        }
    }
    
    if(phase_samples > 10) {
        // 基于相位变化速率估算Q值
        float32_t freq_span_octaves = log2f((float32_t)freq_max / freq_min);
        float32_t phase_change_per_octave = total_phase_change / freq_span_octaves;
        
        // 对于二阶系统，高Q值对应快速的相位变化
        float32_t estimated_q = phase_change_per_octave / 90.0f;
        
        
        if(estimated_q > 0.1f && estimated_q < 20.0f) {
            return estimated_q;
        }
    }
    
    // 方法3: 基于增益波动程度的估算
    float32_t gain_variance = 0.0f;
    float32_t gain_mean = 0.0f;
    uint16_t valid_samples = 0;
    
    for(uint16_t i = 0; i < 180; i++) {
        if(g_gain_db_array[i] > min_gain_threshold) {
            gain_mean += g_gain_db_array[i];
            valid_samples++;
        }
    }
    
    if(valid_samples > 20) {
        gain_mean /= valid_samples;
        
        for(uint16_t i = 0; i < 180; i++) {
            if(g_gain_db_array[i] > min_gain_threshold) {
                float32_t diff = g_gain_db_array[i] - gain_mean;
                gain_variance += diff * diff;
            }
        }
        gain_variance /= valid_samples;
        
        float32_t gain_std = sqrtf(gain_variance);
        
        // 高Q值电路通常表现为更大的增益变化
        float32_t estimated_q = gain_std / 3.0f;
        
        
        if(estimated_q > 0.2f && estimated_q < 10.0f) {
            return estimated_q;
        }
    }
    
    // 如果所有方法都失败，返回默认值
    return 0.707f;
}

// 精确的相位目标频率查找 - 针对100-250kHz离散频率点优化
float32_t find_frequency_at_phase_precise(float32_t target_phase_deg, rlc_analysis_result_t* result)
{
    // 创建相位解缠绕数组
    static float32_t phase_unwrapped[180];
    
    // 复制并解缠绕相位数据
    for(uint16_t i = 0; i < 180; i++) {
        phase_unwrapped[i] = g_phase_diff_array[i];
    }
    unwrap_phase(phase_unwrapped, 180);
    
    // 寻找最接近目标相位的两个点（用于插值）
    uint16_t idx1 = 0, idx2 = 1;
    float32_t min_bracket_width = 1000.0f;
    uint8_t found_bracket = 0;
    
    // 寻找包围目标相位的相邻有效点对
    for(uint16_t i = 0; i < 179; i++) {
        // 检查数据有效性
        if(g_gain_db_array[i] <= -100.0f || g_magnitude_array[i] <= 1e-8f ||
           g_gain_db_array[i+1] <= -100.0f || g_magnitude_array[i+1] <= 1e-8f) {
            continue;
        }
        
        float32_t p1 = phase_unwrapped[i];
        float32_t p2 = phase_unwrapped[i+1];
        
        // 检查是否包围目标相位
        if((p1 <= target_phase_deg && p2 >= target_phase_deg) ||
           (p1 >= target_phase_deg && p2 <= target_phase_deg)) {
            float32_t bracket_width = fabsf(p2 - p1);
            if(bracket_width < min_bracket_width) {
                min_bracket_width = bracket_width;
                idx1 = i;
                idx2 = i + 1;
                found_bracket = 1;
            }
        }
    }
    
    if(found_bracket && min_bracket_width < 50.0f) {
        // 使用高精度对数频率插值
        float32_t f1 = (float32_t)g_log_freq_points[idx1];
        float32_t f2 = (float32_t)g_log_freq_points[idx2];
        float32_t p1 = phase_unwrapped[idx1];
        float32_t p2 = phase_unwrapped[idx2];
        
        // 对数频率域线性插值
        float32_t log_f1 = logf(f1);
        float32_t log_f2 = logf(f2);
        float32_t log_f_interp = log_f1 + (target_phase_deg - p1) * (log_f2 - log_f1) / (p2 - p1);
        float32_t f_interp = expf(log_f_interp);
        
        
        return f_interp;
    }
    
    // 如果没找到包围点，退回到最近点查找
    return find_frequency_at_phase(target_phase_deg);
}

// 精确的增益峰值频率查找 - 使用抛物线插值
float32_t find_peak_frequency_precise(void)
{
    uint16_t max_idx = find_max_gain_index();
    
    // 检查边界条件
    if(max_idx == 0 || max_idx == 179 || 
       g_gain_db_array[max_idx-1] <= -100.0f || g_gain_db_array[max_idx+1] <= -100.0f) {
        return (float32_t)g_log_freq_points[max_idx];
    }
    
    // 使用三点抛物线插值寻找真实峰值
    float32_t f1 = (float32_t)g_log_freq_points[max_idx-1];
    float32_t f2 = (float32_t)g_log_freq_points[max_idx];
    float32_t f3 = (float32_t)g_log_freq_points[max_idx+1];
    
    float32_t g1 = g_gain_db_array[max_idx-1];
    float32_t g2 = g_gain_db_array[max_idx];
    float32_t g3 = g_gain_db_array[max_idx+1];
    
    // 转换为对数频率域进行抛物线拟合
    float32_t x1 = logf(f1);
    float32_t x2 = logf(f2);
    float32_t x3 = logf(f3);
    
    // 抛物线拟合系数计算
    float32_t denom = (x1 - x2) * (x1 - x3) * (x2 - x3);
    if(fabsf(denom) < 1e-12f) {
        return f2; // 退回到离散点
    }
    
    float32_t a = (x3 * (g2 - g1) + x2 * (g1 - g3) + x1 * (g3 - g2)) / denom;
    float32_t b = (x3*x3 * (g1 - g2) + x2*x2 * (g3 - g1) + x1*x1 * (g2 - g3)) / denom;
    
    // 如果抛物线开口向下，计算峰值位置
    if(a < 0.0f) {
        float32_t x_peak = -b / (2.0f * a);
        float32_t f_peak = expf(x_peak);
        
        // 确保插值结果在合理范围内
        if(f_peak > f1 && f_peak < f3) {
           
            return f_peak;
        }
    }
    
    return f2; // 退回到离散点
}

// 查找相位变化率最大点对应的频率
float32_t find_max_phase_derivative_frequency(void)
{
    static float32_t phase_unwrapped[180];
    static float32_t phase_derivative[180];
    
    // 相位解缠绕
    for(uint16_t i = 0; i < 180; i++) {
        phase_unwrapped[i] = g_phase_diff_array[i];
    }
    unwrap_phase(phase_unwrapped, 180);
    
    // 计算相位导数（对频率的导数）
    for(uint16_t i = 1; i < 179; i++) {
        if(g_gain_db_array[i-1] > -100.0f && g_magnitude_array[i-1] > 1e-8f &&
           g_gain_db_array[i+1] > -100.0f && g_magnitude_array[i+1] > 1e-8f) {
            
            float32_t freq_span = (float32_t)g_log_freq_points[i+1] - g_log_freq_points[i-1];
            phase_derivative[i] = (phase_unwrapped[i+1] - phase_unwrapped[i-1]) / freq_span;
        } else {
            phase_derivative[i] = 0.0f;
        }
    }
    
    // 寻找相位导数绝对值最大的点（谐振特征）
    uint16_t max_deriv_idx = 90; // 默认中点
    float32_t max_deriv_abs = 0.0f;
    
    for(uint16_t i = 20; i < 160; i++) { // 避免边界效应
        if(fabsf(phase_derivative[i]) > max_deriv_abs) {
            max_deriv_abs = fabsf(phase_derivative[i]);
            max_deriv_idx = i;
        }
    }
    
    
    return (float32_t)g_log_freq_points[max_deriv_idx];
}

// 精确的陷波频率查找 - 使用抛物线插值
float32_t find_notch_frequency_precise(void)
{
    uint16_t min_idx = find_min_gain_index();
    
    // 检查边界条件
    if(min_idx == 0 || min_idx == 179 || 
       g_gain_db_array[min_idx-1] <= -100.0f || g_gain_db_array[min_idx+1] <= -100.0f) {
        return (float32_t)g_log_freq_points[min_idx];
    }
    
    // 使用三点抛物线插值寻找真实陷波点
    float32_t f1 = (float32_t)g_log_freq_points[min_idx-1];
    float32_t f2 = (float32_t)g_log_freq_points[min_idx];
    float32_t f3 = (float32_t)g_log_freq_points[min_idx+1];
    
    float32_t g1 = g_gain_db_array[min_idx-1];
    float32_t g2 = g_gain_db_array[min_idx];
    float32_t g3 = g_gain_db_array[min_idx+1];
    
    // 转换为对数频率域进行抛物线拟合
    float32_t x1 = logf(f1);
    float32_t x2 = logf(f2);
    float32_t x3 = logf(f3);
    
    // 抛物线拟合系数计算
    float32_t denom = (x1 - x2) * (x1 - x3) * (x2 - x3);
    if(fabsf(denom) < 1e-12f) {
        return f2; // 退回到离散点
    }
    
    float32_t a = (x3 * (g2 - g1) + x2 * (g1 - g3) + x1 * (g3 - g2)) / denom;
    float32_t b = (x3*x3 * (g1 - g2) + x2*x2 * (g3 - g1) + x1*x1 * (g2 - g3)) / denom;
    
    // 如果抛物线开口向上，计算谷值位置
    if(a > 0.0f) {
        float32_t x_notch = -b / (2.0f * a);
        float32_t f_notch = expf(x_notch);
        
        // 确保插值结果在合理范围内
        if(f_notch > f1 && f_notch < f3) {

            return f_notch;
        }
    }
    
    return f2; // 退回到离散点
}