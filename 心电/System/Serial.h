#ifndef __SERIAL_H
#define __SERIAL_H
#include <stm32f10x.h>
#include <stdio.h>
#define USART_REC_LEN  			200  				// 定义最大接收字节数 200
extern uint8_t Serial_TxPacket[];
extern uint8_t Serial_RxPacket[];
extern uint8_t wt901bc_rxA[11];
extern u8  USART_RX_BUF[USART_REC_LEN]; // 接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         				// 接收状态标记	

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

void Serial_SendPacket(void);
uint8_t Serial_GetRxFlag(void);

void Uart1Send(unsigned char *p_data, unsigned int uiSize);
void Serial2_Init();
#endif
