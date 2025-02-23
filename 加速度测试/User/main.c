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
#include "esp8266.h"
#include "ADX922.h"	
#include "spi.h"
#include "led.h"

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
uint16_t Gz_Press_Index = 0;//���ڼ��ٶ����ݴ洢������
uint16_t Gz_Press_Index1 = 0;//���ڼ��ٶ����ݴ洢������
uint8_t Gz_Press_Flag = 0;//���ڼ��ٶ����ݴ洢�ı�־λ
float Gz_Press_Fre = 0;//���ڼ��ٶ����ݴ洢�ı�־λ

DHT11_Data_TypeDef DHT11_Data;//������ʪ��
uint16_t USART_RX_STA = 0;
uint8_t USART_RX_BUF[200];

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
    HMI_send_number("n10.val", (int)Gz_Press_Fre);
	HMI_send_number("n12.val", (int)X);
    X = 0;

}


void TASK3(void)
{    


        if (USART_RX_BUF[0] == 0xFD && USART_RX_BUF[1] == 0xFF && USART_RX_BUF[2] == 0xFF && USART_RX_BUF[3] == 0xFF)
        {
            memset(USART_RX_BUF, 0, 200); // ��ջ�����
            Serial_SendString("OKK");
        }
        else if (USART_RX_BUF[0] == '1') // 0x31���¶ȡ�ʪ�ȡ�����ѹ������
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
        else if (USART_RX_BUF[0] == '3') // 0x33����ѹƵ�ʺ����
        {
            HMI_send_number("n10.val", 110);
            HMI_send_number("n12.val", 5);
        }
        else if (USART_RX_BUF[0] == '4') // 0x34���ĵ硢Ѫ��������
        {
                //HMI_Wave("s0.id", 0, ecg_signal[i]*5+100);
            HMI_send_number("n6.val", 72);
            HMI_send_number("n7.val", 95);
        }
        if (USART_RX_BUF[0] == '6') // 0x36���ͼ��
        {
            HMI_Wave_Clear("s0.id", 0);
        }
        USART_RX_STA = 0;
}






//-----------------------------------------------------------------
// �ĵ����ݲɼ�
//-----------------------------------------------------------------
u32 ch1_data;		// ͨ��1������
u32 ch2_data;		// ͨ��2������
u8 flog;				// �����жϱ�־λ
u16 point_cnt;	// ������ֵ֮��Ĳɼ����������ڼ�������

#define Samples_Number  1    											// ��������
#define Block_Size      1     										// ����һ��arm_fir_f32����Ĳ��������
#define NumTaps        	129     									// �˲���ϵ������

uint32_t blockSize = Block_Size;									// ����һ��arm_fir_f32����Ĳ��������
uint32_t numBlocks = Samples_Number/Block_Size;   // ��Ҫ����arm_fir_f32�Ĵ���

float32_t Input_data1; 														// ���뻺����
float32_t Output_data1;         									// ���������
float32_t firState1[Block_Size + NumTaps - 1]; 		// ״̬���棬��СnumTaps + blockSize - 1
float32_t Input_data2; 														// ���뻺����
float32_t Output_data2;         									// ���������
float32_t firState2[Block_Size + NumTaps - 1]; 		// ״̬���棬��СnumTaps + blockSize - 1

// ��ͨ�˲���ϵ��������Ƶ��Ϊ250Hz����ֹƵ��Ϊ5Hz~40Hz ͨ��filterDesigner��ȡ
const float32_t BPF_5Hz_40Hz[NumTaps]  = {
  3.523997657e-05,0.0002562592272,0.0005757701583,0.0008397826459, 0.000908970891,
  0.0007304374012,0.0003793779761,4.222582356e-05,-6.521392788e-05,0.0001839015895,
  0.0007320778677, 0.001328663086, 0.001635892317, 0.001413777587,0.0006883906899,
  -0.0002056905651,-0.0007648666506,-0.0005919140531,0.0003351111372, 0.001569915912,
   0.002375603188, 0.002117323689,0.0006689901347,-0.001414557919,-0.003109993879,
  -0.003462586319, -0.00217742566,8.629632794e-05, 0.001947802957, 0.002011778764,
  -0.0002987752669,-0.004264956806, -0.00809297245,-0.009811084718,-0.008411717601,
  -0.004596390296,-0.0006214127061,0.0007985962438,-0.001978532877,-0.008395017125,
   -0.01568987407, -0.02018531598, -0.01929843985, -0.01321159769,-0.005181713495,
  -0.0001112028476,-0.001950757345, -0.01125541423,  -0.0243169684, -0.03460548073,
   -0.03605531529, -0.02662901953, -0.01020727865, 0.004513713531, 0.008002913557,
  -0.004921500571, -0.03125274926, -0.05950148031, -0.07363011688, -0.05986980721,
   -0.01351031102,  0.05752891302,   0.1343045086,   0.1933406889,   0.2154731899,
     0.1933406889,   0.1343045086,  0.05752891302, -0.01351031102, -0.05986980721,
   -0.07363011688, -0.05950148031, -0.03125274926,-0.004921500571, 0.008002913557,
   0.004513713531, -0.01020727865, -0.02662901953, -0.03605531529, -0.03460548073,
    -0.0243169684, -0.01125541423,-0.001950757345,-0.0001112028476,-0.005181713495,
   -0.01321159769, -0.01929843985, -0.02018531598, -0.01568987407,-0.008395017125,
  -0.001978532877,0.0007985962438,-0.0006214127061,-0.004596390296,-0.008411717601,
  -0.009811084718, -0.00809297245,-0.004264956806,-0.0002987752669, 0.002011778764,
   0.001947802957,8.629632794e-05, -0.00217742566,-0.003462586319,-0.003109993879,
  -0.001414557919,0.0006689901347, 0.002117323689, 0.002375603188, 0.001569915912,
  0.0003351111372,-0.0005919140531,-0.0007648666506,-0.0002056905651,0.0006883906899,
   0.001413777587, 0.001635892317, 0.001328663086,0.0007320778677,0.0001839015895,
  -6.521392788e-05,4.222582356e-05,0.0003793779761,0.0007304374012, 0.000908970891,
  0.0008397826459,0.0005757701583,0.0002562592272,3.523997657e-05
	};

// ��ͨ�˲���ϵ��������Ƶ��Ϊ250Hz����ֹƵ��Ϊ2Hz ͨ��filterDesigner��ȡ
const float32_t LPF_2Hz[NumTaps]  = {
  -0.0004293085367,-0.0004170549801,-0.0004080719373,-0.0004015014856,-0.0003963182389,
  -0.000391335343,-0.0003852125083,-0.0003764661378,-0.0003634814057,-0.0003445262846,
  -0.0003177672043,-0.0002812864841,-0.0002331012802,-0.0001711835939,-9.348169988e-05,
  2.057720394e-06,0.0001174666468, 0.000254732382,0.0004157739459,0.0006024184986,
   0.000816378044, 0.001059226575, 0.001332378131,  0.00163706555,  0.00197432097,
   0.002344956854, 0.002749550389, 0.003188427072, 0.003661649302, 0.004169005435,
    0.00471000094, 0.005283853505, 0.005889489781, 0.006525543984, 0.007190360688,
   0.007882000878, 0.008598247543, 0.009336617775,  0.01009437256,  0.01086853724,
    0.01165591553,  0.01245311089,  0.01325654797,  0.01406249963,  0.01486710832,
    0.01566641964,  0.01645640284,  0.01723298989,  0.01799209975,  0.01872966997,
    0.01944169216,  0.02012423798,  0.02077349275,  0.02138578519,  0.02195761539,
     0.0224856846,  0.02296692133,  0.02339850739,  0.02377789468,  0.02410283685,
    0.02437139489,  0.02458196506,  0.02473328263,  0.02482444048,  0.02485488541,
    0.02482444048,  0.02473328263,  0.02458196506,  0.02437139489,  0.02410283685,
    0.02377789468,  0.02339850739,  0.02296692133,   0.0224856846,  0.02195761539,
    0.02138578519,  0.02077349275,  0.02012423798,  0.01944169216,  0.01872966997,
    0.01799209975,  0.01723298989,  0.01645640284,  0.01566641964,  0.01486710832,
    0.01406249963,  0.01325654797,  0.01245311089,  0.01165591553,  0.01086853724,
    0.01009437256, 0.009336617775, 0.008598247543, 0.007882000878, 0.007190360688,
   0.006525543984, 0.005889489781, 0.005283853505,  0.00471000094, 0.004169005435,
   0.003661649302, 0.003188427072, 0.002749550389, 0.002344956854,  0.00197432097,
    0.00163706555, 0.001332378131, 0.001059226575, 0.000816378044,0.0006024184986,
  0.0004157739459, 0.000254732382,0.0001174666468,2.057720394e-06,-9.348169988e-05,
  -0.0001711835939,-0.0002331012802,-0.0002812864841,-0.0003177672043,-0.0003445262846,
  -0.0003634814057,-0.0003764661378,-0.0003852125083,-0.000391335343,-0.0003963182389,
  -0.0004015014856,-0.0004080719373,-0.0004170549801,-0.0004293085367
	};
    arm_fir_instance_f32 S1;
	arm_fir_instance_f32 S2;
	
	u32 p_num=0;	  	// ����ˢ�����ֵ����Сֵ
	u32 min[2]={0xFFFFFFFF,0xFFFFFFFF};
	u32 max[2]={0,0};
	u32 Peak;					// ���ֵ
	u32 BPM_LH[3];		// �����жϲ���
	float BPM;				// ����

	

//�ĵ��˲�����ʼ��
void ECG_Init(void)
{

	GPIO_LED_Configuration(); 			// LED��ʼ��
	GPIO_ADX922_Configuration();		// ADX922���ų�ʼ��
	SPI1_Init();										// SPI1��ʼ��
    ADX922_PowerOnInit();						// ADX922�ϵ��ʼ��
	// ��ʼ���ṹ��S1
	arm_fir_init_f32(&S1, NumTaps, (float32_t *)LPF_2Hz, firState1, blockSize);
	// ��ʼ���ṹ��S2
	arm_fir_init_f32(&S2, NumTaps, (float32_t *)BPF_5Hz_40Hz, firState2, blockSize);
	
	CS_L;
	Delay_us(10);
  SPI1_ReadWriteByte(RDATAC);		// ��������������ȡ��������
  Delay_us(10);
	CS_H;						
  START_H; 				// ����ת��
	CS_L;
}



int main(void)
{        

    /*ģ���???��*/
    //I2C_Bus_Init();
    //SetVolume(10);
	//SetReader(Reader_XiaoPing);
   // SetSpeed(10);
    TIM2_Init();	//��ʱ����ʼ��
    //ADXL355();
	Serial_Init();
    //ADXL355_Startup();
    //DHT11_GPIO_Config();//�¶�ʪ�ȴ�������ʼ��
    ECG_Init();	//�ĵ��˲�����ʼ��

    
	while (1)
	{
        Input_data1=(float32_t)(ch1_data^0x800000);
        // ʵ��FIR�˲�
        arm_fir_f32(&S1, &Input_data1, &Output_data1, blockSize);
        Input_data2=(float32_t)(ch2_data^0x800000);
        // ʵ��FIR�˲�
        arm_fir_f32(&S2, &Input_data2, &Output_data2, blockSize);
        // �Ƚϴ�С
        if(min[1]>Output_data2)
            min[1]=Output_data2;
        if(max[1]<Output_data2)
            max[1]=Output_data2;
        
        BPM_LH[0]=BPM_LH[1];
        BPM_LH[1]=BPM_LH[2];
        BPM_LH[2]=Output_data2;
        if((BPM_LH[0]<BPM_LH[1])&(BPM_LH[1]>max[0]-Peak/3)&(BPM_LH[2]<BPM_LH[1]))
        {
            BPM=(float)60000.0/(point_cnt*4);
            point_cnt=0;
        }
        
        // ÿ��2000�������²���һ�������Сֵ
        p_num++;;
        if(p_num>2000)
        {
            min[0]=min[1];			
            max[0]=max[1];
            min[1]=0xFFFFFFFF;
            max[1]=0;
            Peak=max[0]-min[0];
            p_num=0;
        }
        // ���ݣ����������ĵ��źš�����
        printf("B: %8d",(u32)Output_data1);
        printf("A: %8d",((u32)Output_data2));
        printf("C: %6.2f",(BPM));
        flog=0;
        // if(Gz_Press_Flag == 1)
        // {
        //     TASK2();
        //     Gz_Press_Flag = 0;
        // }


		// if(GetDate_Flag == 1)
		// {
		// 	TASK1();
		// 	GetDate_Flag = 0;
		// }
        // if (USART_RX_STA & 0x8000)
        // {
        //     TASK3();
        //     USART_RX_STA = 0; // ����״̬��־
        // }


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
                if (data != 0x0A)  // ������ǻ��з�������մ������¿�ʼ
                    USART_RX_STA = 0;
                else
                    USART_RX_STA |= 0x8000;  // �������
            }
            else
            {
                if (data == 0x0D)  // �ж��Ƿ�Ϊ�س���
                    USART_RX_STA |= 0x4000;
                else
                {
                    USART_RX_BUF[USART_RX_STA & 0x3FFF] = data;
                    USART_RX_STA++;
                    if (USART_RX_STA > (200 - 1))  // ��ֹ���������
                        USART_RX_STA = 0;
                }
            }
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}