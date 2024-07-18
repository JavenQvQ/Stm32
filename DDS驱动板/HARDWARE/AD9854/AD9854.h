#ifndef __AD9854_H
#define __AD9854_H	 
#include "sys.h"


#define AD9854_RST    				PAout(6)   //AD9854��λ�˿�
#define AD9854_UDCLK  				PAout(4)   //AD9854����ʱ��
#define AD9854_WR     				PAout(5)  //AD9854дʹ�ܣ�����Ч
#define AD9854_RD     				PAout(8)   //AD9854��ʹ�ܣ�����Ч
#define AD9854_OSK   					PAout(2)   //AD9854 OSK���ƶ�
#define AD9854_FSK_BPSK_HOLD  PBout(10)   //AD9854 FSK,BPSK,HOLD���ƽţ���AD9854оƬ29��

#define AD9854_DataBus GPIOC->BSRR
#define AD9854_AdrBus  GPIOC->BSRR

#define AUTO 		1//�Զ�ɨƵ
#define MANUAL	0//FSK���ƽſ���ɨƵ

#define uint  unsigned int
#define uchar unsigned char
#define ulong unsigned long
	
//**************************���²���Ϊ��������********************************

void AD9854_IO_Init(void);								//AD9854��Ҫ�õ���IO�ڳ�ʼ��
void AD9854_WR_Byte(u32 addr,u32 dat);		//AD9854���п�д����
void Freq_convert(long Freq);	 						//�����ź�Ƶ������ת��
void Freq_double_convert(double Freq);		//�����ź�Ƶ������ת��,��ڲ���Ϊdouble����ʹ�źŵ�Ƶ�ʸ���ȷ

void AD9854_InitSingle(void);	      	  					//AD9854��Ƶģʽ��ʼ��
void AD9854_SetSine(ulong Freq,uint Shape);	 			//AD9854���Ҳ���������,Ƶ��Ϊ����
void AD9854_SetSine_double(double Freq,uint Shape);//AD9854���Ҳ���������,Ƶ�ʿ�ΪС��

void AD9854_InitFSK(void);												//AD9854��FSKģʽ��ʼ��
void AD9854_SetFSK(ulong Freq1,ulong Freq2);			//AD9854��FSK��������

void AD9854_InitRFSK(uchar autoSweepEn);					//AD9854��RFSKģʽ��ʼ�� ���Ե�Ƶģʽ��ɨƵ	 
void AD9854_SetRFSK(ulong Freq_Low,ulong Freq_High,ulong Freq_Up_Down,ulong FreRate);	//AD9854��RFSKɨƵ��������	

void AD9854_InitChirp(void);											//AD9854��Chirpģʽ��ʼ��
void AD9854_SetChirp(ulong Freq,uint Shape,ulong Freq_Up_Down,ulong FreRate);//AD9854��ChirpɨƵ��������	

void AD9854_InitBPSK(void);												//AD9854��BPSKģʽ��ʼ��			  
void AD9854_SetBPSK(ulong Freq,uint Shape,uint Phase1,uint Phase2);			//AD9854��BPSK��������

void AD9854_InitOSK(void);					 							//AD9854��OSKģʽ��ʼ��
void AD9854_SetOSK(ulong Freq,uchar RateShape);		//AD9854��OSK��������

#endif

