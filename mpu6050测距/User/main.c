#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "MPU6050.h"

#define Task1_ID 1//任务1的ID
#define Task2_ID 2//任务2的ID

/*变量定义*/
int16_t  z;//定义MPU6050的加速度
float z_g;//定义MPU6050的加速度
float totalDisplacement = 0; // 总位移

void TASK1(void)
{
	KEY_Function();
}
void TASK2(void)
{
	KEY_Scan();
}
void Task3(void)
{
	float z_real;
	MPU6050_GetZ(&z,&z_g);	//获取MPU6050的Z轴加速度
	z_real = z_g-0.95;	//减去0.95g的重力加速度
	


}


int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(72000);	//定时器初始化,1ms进入一次中断
	Key_Init();		//按键初始化
	MPU6050_Init();	//MPU6050初始化



	/*添加任务*/
	AddTask(Task1_ID, 100, TASK1);	//添加任务1,100ms判断一次按键状态
	AddTask(Task2_ID, 10, TASK2);	//添加任务2,10ms扫描一次按键
	AddTask(3, 10, Task3);	        //添加任务3,10ms获取一次MPU6050的Z轴加速度
	while (1)
	{
		
		OLED_ShowSignedNum(1, 1, z, 5);	//OLED显示Z轴加速度
		OLED_ShowFloat(2, 1, z_g);	//OLED显示Z轴加速度
	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}