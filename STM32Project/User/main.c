#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "Serial.h"


#define Task1_ID 1//任务1的ID
void TASK1(void)
{
	KEY_Scan();
}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(72000);	//定时器初始化,1ms进入一次中断
	Key_Init();		//按键初始化
	Serial_Init();		//串口初始化
	AddTask(Task1_ID, 10, TASK1);	//添加任务1
	while (1)
	{
		KEY_Function();		//按键功能函数

		if (Serial_GetRxFlag()==1)
		{
			OLED_ShowHexNum(1,1,Serial_RxPacket[0],8);
		}
		
	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}