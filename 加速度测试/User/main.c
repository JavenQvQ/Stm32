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

#define Task1_ID 1//����1��ID

uint16_t countx = 0; // ���ڰ�ѹ��������
uint16_t count = 0; // ���ڰ�ѹ��������
int8_t GetDate_Flag = 0;
float totalAcceleration;
float AX1, AY1, AZ1;
float V = 0;
float X = 0;
float X1 = 0;
float X2 = 0;
float Gz_Press[512];
uint16_t Gz_Press_Index = 0;//���ڼ��ٶ����ݴ洢����???
uint16_t Gz_Press_Index1 = 0;//���ڼ��ٶ����ݴ洢����???
uint8_t Gz_Press_Flag = 0;//���ڼ��ٶ����ݴ洢�ı�־λ
float Gz_Press_Fre = 0;//���ڼ��ٶ����ݴ洢��???��



#define USERNAME   "mqtt_stm32&k1pc8KDkdWX" //�û���
#define PASSWORD   "f3f17f624a5f4d78f8bae2dc2ed1e02388edf9da8f7387900dfb9f15bb335a63" //����
#define CLIENTID   "k1pc8KDkdWX.mqtt_stm32|securemode=2\\,signmethod=hmacsha256\\,timestamp=1725448975997|" //�豸����
#define PRODUCTID  "k1pc8KDkdWX" //��ƷID
#define DOMAINNAME PRODUCTID"iot-06z00dt2zbxttjh.mqtt.iothub.aliyuncs.com" //����
#define DEVICENAME "mqtt_stm32"

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
    delay_ms(100);
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
        case 0://���ٶ�С��0����???
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
        case 1://���ٶȴ���0����???
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
        case 2://���ٶ�С��0����???
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

void TASK2(void)//����λ��
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
        // ʹ��???�λ��ַ������ٶ�
        if (i > 0)
        {
            V += 0.5 * (Gz_Press[i] + Gz_Press[i - 1]) * 0.001;
        }
        else
        {
            V += Gz_Press[i] * 0.001;
        }

        // ʹ��???�λ��ַ�����λ��
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
            // ʹ��???�λ��ַ������ٶ�
            if (i > 0)
            {
                V += 0.5 * (Gz_Press[i] + Gz_Press[i - 1]) * 0.001;
            }
            else
            {
                V += Gz_Press[i] * 0.001;
            }

            // ʹ��???�λ��ַ�����λ��
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
    // �жϰ�ѹ���
    if (X > 5.5)
    {
        speech_text("��ѹ̫��", GB2312);
    }
    else if (X < 4.5)
    {
        speech_text("��ѹ̫��", GB2312);
    }

    // �жϰ�ѹƵ��
    if (Gz_Press_Fre > 120)
    {
        speech_text("��ѹƵ�ʹ���", GB2312);
    }
    else if (Gz_Press_Fre < 90)
    {
        speech_text("��ѹƵ��̫��", GB2312);
    }

    //printf("X=%f\nGz_Press_Fre=%f\ncount=%d\n", X, Gz_Press_Fre, count);
    HMI_send_number("n10.val", Gz_Press_Fre);
	HMI_send_number("n12.val", X);
    X = 0;

}





/** 
 * @brief  У׼����
 * @param  ???
 * @retval ???
**/
int main(void)
{        
	/*ģ���???��*/
    I2C_Bus_Init();
    SetVolume(7);
	SetReader(Reader_XiaoPing);
    SetSpeed(10);
	TIM2_Init();	//��ʱ����ʼ��
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


        //  speech_text("����������Σ�������̾�ҽ",GB2312);
        //  Delay_ms(8000); 
    
	}
}

// TIM2 ??????������
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        // ����??????
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
    //                 V = V * 0.01; // �����ٶ�
    //                 stage = 1;//����1��???
    //                 count = 0;//����
    //             }
    //             break;
    //     case 1:
    //             V += totalAcceleration*0.01;
    //             if(V >= 0)//�ٶ�???0,����ԭ�����Ҽ��ٶȴ���0.25��ʼ׼����???
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
    //             if(totalAcceleration <= 0)//�ڻ�ȥ�Ĺ���???��ʼ��???
    //             {
    //                 stage = 3;
    //             }
    //             break;
    //     case 3:
    //             V += totalAcceleration*0.01;
                
    // }
    // //printf("%f\n",totalAcceleration);