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
#include "DHT11.h"

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

DHT11_Data_TypeDef DHT11_Data;//定义温湿度
uint16_t USART_RX_STA = 0;
uint8_t USART_RX_BUF[200];
const int ecg_signal[360] = {
    // P 波
    0, 1, 2, 3, 4, 5, 4, 3, 2, 1,
    0, -1, -2, -3, -4, -5, -4, -3, -2, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 11, 10, 9, 8, 7, 6, 5,
    4, 3, 2, 1, 0, -1, -2, -3, -4, -5,
    // QRS 复合波
    -6, -7, -8, -9, -10, -11, -12, -11, -10, -9,
    -8, -7, -6, -5, -4, -3, -2, -1, 0, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 14, 13, 12, 11, 10, 9,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    // T 波
    -2, -3, -4, -5, -6, -5, -4, -3, -2, -1,
    0, 1, 2, 3, 2, 1, 0, -1, -2, -3,
    -2, -1, 0, 1, 2, 3, 2, 1, 0, -1,
    -2, -3, -2, -1, 0, 1, 2, 1, 0, -1,
    // 心动周期结束
    0, 1, 2, 3, 4, 5, 4, 3, 2, 1,
    0, -1, -2, -3, -4, -5, -4, -3, -2, -1,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    // 重复信号以模拟持续的心电波形
        // P 波
    0, 1, 2, 3, 4, 5, 4, 3, 2, 1,
    0, -1, -2, -3, -4, -5, -4, -3, -2, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 11, 10, 9, 8, 7, 6, 5,
    4, 3, 2, 1, 0, -1, -2, -3, -4, -5,
    // QRS 复合波
    -6, -7, -8, -9, -10, -11, -12, -11, -10, -9,
    -8, -7, -6, -5, -4, -3, -2, -1, 0, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 14, 13, 12, 11, 10, 9,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    // T 波
    -2, -3, -4, -5, -6, -5, -4, -3, -2, -1,
    0, 1, 2, 3, 2, 1, 0, -1, -2, -3,
    -2, -1, 0, 1, 2, 3, 2, 1, 0, -1,
    -2, -3, -2, -1, 0, 1, 2, 1, 0, -1,
    // 心动周期结束
    0, 1, 2, 3, 4, 5, 4, 3, 2, 1,
    0, -1, -2, -3, -4, -5, -4, -3, -2, -1,
    // 重复信号以模拟持续的心电波形
};
void HMI_send_string(char* name, char* showdata)
{
    // printf("t0.txt=\"%d\"\xff\xff\xff", num);
    printf("%s=\"%s\"\xff\xff\xff", name, showdata);
}
void HMI_send_number(char* name, int num)
{
    // printf("t0.txt=\"%d\"\xff\xff\xff", num);
    printf("%s=%d\xff\xff\xff", name, num);
}
void HMI_send_float(char* name, float num)
{
    // printf("t0.txt=\"%d\"\xff\xff\xff", num);
    printf("%s=%d\xff\xff\xff", name, (int)(num * 100));
}
void HMI_Wave(char* name, int ch, int val)
{
    printf("add %s,%d,%d\xff\xff\xff", name, ch, val);
}
void HMI_Wave_Fast(char* name, int ch, int count, int* show_data)
{
    int i;
    printf("addt %s,%d,%d\xff\xff\xff", name, ch, count);
    Delay_ms(100);
    for (i = 0; i < count; i++)
        printf("%c", show_data[i]);
    printf("\xff\xff\xff");
}
void HMI_Wave_Clear(char* name, int ch)
{
    printf("cle %s,%d\xff\xff\xff", name, ch);
}




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
    // 判断按压深度
    if (X > 5.5)
    {
        speech_text("按压太重", GB2312);
    }
    else if (X < 4.5)
    {
        speech_text("按压太轻", GB2312);
    }

    // 判断按压频率
    if (Gz_Press_Fre > 120)
    {
        speech_text("按压频率过快", GB2312);
    }
    else if (Gz_Press_Fre < 90)
    {
        speech_text("按压频率太慢", GB2312);
    }

    //printf("X=%f\nGz_Press_Fre=%f\ncount=%d\n", X, Gz_Press_Fre, count);
    HMI_send_number("n10.val", (int)Gz_Press_Fre);
	HMI_send_number("n12.val", (int)X);
    X = 0;

}


void TASK3(void)
{    
    uint16_t len;
        len = USART_RX_STA & 0x3FFF; // 得到此次接收到的数据长度

        if (USART_RX_BUF[0] == 0xFD && USART_RX_BUF[1] == 0xFF && USART_RX_BUF[2] == 0xFF && USART_RX_BUF[3] == 0xFF)
        {
            memset(USART_RX_BUF, 0, 200); // 清空缓存区
            Serial_SendString("OKK");
        }
        else if (USART_RX_BUF[0] == '1') // 0x31传温度、湿度、大气压、体温
        {
            Read_DHT11(&DHT11_Data);
            
            HMI_send_number("n0.val", DHT11_Data.temp_int);
            HMI_send_number("n1.val", DHT11_Data.humi_int);
            HMI_send_number("n2.val", 1000);
            HMI_send_number("n3.val", 37);
        }
        else if (USART_RX_BUF[0] == '2') // 0x32
        {
            //HMI_send_float("x0.val", test_float);
        }
        else if (USART_RX_BUF[0] == '3') // 0x33传按压频率和深度
        {
            HMI_send_number("n10.val", 110);
            HMI_send_number("n12.val", 5);
        }
        else if (USART_RX_BUF[0] == '4') // 0x34传心电、血氧、脉搏
        {
            for (int i = 0; i < 359; i++)
            {
                //sin_data[i] = (int)((sin((i + 1) * 3.14 / 50) + 1) * 90);
                HMI_Wave("s0.id", 0, ecg_signal[i]*5+100);
            }
            HMI_send_number("n6.val", 72);
            HMI_send_number("n7.val", 95);
        }
        if (USART_RX_BUF[0] == '6') // 0x36清空图像
        {
            HMI_Wave_Clear("s0.id", 0);
        }
        USART_RX_STA = 0;
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
    SetSpeed(10);
	TIM2_Init();	//定时器初始化
    ADXL355();
	Serial2_Init();
    ADXL355_Startup();
    DHT11_GPIO_Config();//温度湿度传感器初始化
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
        if (USART_RX_STA & 0x8000)
        {
            TASK3();
            USART_RX_STA = 0; // 重置状态标志
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

void USART2_IRQHandler(void)
{
    uint8_t data;
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        data = USART_ReceiveData(USART2);

        if ((USART_RX_STA & 0x8000) == 0)
        {
            if (USART_RX_STA & 0x4000)
            {
                if (data != 0x0A)  // 如果不是换行符，则接收错误，重新开始
                    USART_RX_STA = 0;
                else
                    USART_RX_STA |= 0x8000;  // 接收完成
            }
            else
            {
                if (data == 0x0D)  // 判断是否为回车符
                    USART_RX_STA |= 0x4000;
                else
                {
                    USART_RX_BUF[USART_RX_STA & 0x3FFF] = data;
                    USART_RX_STA++;
                    if (USART_RX_STA > (200 - 1))  // 防止缓存区溢出
                        USART_RX_STA = 0;
                }
            }
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}