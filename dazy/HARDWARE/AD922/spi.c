
//-----------------------------------------------------------------
// ͷ�ļ�����
//-----------------------------------------------------------------
#include "spi.h"	

SPI_InitTypeDef  SPI_InitStructure;
SPI_InitTypeDef  SPI_InitStructure;

//-----------------------------------------------------------------
// void SPI3_Init(void)
//-----------------------------------------------------------------
//
// ��������: SPI3��ʼ��
// ��ڲ���: ��
// �� �� ֵ: ��
// ע������: ��
//
//-----------------------------------------------------------------
void SPI3_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
  
    // ʹ��SPI3��GPIOCʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
 
    // PC10->ADX922_SCLK, PC11->ADX922_DOUT, PC12->ADX922_DIN
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;      // ���ù��� 
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    // �������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // ����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      // ����
    GPIO_Init(GPIOC, &GPIO_InitStructure);            // ��ʼ��

    // ���Ÿ���ӳ��
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3); // SCK
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3); // MISO
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3); // MOSI

    // ���ų�ʼ��ƽ����
    GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // ����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                       // ����SPI����ģʽ:����Ϊ��SPI
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                   // ����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                          // ѡ���˴���ʱ�ӵ���̬:ʱ������
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // ���ݲ����ڵڶ���ʱ����
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                           // NSS�ź����������
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; // ���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ32
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                  // ָ�����ݴ����MSBλ��ʼ
    SPI_InitStructure.SPI_CRCPolynomial = 7;                            // CRCֵ����Ķ���ʽ
    SPI_Init(SPI3, &SPI_InitStructure);                                 // ����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���
 
    SPI_Cmd(SPI3, ENABLE); // ʹ��SPI3����
}   


//-----------------------------------------------------------------
// u8 SPI3_ReadWriteByte(u8 TxData)
//-----------------------------------------------------------------
//
// ��������: SPI3 ��дһ���ֽ�
// ��ڲ���: Ҫд����ֽ�
// �� �� ֵ: ��ȡ�����ֽ�
// ע������: ��
//
//-----------------------------------------------------------------
u8 SPI3_ReadWriteByte(u8 TxData)
{		
    u16 retry=0;	
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) //���ָ����SPI��־λ�������:���ͻ���ձ�־λ
        {
        retry++;
        if(retry>20000)return 0;
        }			  
    SPI_I2S_SendData(SPI3, TxData); //ͨ������SPIx����һ������		
    retry=0;	
        
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET)//���ָ����SPI��־λ�������:���ܻ���ǿձ�־λ
        {
        retry++;
        if(retry>20000)return 0;
        }	  	
    return SPI_I2S_ReceiveData(SPI3); //����ͨ��SPIx������յ�����				
}


