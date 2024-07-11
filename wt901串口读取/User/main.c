#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "TIMEEVEN.H"
#include "KEY.h"
#include "Serial.h"
#include "os_delay.h"
#include "wit_c_sdk.h"

#define Task1_ID 1//任务1的ID
#define ACC_UPDATE		0x01
#define GYRO_UPDATE		0x02
#define ANGLE_UPDATE	0x04
#define MAG_UPDATE		0x08
#define READ_UPDATE		0x80

float Ax,Ay,Az;
static volatile char s_cDataUpdate = 0;
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum);


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
	WitSerialWriteRegister(Uart1Send);//Wit串口写函数注册
	WitRegisterCallBack(SensorDataUpdata);//Wit传感器数据更新回调函数注册
	WitDelayMsRegister(delay_ms);//Wit延时函数注册

	/*变量定义*/
	float fAcc[3], fGyro[3], fAngle[3];
	int i;


	AddTask(Task1_ID, 100, TASK1);	//添加任务1
	
	while (1)
	{
	if(s_cDataUpdate)
		{
			for(i = 0; i < 3; i++)
			{
				fAcc[i] = sReg[AX+i] / 32768.0f * 16.0f;
				fGyro[i] = sReg[GX+i] / 32768.0f * 2000.0f;
				fAngle[i] = sReg[Roll+i] / 32768.0f * 180.0f;
			}
			if(s_cDataUpdate & ACC_UPDATE)
			{
				OLED_ShowFloat(1, 1, fAcc[2]);
				s_cDataUpdate &= ~ACC_UPDATE;//清除标志位
			}
		}


	}
}

void SysTick_Handler(void)
{
	TASK_OS();
}

/*传感器数据更新回调函数
 * uiReg:寄存器地址
 * uiRegNum:寄存器数量
 */
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
{
	int i;
    for(i = 0; i < uiRegNum; i++)
    {
        switch(uiReg)
        {
//            case AX:
//            case AY:
            case AZ:
				s_cDataUpdate |= ACC_UPDATE;
            break;
//            case GX:
//            case GY:
            case GZ:
				s_cDataUpdate |= GYRO_UPDATE;
            break;
//            case HX:
//            case HY:
            case HZ:
				s_cDataUpdate |= MAG_UPDATE;
            break;
//            case Roll:
//            case Pitch:
            case Yaw:
				s_cDataUpdate |= ANGLE_UPDATE;
            break;
            default:
				s_cDataUpdate |= READ_UPDATE;
			break;
        }
		uiReg++;
    }
}
