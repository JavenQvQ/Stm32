#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"

#define Task1_ID 1//任务1的ID
void TASK1(void)
{
	KEY_Function();
}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(720000);	//定时器初始化,10ms进入一次中断
	Key_Init();		//按键初始化
	AddTask(Task1_ID, 100, TASK1);	//添加任务1
	while (1)
	{
		
	}
}

void SysTick_Handler(void)
{
	TASK_OS();
	KEY_Scan();
}