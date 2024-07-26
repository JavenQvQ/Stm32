/**********************************************************
                       康威电子
功能：stm32f103rct6控制AD9854模块输出点频及扫频，长按驱动板中间键切换。
接口：控制引脚接口请参照AD9854.h
时间：
版本：0.6
作者：康威电子
其他：

更多电子需求，请到淘宝店，康威电子竭诚为您服务 ^_^
https://kvdz.taobao.com/ 
**********************************************************/

#include "stm32_config.h"
#include "stdio.h"
#include "led.h"
#include "lcd.h"
#include "AD9854.h" 
#include "key.h"
#include "task_manage.h"
#include "timer.h"

extern uint8_t  USART_RX_BUF[USART_REC_LEN];	//接收缓冲，最大USART_REC_LEN个字节，末字节为换行符 
extern uint16_t USART_RX_STA;					//接收状态标记
uint64_t frequence=1000000;	//频率
uint16_t amplitude=4095;	//幅度
char str[30];	//显示缓存
extern u8 _return;

int main(void)
{
	MY_NVIC_PriorityGroup_Config(NVIC_PriorityGroup_2);	//设置中断分组
	delay_init(72);	//初始化延时函数
	LED_Init();	//初始化LED接口
	USARTx_Init(115200);	//初始化串口
	initial_lcd();
	LCD_Clear();
	delay_ms(300);
	LCD_Refresh_Gram();
	Timerx_Init(99,71);
	LCD_Clear();

	AD9854_IO_Init();
	AD9854_InitSingle();
	AD9854_SetSine_double(1000000,4095);//写频率1M Hz,0X0000~4095对应峰峰值0mv~550mv(左右)
	
	//1、
	//后续代码涉及界面及按键交互功能，频率或幅度或其他参数可能被重写，可能导致上面更改参数无效
	//上述情况，取消下面注释即可，可直接更改频率及幅度，
////	AD9854_SetSine_double(1000000,4095);//写频率1M Hz,0X0000~4095对应峰峰值0mv~550mv(左右)
////	while(1);
	
	//2、	
//	关于扫频的说明
//	该程序的扫频功能利用定时器中断，不断写入自加的频率实现，
//	在timer.C的TIM4_IRQHandler中将自加后的频率写入

	while(1)
	{
		if(USART_RX_STA & 0x8000)	//接收到了一次数据
		{
			USART_RX_BUF[USART_RX_STA & 0X3FFF] = '\0';	//添加结束符,USART_RX_STA & 0X3FFF是接收到的数据长度
			
			switch(USART_RX_BUF[0])
			{
			case 0x01:
					frequence = 0x0000;
					frequence = USART_RX_BUF[1]<<24 | USART_RX_BUF[2]<<16 | USART_RX_BUF[3]<<8 | USART_RX_BUF[4];
					amplitude = 0x0000;
					amplitude = USART_RX_BUF[5]<<8 | USART_RX_BUF[6];
					printf("Received: %llu\n", frequence);
					AD9854_SetSine_double(frequence, amplitude);
					break;
			case 0x02:

		    }
					
			USART_RX_STA = 0x0000;	//清除接收状态
		}

	}	
}


