#ifndef __MYSPI_H
#define __MYSPI_H
#include "stdint.h"

void MySPI_Software_Init(void);//软件初始化SPI
void MySPI_W_SS(uint8_t x);//写片选,1为不选中,0为选中
void MySPI_W_SCK(uint8_t x);//写时钟,1为高电平,0为低电平
void MySPI_W_MOSI(uint8_t x);//写数据,1为高电平,0为低电平
uint8_t MySPI_R_MISO(void);//读数据
void MySPI_Software_Init(void);//软件初始化SPI
uint8_t MySPI_Software_SwapByte(uint8_t Byte);//软件SPI交换数据
void MySPI_Start(void);//开始传输
void MySPI_Stop(void);//停止传输


void MySPI_Hardware_Init(void);//硬件初始化SPI
uint8_t MySPI_Hardware_SwapByte(uint8_t Byte);//硬件SPI交换数据
void MySPI_Hardware_WriteBytes(uint8_t *buf, int len);//硬件SPI写入数据
void MySPI_Hardware_ReadBytes(uint8_t *buf, int len);//硬件SPI读取数据

void PB4_ExintSet(uint8_t en);//MISO外部中断设置


void MySPI2_Harware_Init(void);//硬件SPI2初始化
uint8_t MySPI2_Hardware_SwapByte(uint8_t Byte);//硬件SPI2交换数据
void MySPI2_Hardware_ReadBytes(uint8_t *buf, int len);//硬件SPI2读取数据
void MySPI2_Hardware_WriteBytes(uint8_t *buf, int len);//硬件SPI2写入数据
void MySPI2_Hardware_SelectSS(uint8_t x);//硬件SPI2选择片选
#endif



