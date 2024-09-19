/*woshinidie*/
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <math.h>
#include "OLED.h"
#include "TIM.h"
#include "Serial.h"
#include "arm_math.h"
#include "ADXL355.h"
#include "MySPI.h"

#define Task1_ID 1//任务1的ID

#define GZ_PRESS_SIZE 40
uint16_t countx = 0; // 用于按压次数计数
uint16_t count = 0; // 用于按压次数计数
int8_t GetDate_Flag = 0;
float totalAcceleration;
float AX1, AY1, AZ1;
float V = 0;


ADXL355_HandleTypeDef ADXL355_t;
void ADXL355(void)
{
    ADXL355_t.ADXL355_Range = ADXL355_RANGE_4G;
    ADXL355_t.ADXL355_LowPass = ADXL355_ODR_2000;
    ADXL355_t.ADXL355_HighPass = 0x00;
    ADXL355_Init(&ADXL355_t);
}


void TASK1(void)
{
    static uint8_t state = 0;
    ADXL355_ReadData();
    const float scale = 1.0f / ADXL355_RANGE_4G_SCALE;
    AZ1 = (float)i32SensorZ * scale;
    AX1 = (float)i32SensorX * scale;
    AY1 = (float)i32SensorY * scale;
    arm_sqrt_f32(AX1 * AX1 + AY1 * AY1 + AZ1 * AZ1, &totalAcceleration);
    totalAcceleration=totalAcceleration-0.98;
    switch(state)
    {
        case 0://加速度小于0的情况
            if(countx >= 30) 
            {
                if(totalAcceleration >= 0) 
                {
                    countx = 0;
                    state = 1;
                }
            }
            if(totalAcceleration <= 0) 
            {
                countx++;
                V=V+totalAcceleration;
            }
            else 
            {
                countx = 0;
                V=0;
            }
            break;
        case 1://加速度大于0的情况
            if(countx >= 30) 
            {
                if(totalAcceleration <= 0) 
                {
                    countx = 0;
                    state = 2;
                }
            }
            if(totalAcceleration >= 0) 
            {
                countx++;
                V=V+totalAcceleration;
            }
            else 
            {
                countx = 0;
                V=0;
                state = 0;
            }
            break;
        case 2://加速度小于0的情况
            if(countx >= 30) 
            {
                if(totalAcceleration >= 0) 
                {
                    countx = 0;
                    count++;
                    state = 0;
                }
            }
            if(totalAcceleration <= 0) 
            {
                countx++;
                V=V+totalAcceleration;
            }
            else 
            {
                countx = 0;
                V=0;
                state = 0;
            }
            break;
    }
    printf("%f\n",totalAcceleration);            
                
 }
/** 
 * @brief  校准函数
 * @param  无
 * @retval 无
**/
int main(void)
{        
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	//Key_Init();		//按键初始化
	TIM2_Init();	//定时器初始化
    ADXL355();
	Serial2_Init();
    ADXL355_Startup();
	while (1)
	{
		if(GetDate_Flag == 1)
		{
			TASK1();
			GetDate_Flag = 0;
		}
		OLED_ShowNum(4, 1, count, 3);	//显示按压次
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

	// switch(stage)
    // {
    //     case 0:
    //             if(totalAcceleration <= 0) 
    //             {
    //                 count++;
    //                 V=V+totalAcceleration;
    //             } 
    //             else 
    //             {
    //                 count = 0;
    //                 V=0;
    //             }
    //             if(count >= 40) 
    //             {
    //                 V = V * 0.01; // 计算速度
    //                 stage = 1;//进入1阶段
    //                 count = 0;//清零
    //             }
    //             break;
    //     case 1:
    //             V += totalAcceleration*0.01;
    //             if(V >= 0)//速度为0,到达原来点且加速度大于0.25开始准备返回
    //             {
    //                 if(totalAcceleration > 0)
    //                 {
    //                     V = 0;
    //                     stage = 2;
    //                 }
    //                 else
    //                 {
    //                     V = 0;
    //                     stage = 0;
    //                 }

    //             }
    //             break;
    //     case 2:
    //             V += totalAcceleration*0.01;
    //             if(totalAcceleration <= 0)//在回去的过程中开始减速
    //             {
    //                 stage = 3;
    //             }
    //             break;
    //     case 3:
    //             V += totalAcceleration*0.01;
                
    // }
    // //printf("%f\n",totalAcceleration);