#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "Serial.h"
#include "os_delay.h"
#include "wit_c_sdk.h"

#define Task1_ID 1//任务1的ID

float Gz,Az;//定义两个全局变量

void TASK1(void)
{
	Wit901BC_GetData(&Gz,&Az);
	OLED_ShowFloat(1, 1, Gz);
	OLED_ShowFloat(2, 1, Az);


}

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	SysTick_Config(72000);	//定时器初始化,1ms进入一次中断
	Key_Init();		//按键初始化
	Wit901BC_Init();	//wit901bc初始化

	AddTask(Task1_ID, 100, TASK1);	//添加任务1
	
	while (1)
	{

	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}

// /*传感器数据更新回调函数
//  * uiReg:寄存器地址
//  * uiRegNum:寄存器数量
//  */
// static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
// {
// 	int i;
//     for(i = 0; i < uiRegNum; i++)
//     {
//         switch(uiReg)
//         {
// //            case AX:
// //            case AY:
//             case AZ:
// 				s_cDataUpdate |= ACC_UPDATE;
//             break;
// //            case GX:
// //            case GY:
//             case GZ:
// 				s_cDataUpdate |= GYRO_UPDATE;
//             break;
// //            case HX:
// //            case HY:
//             case HZ:
// 				s_cDataUpdate |= MAG_UPDATE;
//             break;
// //            case Roll:
// //            case Pitch:
//             case Yaw:
// 				s_cDataUpdate |= ANGLE_UPDATE;
//             break;
//             default:
// 				s_cDataUpdate |= READ_UPDATE;
// 			break;
//         }
// 		uiReg++;
//     }
// }
// void Wit901BC_GetData(void)
// {
// 		if(s_cDataUpdate)
// 		{
// 			for(i = 0; i < 3; i++)
// 			{
// 				fAcc[i] = sReg[AX+i] / 32768.0f * 16.0f;
// 				fGyro[i] = sReg[GX+i] / 32768.0f * 2000.0f;
// 			}
// 			if(s_cDataUpdate & ACC_UPDATE)
// 			{
// 				OLED_ShowFloat(1, 1, fAcc[2]);
// 				s_cDataUpdate &= ~ACC_UPDATE;//清除标志位
// 			}

// 			if(s_cDataUpdate & ANGLE_UPDATE)
// 			{
// 				for(i = 0; i < 3; i++)
// 				{
// 					fAngle[i] = sReg[Roll+i] / 32768.0f * 180.0f;
// 				}
// 				OLED_ShowFloat(3, 1, fAngle[2]);
// 				s_cDataUpdate &= ~ANGLE_UPDATE;//清除标志位
// 			}
// 		}
// }
	