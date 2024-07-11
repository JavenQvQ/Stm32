#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "Serial.h"
#include "os_delay.h"
#include "wit_c_sdk.h"

#define Task1_ID 1//任务1的ID
float Ax,Ay,Az;

void TASK1(void)
{


}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(72000);	//定时器初始化,1ms进入一次中断
	Key_Init();		//按键初始化
	Serial_Init();		//串口初始化
	WitInit(WIT_PROTOCOL_NORMAL, 0x50);//Wit初始化
	WitSerialWriteRegister();//Wit串口写函数注册

	/*变量定义*/



	AddTask(Task1_ID, 100, TASK1);	//添加任务1
	
	while (1)
	{

	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}
