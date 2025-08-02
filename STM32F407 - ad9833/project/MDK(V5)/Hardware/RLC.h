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
} filter_type_t;

// 分析结果结构体
typedef struct {
    filter_type_t type;
    float32_t center_freq;      // 中心频率 Hz
    float32_t q_factor;         // Q值
    float32_t bandwidth;        // 带宽 Hz
    float32_t damping_ratio;    // 阻尼比 (zeta = 1/(2*Q))
    float32_t lc_product;       // LC乘积
    float32_t rc_product;       // RC乘积
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

/**
 * @brief 精确的相位目标频率查找 - 针对100-250kHz离散频率点优化
 * @param target_phase_deg: 目标相位（度）
 * @param result: 分析结果结构体指针（用于上下文信息）
 * @return 插值得到的精确频率（Hz）
 * @note  使用对数频率域插值，提高离散频率点的定位精度
 */
float32_t find_frequency_at_phase_precise(float32_t target_phase_deg, rlc_analysis_result_t* result);

/**
 * @brief 精确的增益峰值频率查找 - 使用抛物线插值
 * @return 插值得到的精确峰值频率（Hz）
 * @note  使用三点抛物线拟合来找到真实的增益峰值位置
 */
float32_t find_peak_frequency_precise(void);

/**
 * @brief 查找相位变化率最大点对应的频率
 * @return 相位导数最大处的频率（Hz）
 * @note  谐振频率处相位变化率最大，这是另一种定位中心频率的方法
 */
float32_t find_max_phase_derivative_frequency(void);

/**
 * @brief 精确的陷波频率查找 - 使用抛物线插值
 * @return 插值得到的精确陷波频率（Hz）
 * @note  用于带阻滤波器的中心频率定位
 */
float32_t find_notch_frequency_precise(void);

#endif

