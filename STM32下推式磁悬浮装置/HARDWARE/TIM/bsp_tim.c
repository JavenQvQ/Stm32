#include "bsp_tim.h"


void coil_PWM_init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_11; //TIM1_CH1  TIM1_CH4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM_TimeBaseStructure.TIM_Period = arr;
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //Ĭ�ϲ���Ƶ
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�Ӳ���Ƶ
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //PWMģʽ1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_Pulse = 0; //���ô�װ�벶��ȽϼĴ���������ֵ
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //TIM����Ƚϼ��Ը�
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC4Init(TIM1, &TIM_OCInitStructure);
	
	TIM_CtrlPWMOutputs(TIM1,ENABLE);	//MOE �����ʹ��

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable); //CH1Ԥװ��ʹ��
	TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable); //CH4Ԥװ��ʹ��
	
	TIM_ARRPreloadConfig(TIM1, ENABLE); //ʹ��TIMx��ARR�ϵ�Ԥװ�ؼĴ���
	
	TIM_Cmd(TIM1, ENABLE);
}

//TIM3��ʼ��,�ж�Ƶ��Ϊ1KHz
void TIM3_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	
	//��ʼ����ʱ��1 TIM1	 
	TIM_TimeBaseStructure.TIM_Period = 4999; //�趨�������Զ���װֵ 
	TIM_TimeBaseStructure.TIM_Prescaler = 71; 	//Ԥ��Ƶ��   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  
	
	//�жϷ����ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);//���жϱ�־λ
	
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);//��������ж�
	TIM_Cmd(TIM3, ENABLE);
}

void PWM_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//ʹ��GPIOAʱ��
    GPIO_InitTypeDef GPIO_InitStructure;//����ṹ��
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;//��������ģʽΪ�����������
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0;//�������ź�Ϊ0
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//������������Ϊ50MHz
    GPIO_Init(GPIOA,&GPIO_InitStructure);//���ݲ�����ʼ��GPIOA������
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//ʹ��TIM2ʱ��
    TIM_InternalClockConfig(TIM2);//����TIM2ʱ��ԴΪ�ڲ�ʱ��Դ
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;//����ṹ��
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;//����ʱ�ӷָ�:TDTS=TIM_CKD_DIV1
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//���ü�����ģʽ
    TIM_TimeBaseInitStructure.TIM_Period=3599;//�����Զ���װ�ؼĴ������ڵ�ֵARR
    TIM_TimeBaseInitStructure.TIM_Prescaler=0;//����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��ƵֵPSC
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
    TIM_Cmd(TIM2,ENABLE);//ʹ��TIMx����
    //����TIM2��ͨ��1��PWM���
    TIM_OCInitTypeDef TIM_OCInitStructure;//����ṹ��
    TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_PWM1;//ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ1
    TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;//  ���ü���:TIM����Ƚϼ��Ը�
    TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;//�Ƚ����ʹ��
    TIM_OCInitStructure.TIM_Pulse=0;//���ô�װ�벶��ȽϼĴ���������ֵccr
    TIM_OC1Init(TIM2,&TIM_OCInitStructure);//����TIM_OCInitStruct��ָ���Ĳ�����ʼ������TIMx
}