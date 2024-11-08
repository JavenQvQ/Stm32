/*woshinidie*/
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <math.h>
#include "bsp_i2c.h"
#include "TIM.h"
#include "Serial.h"
#include "arm_math.h"
#include "ADXL355.h"
#include "MySPI.h"

#define Task1_ID 1//任务1的ID

uint16_t countx = 0; // 用于按压次数计数
uint16_t count = 0; // 用于按压次数计数
int8_t GetDate_Flag = 0;
float totalAcceleration;
float AX1, AY1, AZ1;
float V = 0;
float X = 0;
float X1 = 0;
float X2 = 0;
float Gz_Press[512];
uint16_t Gz_Press_Index = 0;//用于加速度数据存储的索???
uint16_t Gz_Press_Index1 = 0;//用于加速度数据存储的索???
uint8_t Gz_Press_Flag = 0;//用于加速度数据存储的标志位
float Gz_Press_Fre = 0;//用于加速度数据存储的???率

ADXL355_HandleTypeDef ADXL355_t;
void ADXL355(void)
{
    ADXL355_t.ADXL355_Range = ADXL355_RANGE_4G;
    ADXL355_t.ADXL355_LowPass = ADXL355_ODR_1000;
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
        case 0://加速度小于0的情???
            if(countx >= 60) 
            {
                if(totalAcceleration >= 0) 
                {
                    countx = 0;
                    state = 1;
                    Gz_Press[Gz_Press_Index] = totalAcceleration;
                    Gz_Press_Index++;
                    break;
                }
            }
            if(totalAcceleration <= 0) 
            {
                countx++;
                Gz_Press[Gz_Press_Index] = totalAcceleration;
                Gz_Press_Index++;
            }
            else 
            {
                countx = 0;
                Gz_Press_Index = 0;
            }
            break;
        case 1://加速度大于0的情???
            if(countx >= 60) 
            {
                if(totalAcceleration <= 0) 
                {
                    countx = 0;
                    state = 2;
                    Gz_Press[Gz_Press_Index] = totalAcceleration;
                    Gz_Press_Index++;
                    break;
                }
            }
            if(totalAcceleration >= 0) 
            {
                countx++;
                Gz_Press[Gz_Press_Index] = totalAcceleration;
                Gz_Press_Index++;
            }
            else 
            {
                countx = 0;
                Gz_Press_Index = 0;
                state = 0;
            }
            break;
        case 2://加速度小于0的情???
            if(countx >= 50) 
            {
                if(totalAcceleration >= 0) 
                {
                    countx = 0;
                    count++;
                    state = 0;
                    Gz_Press[Gz_Press_Index] = totalAcceleration;
                    Gz_Press_Index1 = Gz_Press_Index;
                    Gz_Press_Index=0;
                    Gz_Press_Flag = 1;
                    break;
                }
            }
            if(totalAcceleration <= 0) 
            {
                countx++;
                Gz_Press[Gz_Press_Index] = totalAcceleration;
                Gz_Press_Index++;
            }
            else 
            {
                countx = 0;
                Gz_Press_Index = 0;
                state = 0;
            }
            break;
    }
    //printf("%f\n",totalAcceleration);                      
 }

void TASK2(void)//计算位移
{
    static uint8_t state = 0;
    static uint16_t Index = 0;

    V = 0;
    X = 0;
    X1 = 0;
    X2 = 0;
    Gz_Press_Fre = 60/((Gz_Press_Index1 +1)*0.001);
    for(uint16_t i = 0; i < Gz_Press_Index1+1; i++)
    {
        // 使用???形积分法计算速度
        if (i > 0)
        {
            V += 0.5 * (Gz_Press[i] + Gz_Press[i - 1]) * 0.001;
        }
        else
        {
            V += Gz_Press[i] * 0.001;
        }

        // 使用???形积分法计算位移
        if (i > 0)
        {
            X += 0.5 * (V + (V - Gz_Press[i] * 0.001)) * 0.001;
        }
        else
        {
            X += V * 0.001;
        }

        if (V >= 0)
        {
            X1 = X;
            X = 0;
            Index = i;
            state = 1;
            break;
        }
    }
    if(state == 1)
    {
        for (uint16_t i = Index; i < Gz_Press_Index1+1; i++)
        {
            // 使用???形积分法计算速度
            if (i > 0)
            {
                V += 0.5 * (Gz_Press[i] + Gz_Press[i - 1]) * 0.001;
            }
            else
            {
                V += Gz_Press[i] * 0.001;
            }

            // 使用???形积分法计算位移
            if (i > 0)
            {
                X += 0.5 * (V + (V - Gz_Press[i] * 0.001)) * 0.001;
            }
            else
            {
                X += V * 0.001;
            }

            if (i == Gz_Press_Index1)
            {
                V = 0;
                X2 = X;
                state = 0;
                break;
            }
        }
    }
    X=-X1*1000;
    if(X<4.5)
    {
        if(Gz_Press_Fre>120)
        {
            speech_text("按压频率过快而且按压太轻",GB2312);
        }
        else if(Gz_Press_Fre<90)
        {
            speech_text("按压频率太慢而且按压太轻",GB2312);
        }
        else
        {
            speech_text("按压太轻",GB2312);
        }
    }
    if(X>5.5)
    {
        if(Gz_Press_Fre>120)
        {
            speech_text("按压频率过快而且按压太重",GB2312);
        }
        else if(Gz_Press_Fre<90)
        {
            speech_text("按压频率太慢而且按压太重",GB2312);
        }
        else
        {
            speech_text("按压太重",GB2312);
        }
    }

    printf("X=%f\nGz_Press_Fre:%f\ncount:%d\n",X,Gz_Press_Fre,count);
    X=0;

}




/** 
 * @brief  校准函数
 * @param  ???
 * @retval ???
**/
int main(void)
{        
	/*模块初???化*/
    I2C_Bus_Init();
    SetVolume(7);
	SetReader(Reader_XiaoPing);
	TIM2_Init();	//定时器初始化
    ADXL355();
	Serial2_Init();
    ADXL355_Startup();
	while (1)
	{
        if(Gz_Press_Flag == 1)
        {
            TASK2();
            Gz_Press_Flag = 0;
        }


		if(GetDate_Flag == 1)
		{
			TASK1();
			GetDate_Flag = 0;
		}


        //  speech_text("病人生命垂危，请立刻就医",GB2312);
        //  Delay_ms(8000); 
    
	}
}

// TIM2 ??????处理函数
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        // 处理??????
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
    //                 stage = 1;//进入1阶???
    //                 count = 0;//清零
    //             }
    //             break;
    //     case 1:
    //             V += totalAcceleration*0.01;
    //             if(V >= 0)//速度???0,到达原来点且加速度大于0.25开始准备返???
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
    //             if(totalAcceleration <= 0)//在回去的过程???开始减???
    //             {
    //                 stage = 3;
    //             }
    //             break;
    //     case 3:
    //             V += totalAcceleration*0.01;
                
    // }
    // //printf("%f\n",totalAcceleration);