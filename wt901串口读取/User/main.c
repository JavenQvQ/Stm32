#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "Serial.h"


#define Task1_ID 1//任务1的ID
void TASK1(void)
{
	Wt901bc_Read();		//读取WT901BC加速度数据
}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(72000);	//定时器初始化,1ms进入一次中断
	Key_Init();		//按键初始化
	Serial_Init();		//串口初始化

	/*变量定义*/
    float Ax,Ay,Az;


	AddTask(Task1_ID, 100, TASK1);	//添加任务1
	
	while (1)
	{
		Wt901bc_AConvert(&Ax,&Ay,&Az);	//转换WT901BC加速度数据
		OLED_ShowFloat(1, 1, Az);

		
	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}