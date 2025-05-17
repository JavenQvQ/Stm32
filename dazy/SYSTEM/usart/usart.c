#include "sys.h"
#include "usart.h"	
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
    int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x) 
{ 
    x = x; 
} 
//�ض���fputc���� - �޸�Ϊʹ��USART6
int fputc(int ch, FILE *f)
{ 	
    // �ȴ����ͻ�����Ϊ�գ����TXEλ(λ7)
    while((USART6->SR & 0x80) == 0);
    USART6->DR = (u8)ch;
    
    // ȷ���������
    while((USART6->SR & 0x40) == 0);
    return ch;
}
#endif
 
#if EN_USART1_RX   //���ʹ���˽��� - ����������Ҫ�޸�����궨������
//����6�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	

//��ʼ��IO ����6 
//bound:������
void uart_init(u32 bound){
   //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //ʹ��GPIOCʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6,ENABLE);//ʹ��USART6ʱ�� - USART6����APB2������
 
    //����6��Ӧ���Ÿ���ӳ��
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_USART6); //GPIOC6����ΪUSART6 TX
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_USART6); //GPIOC7����ΪUSART6 RX
    
    //USART6�˿�����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; //GPIOC6��GPIOC7
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
    GPIO_Init(GPIOC,&GPIO_InitStructure); //��ʼ��PC6��PC7

   //USART6 ��ʼ������
    USART_InitStructure.USART_BaudRate = bound;//����������
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
    USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
    USART_Init(USART6, &USART_InitStructure); //��ʼ������6
    
    USART_Cmd(USART6, ENABLE);  //ʹ�ܴ���6
    USART_ClearFlag(USART6, USART_FLAG_TC);
    USART_ClearFlag(USART6, USART_FLAG_RXNE);
    USART_ClearFlag(USART6, USART_FLAG_TXE);
    
#if EN_USART1_RX	
    USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);//��������ж�

    //USART6 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;//����6�ж�ͨ��
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

#endif
    
}

// �����жϴ���������
void USART6_IRQHandler(void)                	//����6�жϷ������
{
    u8 Res;
#if SYSTEM_SUPPORT_OS  //ʹ��UCOS����ϵͳ
    OSIntEnter();    
#endif
    if(USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
    {
        Res = USART_ReceiveData(USART6);	//��ȡ���յ�������
        
        if((USART_RX_STA&0x8000)==0)//����δ���
        {
            if(USART_RX_STA&0x4000)//���յ���0x0d
            {
                if(Res!=0x0a)USART_RX_STA=0;//���մ���,���¿�ʼ
                else USART_RX_STA|=0x8000;	//��������� 
            }
            else //��û�յ�0X0D
            {	
                if(Res==0x0d)USART_RX_STA|=0x4000;
                else
                {
                    USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
                    USART_RX_STA++;
                    if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
                }		 
            }
        }   		 
  } 
    
 #if SYSTEM_SUPPORT_OS  
    OSIntExit();    	//�˳��ж�
#endif
} 
#endif



