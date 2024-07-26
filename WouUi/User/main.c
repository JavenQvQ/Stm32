#include "stm32f10x.h" 
#include "Oled_ui.h"               // Device header
#include "Delay.h"
#include "testui.h"
#include "key.h"
#include "myrtc.h"
#include "buzzer.h"
#include <stdint.h>

Calender time = 
{
    .year = 24,
    .month = 3,
    .day = 1,
	.week = 5,
    .hour = 23,
    .min = 59,
    .sec = 55,
};
Alarm alarm=
{
	.year = 24,
    .month = 3,
    .day = 1,
	.week = 5,
    .hour = 23,
    .min = 59,
    .sec = 59,
};
uint16_t time_out;
void MyRTC__GetAlarm_SetFlag(uint8_t al_num)
{
	uint8_t alatm_sta = 0;
	uint8_t msk = 0;
	DigitalPage * temp_dp = NULL;
    OLED_DigitalPage_UpdateDigitalNumAnimation(temp_dp ,alarm.hour, alarm.min, alarm.sec, Digital_Direct_Increase);
    if(alatm_sta != 0) SET_FLAG(update_flag, msk);
    else CLEAR_FLAG(update_flag, msk);	
}
uint8_t Alarm_TimeSet_Flag=0;
uint16_t countdown;






int main(void)
{
	SysTick_Config(SystemCoreClock/1000);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	MyRTC_Init();
	MyRTC_SetTime(&time);//设置时间
	MyRTC_ReadTime(&time);
	TestUI_Init();
    Buzzer_Init();
    OLED_DigitalPage_UpdateDigitalNumAnimation(&clock_page,time.hour, time.min, time.sec, Digital_Direct_Increase);
    OLED_DigitalPage_UpdateDigitalNumAnimation(&calendar_page,time.year, time.month, time.day, Digital_Direct_Increase);
    OLED_DigitalPage_UpdateLabelAnimation(&calendar_page, time.week -2, Digital_Direct_Increase);
    OLED_DigitalPage_UpdateLabelAnimation(&clock_page, time.week -2, Digital_Direct_Increase);
	Key_Init();
	uint8_t key;
	while (1)
	{
		TestUI_Proc(); 
		key=Key_GetNum();
		if(OLED_UIGetCurrentPageID() == ring_page.page.page_id) //在ring页面按下按键，关闭闹钟
		{

			if(key)
			{
                Buzzer_OFF();
				if(FLAG_IS_SET(update_flag,ALARM1_RING_MSK) )
                {OLED_UIChangeCurrentPage(&alarm1_page);CLEAR_FLAG(update_flag,ALARM1_RING_MSK);}
				key=0;//清除按键
			}  
        } //因此在ring页面不会给消息队列发送消息
		else
		{
			switch (key)//按键处理
			{
				case 1: OLED_MsgQueSend(msg_click);key=0; break;
				case 2: OLED_MsgQueSend(msg_up);key=0; break;
				case 3: OLED_MsgQueSend(msg_down);key=0; break;
				case 4: OLED_MsgQueSend(msg_return);key=0; break;
				default: break;
			}
		}
      if(FLAG_IS_SET(update_flag,TIME_UPDATE_MSK))
      {
          time.hour = clock_page.option_array[Digital_Pos_IndexLeft].val;
          time.min = clock_page.option_array[Digital_Pos_IndexMid].val;
          time.sec = clock_page.option_array[Digital_Pos_IndexRight].val;
          time.week = clock_page.select_label_index-2;
          MyRTC_SetTime(&time);
          CLEAR_FLAG(update_flag,TIME_UPDATE_MSK);
      }
      if(FLAG_IS_SET(update_flag, DATE_UPDATE_MSK))
      {
          time.year = calendar_page.option_array[Digital_Pos_IndexLeft].val;
          time.month = calendar_page.option_array[Digital_Pos_IndexMid].val;
          time.day = calendar_page.option_array[Digital_Pos_IndexRight].val;
          time.week = calendar_page.select_label_index-2;
          MyRTC_SetTime(&time);
          MyRTC_ReadTime(&time);
          OLED_DigitalPage_UpdateLabelAnimation(&calendar_page, time.week -1, Digital_Direct_Increase);
          CLEAR_FLAG(update_flag,DATE_UPDATE_MSK);
      }
      if(FLAG_IS_SET(update_flag, ALARM1_UPDATE_MSK))
      {
		  alarm.year = time.year;
		  alarm.month = time.month;
          alarm.day = time.day;  
          alarm.hour = alarm1_page.option_array[Digital_Pos_IndexLeft].val;
          alarm.min = alarm1_page.option_array[Digital_Pos_IndexMid].val;
          alarm.sec = alarm1_page.option_array[Digital_Pos_IndexRight].val;
          Alarm_TimeSet_Flag=1;
          MyRTC_AlarmSet(&alarm);
          CLEAR_FLAG(update_flag,ALARM1_UPDATE_MSK);
      }
      if(Alarm_TimeSet_Flag==1)
      {
        if(alarm.hour==time.hour)
        {
            if(alarm.min==time.min)
            OLED_UIChangeCurrentPage(&ring_page);
            else if (alarm.min-1==time.min&&alarm.sec<=time.sec)
            OLED_UIChangeCurrentPage(&ring_page);

        }
        Alarm_TimeSet_Flag=0;
      }



          if(clock_page.mod == Digital_Mode_Observe && (!FLAG_IS_SET(update_flag,TIME_UPDATE_MSK)))
          { //处于观察模式，且没有设置时间时才会更新时间
              MyRTC_ReadTime(&time);
    OLED_DigitalPage_UpdateDigitalNumAnimation(&clock_page,time.hour, time.min, time.sec, Digital_Direct_Increase);
 
              if(time.hour == 0 && time.min == 0 && time.sec == 1) //每天第一秒刷新一次日程
              {
                OLED_DigitalPage_UpdateDigitalNumAnimation(&calendar_page,time.year, time.month, time.day, Digital_Direct_Increase);
                OLED_DigitalPage_UpdateLabelAnimation(&calendar_page, time.week -2, Digital_Direct_Increase);
                OLED_DigitalPage_UpdateLabelAnimation(&clock_page, time.week-2 , Digital_Direct_Increase);
              }
          }
    }
}