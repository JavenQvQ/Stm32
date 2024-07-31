#include "usart.h"
#include "control.h"
#include "bsp_adc.h"
#include "pid.h"
#include "bsp_drive.h"
#include "bsp_tim.h"


float ADC_ConvertedValueLocal[3] = {0, 0, 0};
int COIL_X=0, COIL_Y=0, COIL_Z=0;//X,Y,Z��ȦPWM
int ADC_VALUE[3] = {0,0,0};

void limiting_PWM(void)
{
	int amplitude=3500; //PWM������3600 ������3500,ռ�ձ���COIL_X/3600
    
	if(COIL_X < -amplitude) 
		COIL_X = -amplitude;	
	if(COIL_X > amplitude)  
		COIL_X = amplitude;	
	
	if(COIL_Y < -amplitude) 
		COIL_Y = -amplitude;	
	if(COIL_Y > amplitude)  
		COIL_Y = amplitude;		
}



int myabs(int num)
{
	int temp;
	if(num < 0)
		temp = -num;  
	else 
		temp = num;
	return temp;
}


/**
  * @brief  ���PWM��ֵ,XIN1��PB12��XIN2��PB13��PWMX��TIM1_CH1,YIN1��PB14��YIN2��PB15��PWMY��TIM1_CH4
  * 
  * @param  moto1�����1PWM
  * @param  moto2�����2PWM
  * @retval ��
  */
void set_PWM(int coil1,int coil2)
{	
	if(coil1 > 0)			
	{
		XIN1=0;
		XIN2=1;			
	}
	else 	          
	{
		XIN1=1;
		XIN2=0;
	}
	PWMX = myabs(coil1);//PWM��ֵ
	
	if(coil2<0)	
	{
		YIN1=0;
		YIN2=1;
	}
	else 
	{
		YIN1=1;
		YIN2=0;
	}
	PWMY = myabs(coil2);
}


/**
  * @brief  ���ƺ�����PID���Ϻ���
  * @param 	��
  * @retval 0
  */

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)//���ָ����TIM�жϷ������:TIM �ж�Դ 
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //����ж�
		
		//����ADCֵ
		ADC_VALUE[0] = ADC_ConvertedValue[0];
		ADC_VALUE[1] = ADC_ConvertedValue[1];
		ADC_VALUE[2] = ADC_ConvertedValue[2];
		
		//PID����PWM
		COIL_X = x_pid_calc(ADC_VALUE[0]);
		COIL_Y = y_pid_calc(ADC_VALUE[1]);
		limiting_PWM(); //�޷�
		
		//û�и���ʱ�ر���Ȧ
		if(ADC_VALUE[2] > 3660)
		{
			COIL_X = 0;
			COIL_Y = 0;
		}
		
		//������ȦPWM
		set_PWM(COIL_X, COIL_Y);
		
	}
}


