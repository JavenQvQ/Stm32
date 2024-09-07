#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <math.h>
#include "OLED.h"
#include "TIM.h"
#include "KEY.h"
#include "Serial.h"
#include "wit_c_sdk.h"
#include "arm_math.h"

#define Task1_ID 1//任务1的ID

float G[3];
float A[3];
float Gz;
int8_t i;
int countx = 0; // 用于按压次数计数
int8_t GetDate_Flag = 0;




void TASK1(void)
{

	const float Gz_Press[200];
	const int16_t i;
	const int8_t Press_Flag = 0;
	Wit901BC_GetData(G, A);//获取数据
	// 将角度转换为弧度
    float A0_rad = A[0] * 0.017444f;
    float A1_rad = A[1] * 0.017444f;

    // 预计算三角函数
    float sin_A0 = arm_sin_f32(A0_rad);
    float sin_A1 = arm_sin_f32(A1_rad);
    float cos_A0 = arm_cos_f32(A0_rad);
    float cos_A1 = arm_cos_f32(A1_rad);

    // 计算Gz的值
    Gz = -G[0] * sin_A1 + G[1] * sin_A0 + G[2] * cos_A0 * cos_A1-1;


	// if(Gz <=0.02)//判断是否按压
	// {
	// 	countx++;
	// 	Press_Flag = 1;
	// }
	// if (Press_Flag == 1)
	// {
	// 	Gz_Press[i] = Gz;
	// 	i++;
	// }
	// if(Press_Flag == 1 && Gz_Press[i] )
    printf("%.3f\n",Gz);	//串口打印Gz的值
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
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	//Key_Init();		//按键初始化
	Wit901BC_Init();	//wit901bc初始化,最高输出频率为200Hz即0.005s,
	Calibrate();	//校准函数
	Serial2_Init();
	TIM2_Init();	//定时器初始化


	Serial2_Init();
	while (1)
	{
		if(GetDate_Flag == 1)
		{
			TASK1();
			GetDate_Flag = 0;
		}
		OLED_ShowFloat(2, 1, Gz);	//显示Gz的值
		OLED_ShowFloat(4, 2, A[1]);	//显示Roll的值
		

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