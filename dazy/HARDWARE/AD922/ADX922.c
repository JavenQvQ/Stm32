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
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  // ʹ��IO��ʱ��
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOG, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // ʹ��SYSCFGʱ�ӣ������ⲿ�ж�

  // ADX922_DRDY -> PC8 (����Ϊ���룬���ж�)
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;    // ����ģʽ
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;    // ��������
  GPIO_Init(GPIOC, &GPIO_InitStructure);           // ��ʼ��
  
  // ADX922_PWDN  -> PG6
  // ADX922_START -> PG7
  // ADX922_CS    -> PG8
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;   // ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // �������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // ����
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;    // ����
  GPIO_Init(GPIOG, &GPIO_InitStructure);           // ��ʼ��

  // ����PC8���ⲿ�ж�
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);

  // ����EXTI8��
  EXTI_InitStructure.EXTI_Line = EXTI_Line8;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; // �ж�ģʽ
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; // �����ش���
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  // ����NVIC
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; // PC8��ӦEXTI8��ʹ��EXTI9_5_IRQnͨ��
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; // ��ռ���ȼ�
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; // �����ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
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
  SPI3_ReadWriteByte(addr);	// �������������ͼĴ�����ַ
  delay_us(10);
  SPI3_ReadWriteByte(0x00);	// Ҫ��ȡ�ļĴ�����+1
  delay_us(10);
  SPI3_ReadWriteByte(data);	// д�������
	delay_us(10);
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
  SPI3_ReadWriteByte(addr); 			// �������������ͼĴ�����ַ
  delay_us(10);
  SPI3_ReadWriteByte(0x00); 			// Ҫ��ȡ�ļĴ�����+1
  delay_us(10);
  Rxdata = SPI3_ReadWriteByte(0xFF); 	// ��ȡ������
	delay_us(10);
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
  delay_us(1000);
  PWDN_H; // �˳�����ģʽ
  delay_us(1000);   // �ȴ��ȶ�
  PWDN_L; // ������λ����
  delay_us(10);
  PWDN_H;
  delay_us(1000); // �ȴ��ȶ������Կ�ʼʹ��ADX922
	
	START_L;
	CS_L;
	delay_us(10);
  SPI3_ReadWriteByte(SDATAC); // ����ֹͣ������ȡ��������
	delay_us(10);
	CS_H;
	
	// ��ȡоƬID
	device_id = ADX922_Read_Reg(RREG | ID);
	while(device_id != 0xF3)
	{
		device_id = ADX922_Read_Reg(RREG | ID);
		delay_us(1000);
    printf("device_id = %x\r\n����", device_id);

	}
	delay_us(10);
  ADX922_Write_Reg(WREG | CONFIG2,  	0XE0); 	// ʹ���ڲ��ο���ѹ
  delay_ms(10);                            	// �ȴ��ڲ��ο���ѹ�ȶ�
  ADX922_Write_Reg(WREG | CONFIG1,  	0X01); 	// ����ת������Ϊ250SPS
  delay_us(10);
  ADX922_Write_Reg(WREG | LOFF,     	0XF0);	// �üĴ�����������������
  delay_us(10);
  ADX922_Write_Reg(WREG | CH1SET,   	0X00); 	// ����6�����ӵ��缫
  delay_us(10);
  ADX922_Write_Reg(WREG | CH2SET,   	0X00); 	// ����6�����ӵ��缫
  delay_us(10);
  ADX922_Write_Reg(WREG | RLD_SENS, 	0x2F);// ѡ��������������
  delay_us(10);
  ADX922_Write_Reg(WREG | LOFF_SENS,	0x40);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_STAT,	0x00);
  delay_us(10);
  ADX922_Write_Reg(WREG | RESP1,    	0x02);
  delay_us(10);
  ADX922_Write_Reg(WREG | RESP2,    	0x03);
  delay_us(10);
  ADX922_Write_Reg(WREG | GPIO,     	0x0C);
  delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG3,  	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG4,  	0x03);
  delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG5,  	0x70);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_ISTEP,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_RLD,		0x0F);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_AC1,		0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | LON_CFG,		0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | ERM_CFG,		0x40);
  delay_us(10);
	ADX922_Write_Reg(WREG | EPMIX_CFG,	0x14);
  delay_us(10);
	ADX922_Write_Reg(WREG | PACE_CFG1,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG1,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT1,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT2,	0xC0);
  delay_us(10);
	ADX922_Write_Reg(WREG | OP_STAT_CMD,0x27);
  delay_us(10);
	ADX922_Write_Reg(WREG | DEV_CONFG1,	0x38);
  delay_us(10);
	
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
  delay_us(10);
  SPI3_ReadWriteByte(RDATAC);    // ��������������ȡ��������
  delay_us(10);
  CS_H;                          
  START_H;                // ����ת��
  while (DRDY == 1);      // �޸�Ϊʹ��DRDY�꣬�ȴ�DRDY�ź�����
  CS_L;
  delay_us(10);
  for (i = 0; i < 9; i++)        // ������ȡ9������
  {
    *data = SPI3_ReadWriteByte(0xFF);
    data++;
  }
  START_L;                // ֹͣת��
  SPI3_ReadWriteByte(SDATAC);    // ����ֹͣ������ȡ��������
  delay_us(10);
  CS_H;
}
//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
