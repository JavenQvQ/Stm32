

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include <stm32f4xx.h>
#include "ads8361.h"
#include "board.h"
//-----------------------------------------------------------------
float ADS8361_DB_data1[1024];//DB数据
uint8_t ADS8361_Recive_Flag = 0;//接收标志

/********************************************************************************************
*函数名： TIM6_Init
*功 能： 初始化定时器6
*参 数： 无
*返 回： 无
备 注： 1.步骤：1.使能时钟 2.调用定时器初始化函数 3.允许更新中断（映射） 4.NVIC相关设置
2.RCC_APB1Periph时钟
3.基本定时器 (TIM6和TIM7)的特性:16 位自动重装载值递增计数器
4.举例：IM6_Int_Init(8000-1,5000-1); 定时器时钟422M，分频系数8400，所以84M/8400=10Khz的计数频率，计数5000次为500ms
********************************************************************************************/
void TIM6_Config( u32 Fre )
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	u32 MD;
	u16 div=1;
	while( (SystemCoreClock/2/Fre/div)>65535 )//计算分频系数
	{
		div++;
	}//计算分频系数
	MD =  SystemCoreClock/2/Fre/div - 1;	//计算自动重装载值
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM6 , ENABLE );			   //开启TIM3时钟
	TIM_TimeBaseStructure.TIM_Period = MD ;//自动重装载值
	TIM_TimeBaseStructure.TIM_Prescaler = div-1;//分频系数
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//时钟分频,不分频
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);//初始化TIM3
	
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);//允许更新中断
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;//TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//响应优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);//初始化NVIC
	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);//清除TIM3更新中断标志
	TIM_Cmd(TIM6, ENABLE);//使能TIM3
}


//-----------------------------------------------------------------
// void GPIO_ADS8361_Configuration(void)
//-----------------------------------------------------------------
// 
// 函数功能: ADS8361引脚配置函数
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void GPIO_ADS8361_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	  // 使能IO口时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//输入
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOD,&GPIO_InitStructure);

   GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9 |
																	GPIO_Pin_10| GPIO_Pin_11|
																	GPIO_Pin_13| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;//输出
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_Init(GPIOD, &GPIO_InitStructure);
}

//-----------------------------------------------------------------
// void ADS8361_Init(void)
//-----------------------------------------------------------------
// 
// 函数功能: ADS8361引脚配置函数
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void ADS8361_Init(void)
{
	GPIO_ADS8361_Configuration();
	
	M0_H;
	M1_H;
	A0_L;
	CS_H;	
	TIM6_Config(100000);//以100Khz的频率读取数据,在模式3下每通道最大吞吐量为250ksps,在模式4下每通道最大吞吐量为125ksps
}

//-----------------------------------------------------------------
// void ADS8361_Read_Data_Mode4(uint16_t *Data_A, uint16_t *Data_B)
//-----------------------------------------------------------------
// 
// 函数功能: 读取通道数据（模式：M0=1, M1=1, A=0）
// 入口参数: Data_A：通道A读取的数据
//		   	Data_B：通道B读取的数据
// 返 回 值: 无
// 注意事项: 使用四通道时因为是2个ADC多路复用4通道，所以每通道最大吞吐量250ksps，并且需要工作在模式3下。
//          四通道工作在模式4下时，每通道最大吞吐量125ksps.
//
//-----------------------------------------------------------------
void ADS8361_Read_Data_Mode4(uint16_t *Data_A, uint16_t *Data_B)
{
	uint8_t i,j; 	
	uint16_t DAT = 0;
	uint16_t CH = 0;
	uint16_t AB = 0;

	CS_L;
	for(j=0; j<4; j++)
	{
		DAT = 0;
		for(i=0; i<20; i++)
		{
			CLK_H;
			if(i == 0)//第一位
			{
				RD_CONVST_H;
				M0_H;
				M1_H;
				A0_L;
				CLK_L;
				__NOP();
			}
			else if(i == 1)
			{
				RD_CONVST_L;
				CLK_L;
				CH = (uint16_t)(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_14));
			}
			else if(i == 2)
			{
				CLK_L;
				AB = (uint16_t)(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_14));
			}
			else if(i < 19)
			{
				CLK_L;
				DAT = DAT << 1;

				if(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_14) == Bit_SET)
					DAT++;

			}
			else
			{
				CLK_L;
				__NOP();
			}
		}
		if((CH == 0) && (AB == 0))
			Data_A[0] = (uint16_t)DAT;
		else if((CH == 0) && (AB == 1))
			Data_B[0] = (uint16_t)DAT;
		else if((CH == 1) && (AB == 0))
			Data_A[1] = (uint16_t)DAT;
		else if((CH == 1) && (AB == 1))
			Data_B[1] = (uint16_t)DAT;
	}
	CS_H;	
}

//-----------------------------------------------------------------
// void ADS8361_Read_Data_Mode3(uint16_t *Data_A, uint16_t *Data_B)
//-----------------------------------------------------------------
// 
// 函数功能: 读取通道数据（模式：M0=1, M1=0, A=0）
// 入口参数: Data_A：通道A读取的数据
//					 Data_B：通道B读取的数据
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void ADS8361_Read_Data_Mode3(uint16_t *Data_A, uint16_t *Data_B)
{
	uint8_t i,j; 	
	uint16_t DAT_A = 0;
	uint16_t DAT_B = 0;
	uint16_t CH = 0;

	CS_L;
	for(j=0; j<2; j++)
	{
		DAT_A = 0;
		DAT_B = 0;
		for(i=0; i<20; i++)
		{
			CLK_H;
			if(i == 0)
			{
				RD_CONVST_H;
				M0_H;
				M1_L;
				A0_L;
				CLK_L;
				__NOP();
			}
			else if(i == 1)
			{
				RD_CONVST_L;
				CLK_L;
				CH = (uint16_t)(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_14));
			}
			else if(i == 2)
			{
				CLK_L;
				__NOP();
			}
			else if(i < 19)
			{
				CLK_L;
				DAT_A = DAT_A << 1;
				DAT_B = DAT_B << 1;

				if(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_14) == Bit_SET)
					DAT_A++;
				if(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_12) == Bit_SET)
					DAT_B++;
			}
			else
			{
				CLK_L;
				__NOP();
			}
		}
		if(CH == 0)
		{
			Data_A[0] = (uint16_t)DAT_A;
			Data_B[0] = (uint16_t)DAT_B;
		}
		else
		{
			Data_A[1] = (uint16_t)DAT_A;
			Data_B[1] = (uint16_t)DAT_B;
		}
	}
	CS_H;	
}



void TIM6_DAC_IRQHandler(void)
{
	static uint16_t num;//定义一个静态变量,不会被初始化
    uint8_t i = 0;
	uint16_t DA_data[2];//DA数据
	uint16_t DB_data[2];//DB数据
	float   Analog_A[2];
	float   Analog_B[2];

	if(TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)//检查TIM3更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
		ADS8361_Read_Data_Mode3(DA_data, DB_data);
        for(i=0; i<2; i++)
	   {
			if(DA_data[i] & 0x8000)
			{
				Analog_A[i] = ((DA_data[i] ^ 0x7FFF + 1)*5000.0/65535) - 2500;
			}
			else
			{
				Analog_A[i] = (DA_data[i]*5000.0/65535);
			}
			if(DB_data[i] & 0x8000)
			{
				Analog_B[i] = ((DB_data[i] ^ 0x7FFF + 1)*5000.0/65535) - 2500;
			}
			else
			{
				Analog_B[i] = (DB_data[i]*5000.0/65535);
			}	
			
		}
		ADS8361_DB_data1[num] = Analog_A[0];
		num++;
		if(num == 1024)
		{
			num = 0;
			TIM_Cmd(TIM6, DISABLE);//关闭TIM3
			ADS8361_Recive_Flag = 1;
		}		
	}	
}



	