//-----------------------------------------------------------------
// ��������:
//     ADX922��������
// ��    ��: ���ǵ���
// ��ʼ����: 2022-06-21
// �������: 2022-06-21
// ��ǰ�汾: V1.0.0
// ��ʷ�汾:
// - V1.0.0: (2022-06-21) ADX922����
// ���Թ���: ����STM32���Ŀ����塢LZE_ST_LINK2
// ˵    ��:
//
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// ͷ�ļ�����
//-----------------------------------------------------------------
#include "ADX922.h"	
#include "Delay.h"
#include "Serial.h"
#include "spi.h"	


//-----------------------------------------------------------------
// void GPIO_ADX922_Configuration(void)
//-----------------------------------------------------------------
//
// ��������: ADX922���ų�ʼ��
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void GPIO_ADX922_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // ʹ��IO��ʱ��
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// ADX922_DRDY -> PA3
//	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;					// ����ADX922_DRDY
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		// ����
//  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;			// ��������
//  GPIO_Init(GPIOA, &GPIO_InitStructure);							// ��ʼ��
  
	// ADX922_PWDN  -> PA0
	// ADX922_START -> PA1
  // ADX922_CS  	 -> PA2
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		// ����
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;		// �������
  GPIO_Init(GPIOA, &GPIO_InitStructure);							// ��ʼ��
}

//-----------------------------------------------------------------
// void ADX922_Write_Reg(u8 com, u8 data)
//-----------------------------------------------------------------
//
// ��������: ��ADX922���ڲ��Ĵ�������д����
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void ADX922_Write_Reg(u8 addr, u8 data)
{
	CS_L;				// Ƭѡ����
  SPI1_ReadWriteByte(addr);	// �������������ͼĴ�����ַ
 Delay_us(10);
  SPI1_ReadWriteByte(0x00);	// Ҫ��ȡ�ļĴ�����+1
 Delay_us(10);
  SPI1_ReadWriteByte(data);	// д�������
	Delay_us(10);
	CS_H;				// Ƭѡ�ø�
}

//-----------------------------------------------------------------
// u8 ADX922_Read_Reg(u8 addr)
//-----------------------------------------------------------------
//
// ��������: ��ADX922���ڲ��Ĵ������ж�����
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
u8 ADX922_Read_Reg(u8 addr)
{
  u8 Rxdata;
	CS_L;					// Ƭѡ����
  SPI1_ReadWriteByte(addr); 			// �������������ͼĴ�����ַ
 Delay_us(10);
  SPI1_ReadWriteByte(0x00); 			// Ҫ��ȡ�ļĴ�����+1
 Delay_us(10);
  Rxdata = SPI1_ReadWriteByte(0xFF); 	// ��ȡ������
	Delay_us(10);
	CS_H;					// Ƭѡ�ø�
  return Rxdata;
}

//-----------------------------------------------------------------
// void ADX922_PowerOnInit(void)
//-----------------------------------------------------------------
//
// ��������: ADX922�ϵ縴λ
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void ADX922_PowerOnInit(void)
{
	u8 device_id;

  START_H;
  CS_H;
  PWDN_L; // �������ģʽ
  Delay_ms(20);
  PWDN_H; // �˳�����ģʽ
  Delay_ms(20);   // �ȴ��ȶ�
  PWDN_L; // ������λ����
 Delay_us(10);
  PWDN_H;
  Delay_ms(20); // �ȴ��ȶ������Կ�ʼʹ��ADX922
	
	START_L;
	CS_L;
	Delay_us(10);
  SPI1_ReadWriteByte(SDATAC); // ����ֹͣ������ȡ��������
	Delay_us(10);
	CS_H;
	
	// ��ȡоƬID
	device_id = ADX922_Read_Reg(RREG | ID);
	while(device_id != 0xF3)
	{
		printf("ERROR ID:%02x\r\n",device_id);
		device_id = ADX922_Read_Reg(RREG | ID);
		Delay_ms(20);
	}
	
	Delay_us(10);
  ADX922_Write_Reg(WREG | CONFIG2,  	0XE0); 	// ʹ���ڲ��ο���ѹ
  Delay_ms(10);                            	// �ȴ��ڲ��ο���ѹ�ȶ�
  ADX922_Write_Reg(WREG | CONFIG1,  	0X01); 	// ����ת������Ϊ250SPS
 Delay_us(10);
  ADX922_Write_Reg(WREG | LOFF,     	0XF0);	// �üĴ�����������������
 Delay_us(10);
  ADX922_Write_Reg(WREG | CH1SET,   	0X00); 	// ����6�����ӵ��缫
 Delay_us(10);
  ADX922_Write_Reg(WREG | CH2SET,   	0X00); 	// ����6�����ӵ��缫
 Delay_us(10);
  ADX922_Write_Reg(WREG | RLD_SENS, 	0x2F);
 Delay_us(10);
  ADX922_Write_Reg(WREG | LOFF_SENS,	0x40);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_STAT,	0x00);
 Delay_us(10);
  ADX922_Write_Reg(WREG | RESP1,    	0x02);
 Delay_us(10);
  ADX922_Write_Reg(WREG | RESP2,    	0x03);
 Delay_us(10);
  ADX922_Write_Reg(WREG | GPIO,     	0x0C);
 Delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG3,  	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG4,  	0x03);
 Delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG5,  	0x70);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_ISTEP,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_RLD,		0x0F);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_AC1,		0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LON_CFG,		0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | ERM_CFG,		0x40);
 Delay_us(10);
	ADX922_Write_Reg(WREG | EPMIX_CFG,	0x14);
 Delay_us(10);
	ADX922_Write_Reg(WREG | PACE_CFG1,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG1,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT1,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT2,	0xC0);
 Delay_us(10);
	ADX922_Write_Reg(WREG | OP_STAT_CMD,0x27);
 Delay_us(10);
	ADX922_Write_Reg(WREG | DEV_CONFG1,	0x38);
 Delay_us(10);
	
}

//-----------------------------------------------------------------
// u8 ADX922_Read_Data(u8 addr)
//-----------------------------------------------------------------
//
// ��������: ��ȡADX922������
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void ADX922_Read_Data(u8 *data)
{
  u8 i;
	CS_L;
	Delay_us(10);
  SPI1_ReadWriteByte(RDATAC);		// ��������������ȡ��������
 Delay_us(10);
	CS_H;						
  START_H; 				// ����ת��
  while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3) == 1);	// �ȴ�DRDY�ź�����
  CS_L;
	Delay_us(10);
  for (i = 0; i < 9; i++)		// ������ȡ9������
  {
    *data = SPI1_ReadWriteByte(0xFF);
    data++;
  }
  START_L;				// ֹͣת��
  SPI1_ReadWriteByte(SDATAC);		// ����ֹͣ������ȡ��������
	Delay_us(10);
	CS_H;
}
//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
