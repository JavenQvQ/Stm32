#include "stm32f10x.h" // Device header


#include "key.h"
#define KEY_FILTER_TIME 5//按键滤波时间
#define KEY_NUM 3 //按键数量
KEY_T s_tBtn[3]= {0};//按键数组
static Key_FIFO s_tKey;

/*
**********************************************************
* 函 数 名: KEY_FIFO_Put
* 功能说明: 将1个键值压入按键FIFO缓冲区。可用于模拟一个按键。
* 形    参:  _KeyCode : 按键代码
* 返 回 值: 无
**********************************************************
*/

void KEY_FIFO_Put(uint8_t _KeyCode)
{
s_tKey.Buf[s_tKey.Write] = _KeyCode;

 if (++s_tKey.Write  >= 10)
 {
  s_tKey.Write = 0;
 }
}
/*
***********************************************************
* 函 数 名: KEY_FIFO_Get
* 功能说明: 从按键FIFO缓冲区读取一个键值。
* 形    参: 无
* 返 回 值: 按键代码
************************************************************
*/
uint8_t KEY_FIFO_Get(void)
{
 uint8_t ret;

 if (s_tKey.Read == s_tKey.Write)
 {
  return KEY_NONE;
 }
 else
 {
  ret = s_tKey.Buf[s_tKey.Read];

  if (++s_tKey.Read >= 10)//这里是一个循环队列
  {
   s_tKey.Read = 0;
  }
  return ret;
 }
}
//判断按键是否按下
static uint8_t IsKey1Down(void)
{
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)==0)
	return 1;
	else
	return 0;
}
static uint8_t IsKey2Down(void)
{
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)==0)
	return 1;
	else
	return 0;
}
static uint8_t IsKey3Down(void)
{
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==0)
	return 1;
	else
	return 0;
}



/*
 * 按键检测函数
 * i: 按键序号	
 * 
 * 功能：按键检测函数，根据按键的状态，更新按键的状态，并将按键事件压入按键FIFO缓冲区。
 * */
static void KEY_Detect(uint8_t i)
{
    KEY_T *pBtn;
    pBtn = &s_tBtn[i];
    if (pBtn->IsKeyDownFunc()) //判断按键是否按下,没有按下则直接跳到else
    {
        if (pBtn->Count < KEY_FILTER_TIME) //按键计数器会被置为 KEY_FILTER_TIME
        {
            pBtn->Count = KEY_FILTER_TIME;
        }
        else if(pBtn->Count < 2 * KEY_FILTER_TIME) //进行滤波
        {
            pBtn->Count++;
        }
        else //按键按下
        {
            if (pBtn->State == 0)
            {
                pBtn->State = 1;
                KEY_FIFO_Put((uint8_t)(3 * i + 1));
            }
            //长按
            if (pBtn->LongTime > 0)
            {
                if (pBtn->LongCount < pBtn->LongTime) /*LongTime初始值是1000，持续1秒，认为长按事件*/
                {
                    if (++pBtn->LongCount == pBtn->LongTime) /*LongCount等于LongTime(100),10ms进来一次，进来了100次也就是说按下时间为于1s*/
                    {
                        KEY_FIFO_Put((uint8_t)(3 * i + 3));
                    }
                }
                else /*LongCount大于LongTime(100)时执行此语句,也就是说按下时间大于1s*/
                {
                    if (pBtn->RepeatSpeed > 0) /* RepeatSpeed连续按键周期 */
                    {
                        if (++pBtn->RepeatCount >= pBtn->RepeatSpeed)
                        {
                            /* 长按键后，每隔10ms发送1个按键。因为长按也是要发送键值得，10ms发送一次。*/
                            pBtn->RepeatCount = 0;
                            KEY_FIFO_Put((uint8_t)(3 * i + 3));
                        }
                    }
                }
            }
        }
    }
    else //按键未按下
    {
        /*这个里面执行的是按键松手的处理或者按键没有按下的处理*/
        if(pBtn->Count > KEY_FILTER_TIME)
        {
            pBtn->Count = KEY_FILTER_TIME;
        }
        else if(pBtn->Count != 0)
        {
            pBtn->Count--;
        }
        else
        {
            if (pBtn->State == 1)
            {
                pBtn->State = 0;
                KEY_FIFO_Put((uint8_t)(3 * i + 2));
            }
        }
        pBtn->LongCount = 0; //长按计数器清零
        pBtn->RepeatCount = 0;
    }
}

//扫描按键
void KEY_Scan(void)
{
 uint8_t i;

 for (i = 0; i < KEY_NUM; i++)
 {
  KEY_Detect(i);
 }

}


void Timer4_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
	TIM_TimeBaseInitTypeDef TIM4_t;
	TIM4_t.TIM_ClockDivision=TIM_CKD_DIV1;//设置时钟分割:TDTS=TIM_CKD_DIV1
	TIM4_t.TIM_CounterMode=TIM_CounterMode_Up;//设置计数器模式
	TIM4_t.TIM_Period=100-1;//设置自动重装载寄存器周期的值ARR
	TIM4_t.TIM_Prescaler=72-1;//设置用来作为TIMx时钟频率除数的预分频值PSC
	TIM_TimeBaseInit(TIM4,&TIM4_t);
	TIM_ClearFlag(TIM4,TIM_FLAG_Update);//清除更新标志

	NVIC_InitTypeDef NVIC_t;//初始化NVIC
	NVIC_t.NVIC_IRQChannel=TIM4_IRQn;//TIM4中断
	NVIC_t.NVIC_IRQChannelCmd=ENABLE;//使能中断
	NVIC_t.NVIC_IRQChannelPreemptionPriority=1;//抢占优先级
	NVIC_t.NVIC_IRQChannelSubPriority=1;//子优先级

	NVIC_Init(&NVIC_t);
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);//使能更新中断	
	TIM_Cmd(TIM4,ENABLE);//使能TIM2外设	
}
	

	


void Key_Init()//按键初始化
{
	//Timer4_Init();//定时器初始化
	uint8_t i;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_b;
	GPIO_b.GPIO_Mode=GPIO_Mode_IPU;//上拉输入
	GPIO_b.GPIO_Pin=GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
	GPIO_b.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_b);

 /* 对按键FIFO读写指针清零 */
 	s_tKey.Read = 0;
 	s_tKey.Write = 0;

 /* 给每个按键结构体成员变量赋一组缺省值 */
 	for (i = 0; i < KEY_NUM; i++)
	{
  	s_tBtn[i].LongTime = 1000;/* 长按时间 0 表示不检测长按键事件 */
  	s_tBtn[i].Count = 5/ 2; /* 计数器设置为滤波时间的一半 */
    s_tBtn[i].LongCount = 0;/* 长按计数器 */
  	s_tBtn[i].State = 0;/* 按键缺省状态，0为未按下 */
  	s_tBtn[i].RepeatSpeed = 100;/* 按键连发的速度，0表示不支持连发,数值表示按下时间间隔 */
  	s_tBtn[i].RepeatCount = 0;/* 连发计数器 */
 	}
 /* 判断按键按下的函数 */
 	s_tBtn[0].IsKeyDownFunc = IsKey1Down;
 	s_tBtn[1].IsKeyDownFunc = IsKey2Down;
 	s_tBtn[2].IsKeyDownFunc = IsKey3Down;
}


void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4,TIM_IT_Update)!=RESET)
	{
		KEY_Scan();
		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
	}
}
		
