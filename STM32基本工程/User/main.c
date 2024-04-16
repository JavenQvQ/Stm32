#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"

#define Task1_ID 1//任务1的ID
void TASK1(void)
{
	uint8_t key;
	key= KEY_FIFO_Get();
	if(key==KEY_1_DOWN)
	{
		OLED_ShowString(1,1,"KEY1 DOWN");
	}
	else if(key==KEY_1_UP)
	{
		OLED_ShowString(2,1,"KEY1 UP");
	}
	else if(key==KEY_1_LONG)
	{
		OLED_ShowString(3,1,"KEY1 LONG");
	}
}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(72000);	//定时器初始化
	Key_Init();		//按键初始化
	AddTask(Task1_ID, 20, TASK1);	//添加任务1
	while (1)
	{
		
	}
}

void SysTick_Handler(void)
{
	TASK_OS();
	KEY_Scan();
}