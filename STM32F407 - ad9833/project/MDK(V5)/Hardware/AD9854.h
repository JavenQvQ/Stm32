
/*--------------------------------------------------------------
头文件名：AD9854.h				  
文件描述：数据处理头文件
---------------------------------------------------------------*/
#ifndef _AD9854_H_
#define _AD9854_H_
 
 
#include "main.h"
 
//----------------------------------------------------------------
// AD9854_I/O控制线 - 根据实际引脚连接修改
//-----------------------------------------------------------------
// 假设连接如下，请根据实际硬件连接修改
#define AD9854_RD_GPIO      GPIOG
#define AD9854_RD_PIN       GPIO_Pin_2
#define AD9854_WR_GPIO      GPIOG
#define AD9854_WR_PIN       GPIO_Pin_3
#define AD9854_UD_GPIO      GPIOG
#define AD9854_UD_PIN       GPIO_Pin_4

#define AD9854_RT_GPIO      GPIOG
#define AD9854_RT_PIN       GPIO_Pin_8

#define AD9854_SP_GPIO      GPIOC
#define AD9854_SP_PIN       GPIO_Pin_12

#define AD9854_A2_GPIO      GPIOG
#define AD9854_A2_PIN       GPIO_Pin_5
#define AD9854_A1_GPIO      GPIOG
#define AD9854_A1_PIN       GPIO_Pin_6
#define AD9854_A0_GPIO      GPIOG
#define AD9854_A0_PIN       GPIO_Pin_7

#define AD9854_RD_Set       GPIO_SetBits(AD9854_RD_GPIO, AD9854_RD_PIN)         
#define AD9854_RD_Clr       GPIO_ResetBits(AD9854_RD_GPIO, AD9854_RD_PIN)

#define AD9854_WR_Set       GPIO_SetBits(AD9854_WR_GPIO, AD9854_WR_PIN)         
#define AD9854_WR_Clr       GPIO_ResetBits(AD9854_WR_GPIO, AD9854_WR_PIN)

#define AD9854_UDCLK_Set    GPIO_SetBits(AD9854_UD_GPIO, AD9854_UD_PIN)           
#define AD9854_UDCLK_Clr    GPIO_ResetBits(AD9854_UD_GPIO, AD9854_UD_PIN)

#define AD9854_RST_Set      GPIO_SetBits(AD9854_RT_GPIO, AD9854_RT_PIN)				   //复位控制
#define AD9854_RST_Clr      GPIO_ResetBits(AD9854_RT_GPIO, AD9854_RT_PIN)

#define AD9854_SP_Set       GPIO_SetBits(AD9854_SP_GPIO, AD9854_SP_PIN)         
#define AD9854_SP_Clr       GPIO_ResetBits(AD9854_SP_GPIO, AD9854_SP_PIN)

#define AD9854_OSK_Set         
#define AD9854_OSK_Clr 

#define AD9854_FDATA_Set         //fsk/bpsk/hold  
#define AD9854_FDATA_Clr 

// I/O驱动  使用别名
#define SPI_IO_RST_Set      GPIO_SetBits(AD9854_A2_GPIO, AD9854_A2_PIN)
#define SPI_IO_RST_Clr      GPIO_ResetBits(AD9854_A2_GPIO, AD9854_A2_PIN)
#define SPI_SDO_Set         GPIO_SetBits(AD9854_A1_GPIO, AD9854_A1_PIN)
#define SPI_SDO_Clr         GPIO_ResetBits(AD9854_A1_GPIO, AD9854_A1_PIN)
#define SPI_SDI_Set         GPIO_SetBits(AD9854_A0_GPIO, AD9854_A0_PIN)
#define SPI_SDI_Clr         GPIO_ResetBits(AD9854_A0_GPIO, AD9854_A0_PIN)
 
 // AD9854寄存器地址
#define PHASE1	0x00	  //phase adjust register #1
#define PHASE2  0x01		//phase adjust register #2
#define FREQ1   0x02		//frequency tuning word 1
#define FREQ2   0x03		//frequency tuning word 2
#define DELFQ   0x04		//delta frequency word
#define UPDCK   0x05		//update clock
#define RAMPF   0x06		//ramp rate clock
#define CONTR   0x07		//control register
#define SHAPEI  0x08		//output shape key I mult
#define SHAPEQ  0x09		//output shape key Q mult 
#define RAMPO   0x0A		//output shape key ramp rate
#define CDAC    0x0B		//QDAC
 
//**************************以下部分为函数定义********************************
extern void GPIO_AD9854_Configuration(void);						// AD9854_IO口初始化
static void AD9854_WR_Byte(uint8_t Adata);	  
extern void AD9854_Init(void);						  
static void Freq_convert(long Freq);	         	  
extern void AD9854_SetSine(uint32_t Freq,uint16_t Shape);	  
static void Freq_double_convert(double Freq);		  
extern void AD9854_SetSine_double(double Freq,uint16_t Shape);
extern void AD9854_InitFSK(void);				
extern void AD9854_SetFSK(uint32_t Freq1,uint32_t Freq2);					  
extern void AD9854_InitBPSK(void);					  
extern void AD9854_SetBPSK(uint16_t Phase1,uint16_t Phase2);					
extern void AD9854_InitOSK(void);					 
extern void AD9854_SetOSK(uint8_t RateShape);					  
extern void AD9854_InitAM(void);					 
extern void AD9854_SetAM(uint16_t Shape);					
extern void AD9854_InitRFSK(void);					 
extern void AD9854_SetRFSK(uint32_t Freq_Low,uint32_t Freq_High,uint32_t Freq_Up_Down,uint32_t FreRate);				
 
#endif