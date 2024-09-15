#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <math.h>
#include "OLED.h"
#include "TIM.h"
#include "KEY.h"
#include "Serial.h"
#include "wit_c_sdk.h"
#include "arm_math.h"
#include "ADXL355.h"
#include "MySPI.h"

#define Task1_ID 1//任务1的ID

float G[3];
float A[3];
float Gz;
uint16_t countx = 0; // 用于按压次数计数
int8_t GetDate_Flag = 0;

float AX1, AY1, AZ1;

ADXL355_HandleTypeDef ADXL355_t;
void ADXL355(void)
{
    ADXL355_t.ADXL355_Range = ADXL355_RANGE_2G;
    ADXL355_t.ADXL355_LowPass = ADXL355_ODR_1000;
    ADXL355_t.ADXL355_HighPass = 0x00;
    ADXL355_Init(&ADXL355_t);
}

void TASK1(void)
{

    ADXL355_ReadData();
    AZ1 = (float)i32SensorZ / ADXL355_RANGE_2G_SCALE;
    AY1= (float)i32SensorY / ADXL355_RANGE_2G_SCALE;
    AX1 = (float)i32SensorX / ADXL355_RANGE_2G_SCALE;

// 	static float Gz_Press[150];
//     static uint16_t i = 0;
//     static int8_t Press_Flag = 0;
//     static int8_t stage = 0; // 阶段标志
// 	static uint8_t currentIndex = 0; // 当前检测的位置
// 	static uint8_t Gz_Press_Start;//按压开始数组索引
// 	static uint8_t Gz_Press_End;//按压结束数组索引
	

// 	Wit901BC_GetData(G, A);//获取数据
// 	// 将角度转换为弧度
//     float A0_rad = A[0] * 0.017444f;
//     float A1_rad = A[1] * 0.017444f;

//     // 预计算三角函数
//     float sin_A0 = arm_sin_f32(A0_rad);
//     float sin_A1 = arm_sin_f32(A1_rad);
//     float cos_A0 = arm_cos_f32(A0_rad);
//     float cos_A1 = arm_cos_f32(A1_rad);

//     // 计算Gz的值
//     Gz = -G[0] * sin_A1 + G[1] * sin_A0 + G[2] * cos_A0 * cos_A1-1;

// 	// 更新Gz_Press数组
// 	Gz_Press[i] = Gz;
//     i = (i + 1) % 150;//循环数组

//    // 检测逻辑
//     int consecutiveCount = 0;//连续判断是否满足条件的计数
//     switch (stage)
//     {
//     case 0:
//         // 检测至少连续20个数字小于-0.01
//         for (int j = currentIndex; j < currentIndex + 150; j++)
//         {
//             int index = j % 150; // 确保索引在数组范围内
//             if (Gz_Press[index] < -0.01)
//             {
// 				if (consecutiveCount == 0) {
//                     Gz_Press_Start = index; // 记录开始检测的位置
//                 }
//                 consecutiveCount++;
// 			if (consecutiveCount >= 20)
//             {
//                 // 如果Gz的值大于0.1，进入阶段1
//                 if (Gz > 0.1)
//                 {
//                     stage = 1;
//                     currentIndex = index; // 更新当前检测的位置
//                 }
//                 break;
//             }
//             }
//             else
//             {
//                 consecutiveCount = 0;
//             }

//         }
//         break;
//     case 1:
//         // 检测至少连续40个数字大于0.01
//         for (int j = currentIndex; j < currentIndex + 150; j++)
//         {
//             int index = j % 150; // 确保索引在数组范围内
//             if (Gz_Press[index] > 0.01)
//             {
//                 consecutiveCount++;
//                 if (consecutiveCount >= 20)
//                 {
//                     // 如果Gz的值小于-0.01，进入阶段2
//                     if (Gz < -0.01)
//                     {
//                         stage = 2;
//                         currentIndex = index; // 更新当前检测的位置
//                     }
//                     break;
//                 }
//             }
//             else
//             {
//                 consecutiveCount = 0;
//             }
//         }
//         break;
//     case 2:
//         // 检测至少连续40个数字小于-0.01
//         for (int j = currentIndex; j < currentIndex + 150; j++)
//         {
//             int index = j % 150; // 确保索引在数组范围内
//             if (Gz_Press[index] < -0.01)
//             {
//                 consecutiveCount++;
//                 if (consecutiveCount >= 20)
//                 {
// 					if(Gz > 0)
// 					{
// 						countx++;
//                     	stage = 0;
//                    		// 清空Gz_Press数组
// 						memset(Gz_Press, 0, sizeof(Gz_Press));//清空数组
//                     	i = 0;
//                     	currentIndex = 0; // 重置当前检测的位置
//                     	break;
// 					}

//                 }
//             }
//             else
//             {
//                 consecutiveCount = 0;
//             }
//         }
//         break;
//     }
     printf("%.3f\n",AZ1);	//串口打印Gz的值
}
/** 
 * @brief  校准函数
 * @param  无
 * @retval 无
*/
void Calibrate(void)
{
   WitStartAccCali();//开始校准
}
int main(void)
{        
    uint8_t ui24Result[3];//存放读取的数据
    uint32_t ui32Result;
    uint8_t ui8temp;
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	//Key_Init();		//按键初始化
	// Wit901BC_Init();	//wit901bc初始化,最高输出频率为200Hz即0.005s,
	// Calibrate();	//校准函数
	TIM2_Init();	//定时器初始化
    ADXL355();
	Serial2_Init();
	while (1)
	{
		if(GetDate_Flag == 1)
		{
			//TASK1();
			GetDate_Flag = 0;
		}
		OLED_ShowFloat(2, 1,AZ1);	//显示Gz的值
		OLED_ShowNum(4, 1, countx, 3);	//显示按压次
        // Delay_ms(100);
        MySPI2_Hardware_SelectSS(0);//选择片选
        MySPI2_Hardware_SwapByte(0x00);
        ui8temp = MySPI2_Hardware_SwapByte(0xff);
        MySPI2_Hardware_SelectSS(1);//取消片选
        //ui8temp = (uint8_t) ADXL355_ReadRegister(ADXL355_POWER_CTL, SPI_READ_ONE_REG);
        printf("%x\n",ui8temp);
        
	}
}

// TIM2 中断处理函数
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        // 处理中断
        GetDate_Flag = 1;

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
    

}