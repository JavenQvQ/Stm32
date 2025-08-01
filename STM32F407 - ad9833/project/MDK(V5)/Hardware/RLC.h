#ifndef __RLC_H
#define __RLC_H

#include "arm_math.h"

// 前向声明频率分段配置结构体
struct freq_segment_config_t;

// 滤波器类型枚举
typedef enum {
    FILTER_LOW_PASS = 0,
    FILTER_HIGH_PASS = 1,
    FILTER_BAND_PASS = 2,
    FILTER_BAND_STOP = 3,
    FILTER_UNDETERMINED = 4
} filter_type_t;

// 分析结果结构体
typedef struct {
    filter_type_t type;
    float32_t center_freq;      // 中心频率 Hz
    float32_t q_factor;         // Q值
    float32_t bandwidth;        // 带宽 Hz
    float32_t damping_ratio;    // 阻尼比 (zeta = 1/(2*Q))
    float32_t confidence;       // 置信度 0-1
} rlc_analysis_result_t;

/**
 * @brief 分析RLC滤波器的特性
 * @param result: 指向分析结果结构体的指针
 * @note  该函数通过测量数据分析RLC滤波器的类型及其关键参数，包括中心频率、Q值、带宽和阻尼比。
 *        支持的滤波器类型包括：低通、高通、带通和带阻。
 *        函数将结果存储在提供的结果结构体中，供后续使用或显示。
 */
void analyze_rlc_filter(rlc_analysis_result_t* result);

/**
 * @brief 打印详细的分析报告
 * @param result: 指向分析结果结构体的指针
 * @note  该函数用于打印RLC滤波器分析的详细报告，包括滤波器类型、中心频率、Q值、带宽和阻尼比等信息。
 *        报告格式化为易于阅读的字符串，适合在串口监控或调试输出。
 */
void print_detailed_analysis_report(const rlc_analysis_result_t* result);

#endif

