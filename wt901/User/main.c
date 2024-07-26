#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <math.h>
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "Serial.h"
#include "os_delay.h"
#include "wit_c_sdk.h"

#define Task1_ID 1//任务1的ID

float Gz, Az; //定义两个全局变量
float G[2], Vz[2], Xz[2];
int8_t i;
int countx = 0; // 用于计数加速度值为零的次数



void TASK1(void)
{
	Wit901BC_GetData(&Gz,&Az);
	if (Gz < 0.992 || Gz > 1.04)
	{
		G[i % 2] = Gz-0.998; // 使用i%2循环存储Gz值

		if (i >= 1) // 确保我们有两个值来计算
		{
			// 第一次积分，计算速度
			Vz[1] = Vz[0] + G[0] + ((G[1] - G[0])/2); //速度积分,时间是1ms,速度是m/ms
			Vz[0] = Vz[1];
			// 第二次积分，计算位移
			Xz[1] = Xz[0] + Vz[0] + ((Vz[1] - Vz[0])/2); //位移积分,时间是1ms,位移是cm
			Xz[0] = Xz[1];
		}
		i++; // 更新i以便下次计算
		if (i >= 2) i = 0; // 重置i
		countx = 0; // 如果Gz不为零，重置countx
	}
	else
	{
		countx++; // 如果Gz为零，增加countx
		if (countx >= 10) // 如果连续25次加速度为零，假设速度为零
		{
			Vz[1] = 0;
			Vz[0] = 0;
			countx = 0; // 重置countx以便新的计数
		}
	}
}
/** 
 * @brief  校准函数
 * @param  无
 * @retval 无
*/
void Calibrate(void)
{
   WitStartAccCali();
}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Key_Init();		//按键初始化
	Wit901BC_Init();	//wit901bc初始化
	Calibrate();	//校准函数


	SysTick_Config(72000);	//定时器初始化,1ms进入一次中断
	AddTask(Task1_ID, 100, TASK1);	//添加任务1
	
	while (1)
	{
		OLED_ShowFloat(2, 1, Gz);	//显示Gz的值
		OLED_ShowFloat(1, 1, Xz[1]);	//显示Xz[1]的值
	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}
