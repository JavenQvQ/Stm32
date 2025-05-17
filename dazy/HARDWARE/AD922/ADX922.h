//-----------------------------------------------------------------
// ADX922驱动头文件
// 头文件名: ADX922.h
// 作    者: 凌智电子
// 开始日期: 2022-06-21
// 完成日期: 2022-06-21
// 修改日期: 2022-06-21
// 当前版本: V1.0.0
// 历史版本:
//   - V1.0: (2022-06-21)ADX922驱动头文件
//-----------------------------------------------------------------
#ifndef __ADX922_H
#define __ADX922_H
#include "sys.h"

//-----------------------------------------------------------------
// 位操作
//-----------------------------------------------------------------
// 引脚定义宏
#define PWDN_H      GPIO_SetBits(GPIOG, GPIO_Pin_6)
#define PWDN_L      GPIO_ResetBits(GPIOG, GPIO_Pin_6)
#define START_H     GPIO_SetBits(GPIOG, GPIO_Pin_7)
#define START_L     GPIO_ResetBits(GPIOG, GPIO_Pin_7)
#define CS_H        GPIO_SetBits(GPIOG, GPIO_Pin_8)
#define CS_L        GPIO_ResetBits(GPIOG, GPIO_Pin_8)
#define DRDY        GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) // 修改为PC8

//-----------------------------------------------------------------
// ADX922命令定义
//-----------------------------------------------------------------
// 系统命令
#define WAKEUP			0X02	// 从待机模式唤醒
#define STANDBY			0X04	// 进入待机模式
#define RESET				0X06	// 复位ADX922
#define START				0X08	// 启动或转换
#define STOP				0X0A	// 停止转换
#define LOCK				0X15	// 锁SPI接口
#define UNLOCK			0X16	// 解锁SPI接口
#define OFFSETCAL		0X1A	// 通道偏移校准
// 数据读取命令
#define RDATAC			0X10	// 启用连续的数据读取模式,默认使用此模式
#define SDATAC			0X11	// 停止连续的数据读取模式
#define RDATA				0X12	// 通过命令读取数据;支持多种读回。
// 寄存器读取命令
#define	RREG				0X20	// 读取001r rrrr 000n nnnn  其中r rrrr是起始寄存器地址
#define WREG				0X40	// 写入010r rrrr 000n nnnn	其中r rrrr是起始寄存器地址 n nnnn=是要写的数据*/
#define RFIFO				0X60	// 读取FIFO

// ADX922内部寄存器地址定义
// 设备设置（只读寄存器）
#define ID					0X00	// ID控制寄存器

// 跨渠道的全局设置（读写寄存器）
#define CONFIG1			0X01	// 配置寄存器1
#define CONFIG2			0X02	// 配置寄存器2
#define LOFF				0X03	// 导联脱落控制寄存器

// 通道特定设置（读写寄存器）
#define CH1SET			0X04	// 通道1设置寄存器
#define CH2SET			0X05	// 通道2设置寄存器
#define RLD_SENS		0X06	// 右腿驱动选择寄存器
#define LOFF_SENS		0X07	// 导联脱落检测选择寄存器
#define LOFF_STAT		0X08	// 导联脱落检测状态寄存器

// GPIO和其他寄存器（读写寄存器）
#define	RESP1				0X09	// 呼吸检测控制寄存器1
#define	RESP2				0X0A	// 呼吸检测控制寄存器2
#define	GPIO				0X0B	// GPIO控制寄存器			  	    													  

// 增强性能设置寄存器（读写寄存器）
#define CONFIG3			0X0C	// 配置寄存器3
#define CONFIG4			0X0D	// 配置寄存器4
#define CONFIG5			0X0E	// 配置寄存器5
#define LOFF_ISTEP	0X0F	// IDAC电流寄存器
#define LOFF_RLD		0X10	// RLD阈值寄存器
#define LOFF_AC1		0X11	// AC引线脱落检测寄存器
#define LON_CFG			0X12	// 检测前导寄存器
#define ERM_CFG			0X13	// ERM寄存器
#define EPMIX_CFG		0X14	// PACE配置寄存器
#define PACE_CFG1		0X15	// PACE配置寄存器
#define FIFO_CFG1		0X16	// FIFO配置寄存器1
#define FIFO_CFG2		0X17	// FIFO配置寄存器2

// 设备状态和属性（只读寄存器）
#define FIFO_STAT		0X18	// FIFO状态寄存器
#define OP_STAT_SYS	0X1C	// 用户操作状态寄存器
#define ID_MF				0X1D	// 设备制造信息寄存器
#define ID_PR				0X1E	// 设备部件信息寄存器

// 设备状态和属性（读写寄存器）
#define MOD_STAT1		0X19	// 设备子模块状态寄存器1
#define MOD_STAT2		0X1A	// 设备子模块状态寄存器2
#define OP_STAT_CMD 0X1B	// 用户命令状态寄存器

// 扩展功能配置
#define AC_CMP_THD0	0X1F	// AC引出比较器阈值寄存器-低字节
#define AC_CMP_THD1	0X20	// AC引出比较器阈值寄存器-低字节
#define AC_CMP_THD2	0X21	// AC引出比较器阈值寄存器-低字节
#define OFC_CH10		0X22	// 通道1偏移校准寄存器-Byte0
#define OFC_CH11		0X23	// 通道1偏移校准寄存器-Byte1
#define OFC_CH12		0X24	// 通道1偏移校准寄存器-Byte2
#define OFC_CH20		0X25	// 通道2偏移校准寄存器-Byte0
#define OFC_CH21		0X26	// 通道2偏移校准寄存器-Byte1
#define OFC_CH22		0X27	// 通道2偏移校准寄存器-Byte2
#define DEV_CONFG1	0X28	// 设备配置信息寄存器

//-----------------------------------------------------------------
// 外部函数声明
//-----------------------------------------------------------------
void GPIO_ADX922_Configuration(void);
void ADX922_PowerOnInit(void);
void ADX922_Write_Reg(u8 addr, u8 data);
u8 ADX922_Read_Reg(u8 addr);
void ADX922_Read_Data(u8 *data);
		 
#endif
