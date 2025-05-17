//-----------------------------------------------------------------
// ADX922����ͷ�ļ�
// ͷ�ļ���: ADX922.h
// ��    ��: ���ǵ���
// ��ʼ����: 2022-06-21
// �������: 2022-06-21
// �޸�����: 2022-06-21
// ��ǰ�汾: V1.0.0
// ��ʷ�汾:
//   - V1.0: (2022-06-21)ADX922����ͷ�ļ�
//-----------------------------------------------------------------
#ifndef __ADX922_H
#define __ADX922_H
#include "sys.h"

//-----------------------------------------------------------------
// λ����
//-----------------------------------------------------------------
// ���Ŷ����
#define PWDN_H      GPIO_SetBits(GPIOG, GPIO_Pin_6)
#define PWDN_L      GPIO_ResetBits(GPIOG, GPIO_Pin_6)
#define START_H     GPIO_SetBits(GPIOG, GPIO_Pin_7)
#define START_L     GPIO_ResetBits(GPIOG, GPIO_Pin_7)
#define CS_H        GPIO_SetBits(GPIOG, GPIO_Pin_8)
#define CS_L        GPIO_ResetBits(GPIOG, GPIO_Pin_8)
#define DRDY        GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) // �޸�ΪPC8

//-----------------------------------------------------------------
// ADX922�����
//-----------------------------------------------------------------
// ϵͳ����
#define WAKEUP			0X02	// �Ӵ���ģʽ����
#define STANDBY			0X04	// �������ģʽ
#define RESET				0X06	// ��λADX922
#define START				0X08	// ������ת��
#define STOP				0X0A	// ֹͣת��
#define LOCK				0X15	// ��SPI�ӿ�
#define UNLOCK			0X16	// ����SPI�ӿ�
#define OFFSETCAL		0X1A	// ͨ��ƫ��У׼
// ���ݶ�ȡ����
#define RDATAC			0X10	// �������������ݶ�ȡģʽ,Ĭ��ʹ�ô�ģʽ
#define SDATAC			0X11	// ֹͣ���������ݶ�ȡģʽ
#define RDATA				0X12	// ͨ�������ȡ����;֧�ֶ��ֶ��ء�
// �Ĵ�����ȡ����
#define	RREG				0X20	// ��ȡ001r rrrr 000n nnnn  ����r rrrr����ʼ�Ĵ�����ַ
#define WREG				0X40	// д��010r rrrr 000n nnnn	����r rrrr����ʼ�Ĵ�����ַ n nnnn=��Ҫд������*/
#define RFIFO				0X60	// ��ȡFIFO

// ADX922�ڲ��Ĵ�����ַ����
// �豸���ã�ֻ���Ĵ�����
#define ID					0X00	// ID���ƼĴ���

// ��������ȫ�����ã���д�Ĵ�����
#define CONFIG1			0X01	// ���üĴ���1
#define CONFIG2			0X02	// ���üĴ���2
#define LOFF				0X03	// ����������ƼĴ���

// ͨ���ض����ã���д�Ĵ�����
#define CH1SET			0X04	// ͨ��1���üĴ���
#define CH2SET			0X05	// ͨ��2���üĴ���
#define RLD_SENS		0X06	// ��������ѡ��Ĵ���
#define LOFF_SENS		0X07	// ����������ѡ��Ĵ���
#define LOFF_STAT		0X08	// ����������״̬�Ĵ���

// GPIO�������Ĵ�������д�Ĵ�����
#define	RESP1				0X09	// ���������ƼĴ���1
#define	RESP2				0X0A	// ���������ƼĴ���2
#define	GPIO				0X0B	// GPIO���ƼĴ���			  	    													  

// ��ǿ�������üĴ�������д�Ĵ�����
#define CONFIG3			0X0C	// ���üĴ���3
#define CONFIG4			0X0D	// ���üĴ���4
#define CONFIG5			0X0E	// ���üĴ���5
#define LOFF_ISTEP	0X0F	// IDAC�����Ĵ���
#define LOFF_RLD		0X10	// RLD��ֵ�Ĵ���
#define LOFF_AC1		0X11	// AC����������Ĵ���
#define LON_CFG			0X12	// ���ǰ���Ĵ���
#define ERM_CFG			0X13	// ERM�Ĵ���
#define EPMIX_CFG		0X14	// PACE���üĴ���
#define PACE_CFG1		0X15	// PACE���üĴ���
#define FIFO_CFG1		0X16	// FIFO���üĴ���1
#define FIFO_CFG2		0X17	// FIFO���üĴ���2

// �豸״̬�����ԣ�ֻ���Ĵ�����
#define FIFO_STAT		0X18	// FIFO״̬�Ĵ���
#define OP_STAT_SYS	0X1C	// �û�����״̬�Ĵ���
#define ID_MF				0X1D	// �豸������Ϣ�Ĵ���
#define ID_PR				0X1E	// �豸������Ϣ�Ĵ���

// �豸״̬�����ԣ���д�Ĵ�����
#define MOD_STAT1		0X19	// �豸��ģ��״̬�Ĵ���1
#define MOD_STAT2		0X1A	// �豸��ģ��״̬�Ĵ���2
#define OP_STAT_CMD 0X1B	// �û�����״̬�Ĵ���

// ��չ��������
#define AC_CMP_THD0	0X1F	// AC�����Ƚ�����ֵ�Ĵ���-���ֽ�
#define AC_CMP_THD1	0X20	// AC�����Ƚ�����ֵ�Ĵ���-���ֽ�
#define AC_CMP_THD2	0X21	// AC�����Ƚ�����ֵ�Ĵ���-���ֽ�
#define OFC_CH10		0X22	// ͨ��1ƫ��У׼�Ĵ���-Byte0
#define OFC_CH11		0X23	// ͨ��1ƫ��У׼�Ĵ���-Byte1
#define OFC_CH12		0X24	// ͨ��1ƫ��У׼�Ĵ���-Byte2
#define OFC_CH20		0X25	// ͨ��2ƫ��У׼�Ĵ���-Byte0
#define OFC_CH21		0X26	// ͨ��2ƫ��У׼�Ĵ���-Byte1
#define OFC_CH22		0X27	// ͨ��2ƫ��У׼�Ĵ���-Byte2
#define DEV_CONFG1	0X28	// �豸������Ϣ�Ĵ���

//-----------------------------------------------------------------
// �ⲿ��������
//-----------------------------------------------------------------
void GPIO_ADX922_Configuration(void);
void ADX922_PowerOnInit(void);
void ADX922_Write_Reg(u8 addr, u8 data);
u8 ADX922_Read_Reg(u8 addr);
void ADX922_Read_Data(u8 *data);
		 
#endif
