// #include "stm32f10x.h"                  // Device header

// /**
//   * 函    数：LED初始化
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED_Init(void)
// {
// 	/*开启时钟*/
// 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
// 	/*GPIO初始化*/
// 	GPIO_InitTypeDef GPIO_InitStructure;
// 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
// 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
// 	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA1和PA2引脚初始化为推挽输出
	
// 	/*设置GPIO初始化后的默认电平*/
// 	GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);				//设置PA1和PA2引脚为高电平
// }

// /**
//   * 函    数：LED1开启
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED1_ON(void)
// {
// 	GPIO_ResetBits(GPIOA, GPIO_Pin_1);		//设置PA1引脚为低电平
// }

// /**
//   * 函    数：LED1关闭
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED1_OFF(void)
// {
// 	GPIO_SetBits(GPIOA, GPIO_Pin_1);		//设置PA1引脚为高电平
// }

// /**
//   * 函    数：LED1状态翻转
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED1_Turn(void)
// {
// 	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0)		//获取输出寄存器的状态，如果当前引脚输出低电平
// 	{
// 		GPIO_SetBits(GPIOA, GPIO_Pin_1);					//则设置PA1引脚为高电平
// 	}
// 	else													//否则，即当前引脚输出高电平
// 	{
// 		GPIO_ResetBits(GPIOA, GPIO_Pin_1);					//则设置PA1引脚为低电平
// 	}
// }

// /**
//   * 函    数：LED2开启
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED2_ON(void)
// {
// 	GPIO_ResetBits(GPIOA, GPIO_Pin_2);		//设置PA2引脚为低电平
// }

// /**
//   * 函    数：LED2关闭
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED2_OFF(void)
// {
// 	GPIO_SetBits(GPIOA, GPIO_Pin_2);		//设置PA2引脚为高电平
// }

// /**
//   * 函    数：LED2状态翻转
//   * 参    数：无
//   * 返 回 值：无
//   */
// void LED2_Turn(void)
// {
// 	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2) == 0)		//获取输出寄存器的状态，如果当前引脚输出低电平
// 	{                                                  
// 		GPIO_SetBits(GPIOA, GPIO_Pin_2);               		//则设置PA2引脚为高电平
// 	}                                                  
// 	else                                               		//否则，即当前引脚输出高电平
// 	{                                                  
// 		GPIO_ResetBits(GPIOA, GPIO_Pin_2);             		//则设置PA2引脚为低电平
// 	}
// }
//-----------------------------------------------------------------
// 程序描述:
//     LED显示驱动程序
// 作    者: 凌智电子
// 开始日期: 2014-01-28
// 完成日期: 2014-01-28
// 修改日期: 2014-02-16
// 当前版本: V1.0.1
// 历史版本:
//  - V1.0: (2014-02-07)LED IO 配置
// - V1.0.1:(2014-02-16)头文件中不包含其他头文件
// 调试工具: 凌智STM32核心开发板、LZE_ST_LINK2
// 说    明:
//
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include <stm32f10x.h>
#include "LED.h"

//-----------------------------------------------------------------
// 初始化程序区
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// void GPIO_LED_Configuration(void)
//-----------------------------------------------------------------
//
// 函数功能: LED GPIO配置
// 入口参数: 无
// 返 回 值: 无
// 全局变量: 无
// 调用模块: RCC_APB2PeriphClockCmd();GPIO_Init();
// 注意事项: 无
//
//-----------------------------------------------------------------
void GPIO_LED_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // 使能IO口时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

  // LED灯PE4,PE5配置
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  // 推挽输出
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  // 先关闭两个LED
  LED1_OFF;
  LED2_OFF;
}

//-----------------------------------------------------------------
// End Of File
//----------