
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

#define SIN_Value_Length 128  // 增加采样点数提高波形质量
// 标准正弦波数据 (12位DAC，0-4095范围，中心值2048)
uint16_t SIN_Value[SIN_Value_Length];


#define PI_PRECISE 3.141592653589793

void generate_sin_table(float amp)
{
    // 限制幅值范围在0-3.3V之间
    if(amp > 3.3f) amp = 3.3f;
    if(amp < 0.0f) amp = 0.0f;
    
    // 使用双精度常量进行计算，提高精度
    const double dac_scale = 4095.0 / 3.3;
    const double angle_step = 2.0 * PI_PRECISE / SIN_Value_Length;
    const double amp_double = (double)amp;
    
    // 设置中心偏置电压，避免信号接近0V
    const double center_voltage = 1.65; // 中心电压1.65V (3.3V/2)
    const double peak_voltage = amp_double / 2.0; // 峰值电压
    
    for(int i = 0; i < SIN_Value_Length; i++)
    {
        // 使用双精度角度计算
        double angle = angle_step * i;
        
        // 使用双精度sin函数计算正弦值，范围 -1 到 +1
        double sin_val = sin(angle);
        
        // 中心偏置的正弦波：center ± peak
        // 电压范围：(1.65 - amp/2) 到 (1.65 + amp/2)
        double voltage = center_voltage + sin_val * peak_voltage;
        
        // 确保电压在有效范围内
        if(voltage > 3.3) voltage = 3.3;
        if(voltage < 0.0) voltage = 0.0;
        
        // 转换为DAC数字值，使用双精度计算后四舍五入
        double dac_double = voltage * dac_scale + 0.5;
        uint16_t dac_value = (uint16_t)dac_double;
        
        // 确保值在有效范围内 (0-4095)
        if(dac_value > 4095) dac_value = 4095;
        
        SIN_Value[i] = dac_value;
    }
}



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
 

uint16_t model = 0; // 模型选择变量	


//主函数
int main(void)
{
	board_init();
	OLED_Init();
	uart1_init(115200U);
	EXTI_Key_Init();//按键初始化
	OLED_Init(); // OLED初始化
	OLED_Clear(); // 清屏
	//uart2_init(115200U);
	//ADC_FFT_Init();//ADC初始化
    //AD9220_DCMIDMA_Config();//AD9220初始化
	AD9833_Init_GPIO();//AD9833初始化GPIO
	// AD9833_WaveSeting(3000,0, SIN_WAVE, 0);//设置AD9833输出正弦波，频率1000Hz
	// AD9833_AmpSet(128);//设置AD9833输出幅度 33-对应滤波器输出2V


	// generate_sin_table(0.2f); // 生成正弦波表，幅值为1.0
	// DAC_Configuration(SIN_Value, SIN_Value_Length);//DAC配置，使用正弦波数据
	// Tim4_Configuration(1000, SIN_Value_Length);//配置定时器4，频率1000Hz，采样点数为SIN_Value_Length
	
	while(1)
	{

		//ADC_DATAHandle();//ADC数据处理函数
		//OLED_ShowNum(0,0,AD9220ReadData(100000),5,12,1);//显示AD9220数据
		//OLED_Refresh();
		//AD9220_DATAHandle();//AD9220数据处理函数
		//printf("32\n");
		//Model_3(); // 调用模型1函数
        
		OLED_Refresh(); // 刷新OLED显示
		switch (model)
		{
			case 0:
				OLED_Clear(); // 清屏
				OLED_ShowString(16, 0, "Model: 0", 16, 1); // 显示当前模型
				OLED_Refresh(); // 刷新OLED显示
				Model_0(); // 调用模型0函数
				break;
			case 1:
				Model_1(); // 调用模型1函数
				break;
			case 2:
				Model_2(); // 调用模型2函数
				break;
			case 3:
				Model_3(); // 调用模型3函数
				break;
			default:
				break;
		}

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

void Model_0(void)
{
	while(1)
	{
		delay_ms(100); // 延时100毫秒
		if(Key_WakeUp_interrupt_flag)
		{
			Key_WakeUp_interrupt_flag = 0; // 清除中断标志
			printf("Exit Model_0\r\n");
			model = 1; // 切换到模型1
			return; // 退出函数
		}
	}
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
        AD9833_AmpSet(150);
        printf("AD9833 initialized. Frequency: %lu Hz (Max: 5MHz)\r\n", current_frequency);
        printf("KEY2: +freq, KEY0: -freq, KEY1: step, WakeUp: exit\r\n");  // 修正提示信息
        initialized = 1;
    }
	OLED_Clear(); // 清屏
    
    OLED_ShowString(0, 16, "Model: 1", 16, 1); // 显示当前模型
    
    while(1)
    {
        // KEY2: 频率增加
        if(KEY2_interrupt_flag)
        {
            KEY2_interrupt_flag = 0; // 清除中断标志
            
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
        
        // KEY0: 频率减少
        if(KEY0_interrupt_flag)
        {
            KEY0_interrupt_flag = 0; // 清除中断标志
            
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
        
        // KEY1: 切换步长
        if(KEY1_interrupt_flag)
        {
            KEY1_interrupt_flag = 0; // 清除中断标志
            
            step_index = (step_index + 1) % 3; // 循环切换0,1,2
            frequency_step = step_values[step_index];
            
            const char* step_names[3] = {"100Hz", "1kHz", "10kHz"};
            printf("Step changed to: %s (%lu Hz)\r\n", 
                   step_names[step_index], frequency_step);
        }
        
        // 显示当前状态
        OLED_ShowString(0, 32, "Freq: ", 16, 1); // 显示频率
        OLED_ShowNum(0, 48, current_frequency, 7, 16, 1); // 显示当前频率
        OLED_ShowString(0, 64, "Step: ", 16, 1); // 显示步长
        OLED_ShowNum(72, 64, frequency_step, 5, 16, 1); // 显示当前步长
        OLED_Refresh(); // 刷新OLED显示
        
        // 退出条件：使用WakeUp按键切换模式
        if(Key_WakeUp_interrupt_flag)
        {
            Key_WakeUp_interrupt_flag = 0; // 清除中断标志
            printf("Exit Model_1\r\n");
            model = 2; // 切换到模型2
            return; // 退出函数
        }
    }
}

void Model_2(void)
{
	generate_sin_table(0.79f); // 生成正弦波表，幅值为0.79V
	DAC_Configuration(SIN_Value, SIN_Value_Length);//DAC配置，使用正弦波数据
	Tim4_Configuration(1000, SIN_Value_Length);//配置定时器4，频率1000Hz，采样点数为SIN_Value_Length
	OLED_Clear(); // 清屏
	OLED_ShowString(0, 0, "Model: 2", 16, 1); // 显示当前模型
	OLED_ShowString(0, 16, "Freq: 1000Hz", 16, 1); // 显示频率
	OLED_ShowString(0, 32, "Amp: 2.0V", 16, 1); // 显示幅度
	OLED_Refresh(); // 刷新OLED显示
	while(1)
	{
		delay_ms(100); // 延时100ms，避免过快刷新
		if(Key_WakeUp_interrupt_flag)
		{
			Key_WakeUp_interrupt_flag = 0; // 清除中断标志
			printf("Exit Model_2\r\n");
			model = 3; // 切换到模型3
			return; // 退出函数
		}
	}
	
}

// 计算H(s)滤波器在给定频率下的幅值补偿
// H(s) = 5/(10^-8*s^2 + 3*10^-4*s + 1)
float calculate_amplitude_compensation(uint32_t frequency)
{
    // 使用双精度提高计算精度
    double omega = 2.0 * PI_PRECISE * (double)frequency; // 角频率 ω = 2πf
    double omega2 = omega * omega;
    
    // H(jω)的模长计算，使用双精度
    // |H(jω)| = 5 / sqrt((1 - 10^-8*ω^2)^2 + (3*10^-4*ω)^2)
    double real_part = 1.0 - 1e-8 * omega2;
    double imag_part = 3e-4 * omega;
    
    double magnitude = 5.0 / sqrt(real_part * real_part + imag_part * imag_part);
    
    // 目标输出幅值为2V，需要的输入幅值补偿
    double required_input = 2.0 / magnitude;
    
    // 转换回单精度并限制补偿范围，避免DAC饱和
    float result = (float)required_input;
    if(result > 3.2f) result = 3.2f;
    if(result < 0.1f) result = 0.1f;
    
    return result;
}

// 在Model_3函数中，修改switch (control_mode)部分：

void Model_3(void)
{
    static uint32_t dac_frequency = 100;      // DAC输出频率，初始100Hz
    static uint32_t frequency_step = 100;     // 频率步长，固定100Hz
    static uint32_t max_frequency = 3000;     // 最大频率3kHz
    static uint32_t min_frequency = 100;      // 最小频率100Hz
    
    // 新增电压控制变量
    static float target_voltage = 2.0f;      // 目标输出电压峰峰值，初始2V
    static float voltage_step = 0.1f;        // 电压步长0.1V
    static float max_voltage = 2.0f;         // 最大电压2V
    static float min_voltage = 1.0f;         // 最小电压1V
    
    static uint8_t control_mode = 0;         // 控制模式：0=电压控制，1=频率控制
    static uint8_t initialized = 0;         // 初始化标志
	OLED_Clear(); // 清屏
    
    // 首次运行时初始化DAC
    if (!initialized)
    {
        // 使用优化的计算函数
        float amplitude_compensation = calculate_amplitude_compensation(dac_frequency);
        
        // 使用电压幅值生成正弦波表
        generate_sin_table(amplitude_compensation);
        
        DAC_Configuration(SIN_Value, SIN_Value_Length);
        Tim4_Configuration(dac_frequency * SIN_Value_Length, 1);
        
        printf("Model_3 initialized. Mode: VOLTAGE control\r\n");
        printf("Freq: %lu Hz, Target: %.1fV, Input: %.3fV\r\n", 
               dac_frequency, target_voltage, amplitude_compensation);
        printf("KEY1: switch mode, KEY0/KEY2: adjust value, WakeUp: exit\r\n");
        initialized = 1;
    }
	OLED_ShowString(0, 0, "Model: 3", 16, 1); // 显示当前模型
    while(1)
    {
         // 清除显示区域，避免字符重叠
        OLED_ShowString(0, 0, "Model: 3        ", 16, 1); // 显示当前模型
        

		// KEY1: 切换控制模式（电压/频率）
        if(KEY1_interrupt_flag)
        {
            KEY1_interrupt_flag = 0; // 清除中断标志
            
            control_mode = !control_mode; // 切换模式
        }
            if(control_mode == 0)
            {
                OLED_ShowString(0, 16, "Mode: Voltage   ", 16, 1);
            }
            else
            {
                OLED_ShowString(0, 16, "Mode: Frequency ", 16, 1);
            }
        // 根据控制模式处理按键
        switch (control_mode)
        {
            case 0: // 电压控制模式
            {
                // KEY2: 增加电压
                if(KEY2_interrupt_flag)
                {
                    KEY2_interrupt_flag = 0;
                    
                    if(target_voltage + voltage_step <= max_voltage + 0.001f) // 浮点数比较容错
                    {
                        target_voltage += voltage_step;
                        
                        // 使用优化的计算函数，基于新的目标电压重新计算
                        float amplitude_compensation = (target_voltage / 2.0f) * calculate_amplitude_compensation(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2f) amplitude_compensation = 3.2f;
                        if(amplitude_compensation < 0.1f) amplitude_compensation = 0.1f;
                        
                        // 重新生成正弦波表
                        generate_sin_table(amplitude_compensation);
                        
                        printf("Voltage+: %.1fV, Freq: %lu Hz, Input: %.3fV\r\n", 
                               target_voltage, dac_frequency, amplitude_compensation);
                    }
                    else
                    {
                        printf("Maximum voltage reached: %.1fV\r\n", max_voltage);
                    }
                }
                
                // KEY0: 减少电压
                if(KEY0_interrupt_flag)
                {
                    KEY0_interrupt_flag = 0;
                    
                    if(target_voltage >= min_voltage + voltage_step - 0.001f) // 浮点数比较容错
                    {
                        target_voltage -= voltage_step;
                        
                        // 使用优化的计算函数，基于新的目标电压重新计算
                        float amplitude_compensation = (target_voltage / 2.0f) * calculate_amplitude_compensation(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2f) amplitude_compensation = 3.2f;
                        if(amplitude_compensation < 0.1f) amplitude_compensation = 0.1f;
                        
                        // 重新生成正弦波表
                        generate_sin_table(amplitude_compensation);
                        
                        printf("Voltage-: %.1fV, Freq: %lu Hz, Input: %.3fV\r\n", 
                               target_voltage, dac_frequency, amplitude_compensation);
                    }
                    else
                    {
                        printf("Minimum voltage reached: %.1fV\r\n", min_voltage);
                    }
                }
                break;
            }
            
            case 1: // 频率控制模式
            {
                // KEY2: 增加频率
                if(KEY2_interrupt_flag)
                {
                    KEY2_interrupt_flag = 0;
                    
                    if(dac_frequency + frequency_step <= max_frequency)
                    {
                        dac_frequency += frequency_step;
                        
                        // 使用优化的计算函数
                        float amplitude_compensation = (target_voltage / 2.0f) * calculate_amplitude_compensation(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2f) amplitude_compensation = 3.2f;
                        if(amplitude_compensation < 0.1f) amplitude_compensation = 0.1f;
                        
                        // 重新生成正弦波表和配置定时器
                        generate_sin_table(amplitude_compensation);
                        Tim4_Configuration(dac_frequency * SIN_Value_Length, 1);
                        
                        printf("Freq+: %lu Hz, Target: %.1fV, Input: %.3fV\r\n", 
                               dac_frequency, target_voltage, amplitude_compensation);
                    }
                    else
                    {
                        printf("Maximum frequency reached: %lu Hz\r\n", max_frequency);
                    }
                }
                
                // KEY0: 减少频率
                if(KEY0_interrupt_flag)
                {
                    KEY0_interrupt_flag = 0;
                    
                    if(dac_frequency >= min_frequency + frequency_step)
                    {
                        dac_frequency -= frequency_step;
                        
                        // 使用优化的计算函数
                        float amplitude_compensation = (target_voltage / 2.0f) * calculate_amplitude_compensation(dac_frequency);
                        
                        // 限制补偿范围
                        if(amplitude_compensation > 3.2f) amplitude_compensation = 3.2f;
                        if(amplitude_compensation < 0.1f) amplitude_compensation = 0.1f;
                        
                        // 重新生成正弦波表和配置定时器
                        generate_sin_table(amplitude_compensation);
                        Tim4_Configuration(dac_frequency * SIN_Value_Length, 1);
                        
                        printf("Freq-: %lu Hz, Target: %.1fV, Input: %.3fV\r\n", 
                               dac_frequency, target_voltage, amplitude_compensation);
                    }
                    else
                    {
                        printf("Minimum frequency reached: %lu Hz\r\n", min_frequency);
                    }
                }
                break;
            }
            
            default:
                break;
        }
        // 显示频率 - 修正显示位置和格式
        OLED_ShowString(0, 32, "Freq:", 16, 1);
        OLED_ShowNum(64, 32, dac_frequency, 4, 16, 1); // 显示频率数值
        OLED_ShowString(96, 32, "Hz  ", 16, 1); // 显示单位
        
        // 显示目标电压 - 修正小数显示
        OLED_ShowString(0, 48, "Volt:", 16, 1);
        // 将浮点数转换为整数显示（例如2.1V显示为21，表示2.1V）
        uint16_t voltage_display = (uint16_t)(target_voltage * 10);
        uint16_t voltage_integer = voltage_display / 10;
        uint16_t voltage_decimal = voltage_display % 10;
        
        OLED_ShowNum(64, 48, voltage_integer, 1, 16, 1); // 整数部分
        OLED_ShowString(80, 48, ".", 16, 1);             // 小数点
        OLED_ShowNum(88, 48, voltage_decimal, 1, 16, 1); // 小数部分
        OLED_ShowString(104, 48, "V  ", 16, 1);          // 单位
		OLED_Refresh(); // 刷新OLED显示

        // 退出条件：使用WakeUp按键
        if(Key_WakeUp_interrupt_flag)
        {
            Key_WakeUp_interrupt_flag = 0; // 清除中断标志
            printf("Exit Model_3\r\n");
			model = 0; // 切换到模型0
            
            // 停止DAC输出
            TIM_Cmd(TIM4, DISABLE);
            DAC_Cmd(DAC_Channel_2, DISABLE);
            return; // 退出函数
        }
    }
}


