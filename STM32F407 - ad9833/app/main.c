
#include "board.h"
#include "bsp_uart.h"//使用PA9和PA10
#include "arm_math.h"
#include "OLED.h"//使用PB14SCL PB15SDA
#include "FFT.h"//使用arm_math.h
#include "ADC.h"//使用PB01和定时器3以及DMA2_Stream0
#include "DAC.h"//使用PA05和TIM4以及DMA1_Stream5
#include "ad9220_dcmi.h"//使用DCMI接口和DMA2_Stream1
						//GPIOA: 3个引脚（PA2, PA4, PA6）
						//GPIOB: 3个引脚（PB5, PB6, PB7）
						//GPIOC: 7个引脚（PC6, PC7, PC8, PC9, PC10, PC12）
						//GPIOD: 1个引脚（PD2）
						//GPIOE: 4个引脚（PE4, PE5, PE6, PE14, PE15）
#include "ad9833.h"//使用PB12 PB13 PB14,

#include "key.h"//使用PA0 PE2 PE3 PE4

#include <stdio.h>
#include "board.h"
#define Filter_Number 4//滤波器长度
float filter5(float *filter_buf); //中位值平均滤波法

extern uint8_t ADC1_DMA_Flag;//ADC采集完成数据标志
extern u16 ADC1_ConvertedValue[ ADC1_DMA_Size ];//ADC数据
extern uint32_t adc_convert_value[ADC_DMA_DATA_LENGTH]; //存储从 DCMI 接收来的数据
uint16_t Model = 0; // 模型选择变量
extern uint8_t KEY0_interrupt_flag ;
extern uint8_t KEY1_interrupt_flag ;
extern uint8_t KEY2_interrupt_flag ;
extern uint8_t Key_WakeUp_interrupt_flag ;
void Model_0(void); // 模型0函数声明
void Model_1(void); // 模型1函数声明
void Model_2(void); // 模型2函数声明
void Model_3(void); // 模型3函数声明
void Model_4(void); // 模型4函数声明
void Model_5(void); // 模型5函数声明

#define SIN_Value_Length 64  // 增加采样点数提高波形质量
// 标准正弦波数据 (12位DAC，0-4095范围，中心值2048)
uint16_t SIN_Value[SIN_Value_Length] = {
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346, 3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
    4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630, 3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
    2048, 1847, 1648, 1453, 1264, 1082, 910, 749, 600, 465, 345, 242, 156, 88, 39, 10,
    0, 10, 39, 88, 156, 242, 345, 465, 600, 749, 910, 1082, 1264, 1453, 1648, 1847
};

// void AD9220_DATAHandle(void)
// {
// 	static int Filter_Number_Count=0;
// 	static float AD9220_FFtInput[FFT_Size];//FFT输入数据
// 	//static uint16_t AD9220_FFt[FFT_Size];//FFT输入数据
// 	static float AD9220_FFtOutput[FFT_Size];//FFT输出数据
// 	//static float AD9220_FFtOutputReal[FFT_Size/2];//FFT输出实部
// 	static float AD9220_FFtOutputAmplitude[FFT_Size/2];//FFT输出幅值
// 	static float AD9220_FFtOutputMax[Filter_Number];//FFT输出最大值
// 	static uint32_t AD9220_FFtOutputMaxIndex[Filter_Number];//FFT输出最大值索引
// 	if(AD9220_DMA_Flag==1)
// 	{
// 		TIM_Cmd(TIM6, DISABLE);//关闭定时器
// 		AD9220_DMA_Flag=0;
//     	for(uint16_t t=0;t<FFT_Size;t++)
//     	{
//     		AD9220_FFtInput[t] = AD9220_Data[t]* 5.0f/ 4096.0f; // 归一化

// 	  	}		
// 		FFT_RealDatedeal(AD9220_FFtInput,AD9220_FFtOutput);//FFT计算
// 		//取实部,并把直流分量去掉
// 		AD9220_FFtOutput[0]=0;//直流分量去掉
// 		arm_cmplx_mag_f32(AD9220_FFtOutput,AD9220_FFtOutputAmplitude,FFT_Size/2);//求幅值
// 		//滤波
// 		FFT_GetMax(AD9220_FFtOutputAmplitude,&AD9220_FFtOutputMax[Filter_Number_Count],&AD9220_FFtOutputMaxIndex[Filter_Number_Count]);//获取最大值和最大值索引
// 		Filter_Number_Count++;
// 		if(Filter_Number_Count==Filter_Number)
// 		{
// 			Filter_Number_Count=0;
// 			//滤波
// 			//OLED_ShowFloat(2,3,AD9220_FFtOutputMax[0],12,1);//显示最大值
// 			float floatIndex[Filter_Number];
// 			for (int i = 0; i < Filter_Number; i++) {
// 				floatIndex[i] = (float)AD9220_FFtOutputMaxIndex[i];
// 			}
// 			OLED_ShowFloat(2,0,filter5(floatIndex)*fre/FFT_Size,12,1);//显示频率
// 			OLED_ShowFloat(2,16,AD9220_FFtOutputMax[0],12,1);//显示幅值
// 			OLED_Refresh();
// 		}
			
// 	}
// 		TIM_Cmd(TIM6, ENABLE);//打开定时器
// }



//ADC数据处理函数,通过DMA采集数据,采集数据后进入数据处理函数
//放在主循环中调用
//void ADC_DATAHandle(void)
//{
//	uint16_t i;
//	static int Filter_Number_Count=0;
//	static float ADC1_FFtInput[FFT_Size];//FFT输入数据
//	static float ADC1_FFtOutput[FFT_Size];//FFT输出数据
//	static float ADC1_FFtOutputReal[FFT_Size/2];//FFT输出实部
//	static float ADC1_FFtOutputAmplitude[FFT_Size/2];//FFT输出幅值
//	static float ADC1_FFtOutputMax[Filter_Number];//FFT输出最大值
//	static uint32_t ADC1_FFtOutputMaxIndex[Filter_Number];//FFT输出最大值索引
//	if(ADC1_DMA_Flag==1)
//	{	
//		ADC1_DMA_Flag=0;
//		//数据处理
//		for(i=0;i<FFT_Size;i++)
//		{
//			ADC1_FFtInput[i]=ADC1_ConvertedValue[i]*3.3/4096;//归一化
//			
//		}
//		FFT_RealDatedeal(ADC1_FFtInput,ADC1_FFtOutput);//FFT计算
//		

//		//取实部,并把直流分量去掉
//		ADC1_FFtOutput[0]=0;//直流分量去掉
//		for(i=0;i<FFT_Size/2;i++)
//		{
//			ADC1_FFtOutputReal[i]=ADC1_FFtOutput[i*2];//取实部
//		}
//		arm_cmplx_mag_f32(ADC1_FFtOutput,ADC1_FFtOutputAmplitude,FFT_Size/2);//求幅值
//		//滤波
//		FFT_GetMax(ADC1_FFtOutputAmplitude,&ADC1_FFtOutputMax[Filter_Number_Count],&ADC1_FFtOutputMaxIndex[Filter_Number_Count]);//获取最大值和最大值索引


//		Filter_Number_Count++;
//		if(Filter_Number_Count==Filter_Number)
//		{
//			Filter_Number_Count=0;
//			//滤波
//			//OLED_ShowFloat(2,3,ADC1_FFtOutputMax[0],12,1);//显示最大值
//			float floatIndex[Filter_Number];
//			for (int i = 0; i < Filter_Number; i++) {
//				floatIndex[i] = (float)ADC1_FFtOutputMaxIndex[i];
//			}
//			OLED_ShowFloat(2,0,filter5(floatIndex)*SAM_FRE/FFT_Size,12,1);//显示频率
//			OLED_ShowFloat(2,16,ADC1_FFtOutputMax[0],12,1);//显示幅值
//			OLED_Refresh();
//		}

//			
//		ADC_DMA_Trig(ADC1_DMA_Size);//重新开始采集数据
//	}
//}
 




//主函数
int main(void)
{
	board_init();
	//OLED_Init();
	
	uart1_init(115200U);
	EXTI_Key_Init();//按键初始化
	//uart2_init(115200U);
	//ADC_FFT_Init();//ADC初始化
    //AD9220_DCMIDMA_Config();//AD9220初始化
	AD9833_Init_GPIO();//AD9833初始化GPIO
	// AD9833_WaveSeting(3000,0, SIN_WAVE, 0);//设置AD9833输出正弦波，频率1000Hz
	// AD9833_AmpSet(128);//设置AD9833输出幅度 33-对应滤波器输出2V



	//DAC_Configuration(SIN_Value, SIN_Value_Length,0.79);//DAC配置，使用正弦波数据
	//Tim4_Configuration(1000, SIN_Value_Length);//配置定时器4，频率1000Hz，采样点数为SIN_Value_Length
	while(1)
	{

		//ADC_DATAHandle();//ADC数据处理函数
		//OLED_ShowNum(0,0,AD9220ReadData(100000),5,12,1);//显示AD9220数据
		//OLED_Refresh();
		//AD9220_DATAHandle();//AD9220数据处理函数
		//printf("32\n");
		Model_1(); // 调用模型1函数
		



	}
}


void DMA2_Stream1_IRQHandler(void)
{
	uint16_t counter;

	//dcmi_capture_control(DISABLE);//关闭DCMI采集
	HSYNC_VSYNC_init();
	for(counter = 0; counter < ADC_DMA_DATA_LENGTH; counter++)
	{
//		printf("%u\r\n",adc_convert_value[counter]);//串口打印数据
		printf("%u\r\n",(uint16_t)adc_convert_value[counter]);
		printf("%u\r\n",(uint16_t)(adc_convert_value[counter] >> 16));//串口打印数据
		//printf("32\n");
	}
	//dcmi_capture_control(ENABLE);//开启DCMI采集
	DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TCIF1); //清除中断标志位 TCIF:DMA 传输完成
	GPIO_ResetBits(GPIOE,GPIO_Pin_14 | GPIO_Pin_15);//将 HSYNC 和 VSYNC 拉低，开始采集
}



//{
//	u16 i=0;
//  print_host(4,filter4());
//}
/*//////////////////////////////////////////////////////////////////////////
方法五：中位值平均滤波法（又称防脉冲干扰平均滤波法）
方法： 采一组队列去掉最大值和最小值后取平均值，     （N值的选取：3-14）。 
      相当于“中位值滤波法”+“算术平均滤波法”。
      连续采样N个数据，去掉一个最大值和一个最小值，
      然后计算N-2个数据的算术平均值。    
优点： 融合了“中位值滤波法”+“算术平均滤波法”两种滤波法的优点。
       对于偶然出现的脉冲性干扰，可消除由其所引起的采样值偏差。
       对周期干扰有良好的抑制作用。
       平滑度高，适于高频振荡的系统。
缺点：对于测量速度较慢或要求数据计算速度较快的实时控制不适用，比较浪费RAM。
*/
float filter5(float *filter_buf) 
{
 int i, j;
 float filter_temp;//中间变量
 float filter_sum = 0;
 // 采样值从小到大排列（冒泡法）
 for(j = 0; j < Filter_Number - 1; j++) 
	{
   for(i = 0; i < Filter_Number - 1 - j; i++) 
		{
	 if(filter_buf[i] > filter_buf[i + 1]) 
			{
	   filter_temp = filter_buf[i];
	   filter_buf[i] = filter_buf[i + 1];
	   filter_buf[i + 1] = filter_temp;
	 }
   }
 }
 // 去除最大最小极值后求平均
 for(i = 1; i < Filter_Number - 1; i++) filter_sum += filter_buf[i];
//	printf("%d\n",filter_sum / ( Filter_Number - 2));
 return filter_sum / (Filter_Number - 2);
}



void Model_1(void)
{
    static uint32_t current_frequency = 3000;  // 当前频率，初始3kHz
    static uint32_t frequency_step = 100;      // 频率步长，初始100Hz
    static uint32_t max_frequency = 5000000;   // 最大频率5MHz (修改为5M)
    static uint32_t min_frequency = 100;       // 最小频率100Hz
    // 步长选择（0=100Hz, 1=1kHz, 2=10kHz）
    static uint8_t step_index = 0;
    static uint32_t step_values[3] = {100, 1000, 10000};
    static uint8_t initialized = 0;            // 初始化标志
    
    // 首次运行时初始化AD9833
    if (!initialized)
    {
        AD9833_WaveSeting(current_frequency, 0, SIN_WAVE, 0);
        AD9833_AmpSet(128);
        printf("AD9833 initialized. Frequency: %lu Hz (Max: 5MHz)\r\n", current_frequency);
        printf("KEY0: +freq, KEY1: -freq, KEY2: step, WakeUp: exit\r\n");
        initialized = 1;
    }
    
    // KEY0: 频率增加
    if(KEY0_interrupt_flag)
    {
        KEY0_interrupt_flag = 0; // 清除中断标志
        
        if(current_frequency + frequency_step <= max_frequency)
        {
            current_frequency += frequency_step;
            AD9833_WaveSeting(current_frequency, 0, SIN_WAVE, 0);
            printf("Frequency increased to: %lu Hz, Step: %lu Hz\r\n", 
                   current_frequency, frequency_step);
        }
        else
        {
            printf("Maximum frequency reached: %lu Hz (5MHz)\r\n", max_frequency);
        }
    }
    
    // KEY1: 频率减少
    if(KEY1_interrupt_flag)
    {
        KEY1_interrupt_flag = 0; // 清除中断标志
        
        if(current_frequency >= min_frequency + frequency_step)
        {
            current_frequency -= frequency_step;
            AD9833_WaveSeting(current_frequency, 0, SIN_WAVE, 0);
            printf("Frequency decreased to: %lu Hz, Step: %lu Hz\r\n", 
                   current_frequency, frequency_step);
        }
        else
        {
            printf("Minimum frequency reached: %lu Hz\r\n", min_frequency);
        }
    }
    
    // KEY2: 切换步长
    if(KEY2_interrupt_flag)
    {
        KEY2_interrupt_flag = 0; // 清除中断标志
        
        step_index = (step_index + 1) % 3; // 循环切换0,1,2
        frequency_step = step_values[step_index];
        
        const char* step_names[3] = {"100Hz", "1kHz", "10kHz"};
        printf("Step changed to: %s (%lu Hz)\r\n", 
               step_names[step_index], frequency_step);
    }
    
    // 退出条件：使用WakeUp按键切换模式
    if(Key_WakeUp_interrupt_flag)
    {
        Key_WakeUp_interrupt_flag = 0; // 清除中断标志
        printf("Exit Model_1\r\n");
        return; // 退出函数
    }
}





void Model_0(void)
{
	switch (Model)
	{
		case 1:
			Model_1();
			Key_WakeUp_interrupt_flag = 0; //清除中断标志
			break;
		case 2:
			
			break;
		case 3:
			
			break;
		default:
			break;
	}
}

