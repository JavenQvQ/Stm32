#include "stm32f10x.h"
#include "Delay.h"
#include "testui.h"
#include "buzzer.h"
#include <time.h>
typedef struct
{
    uint16_t year;
    uint16_t month;
    uint16_t day;
	uint16_t week;
    uint16_t hour;
    uint16_t min;
    uint16_t sec;
}Calender;


Calender cal = 
{
    .year = 24,
    .month = 3,
    .day = 1,
	.week = 5,
    .hour = 21,
    .min = 44,
    .sec = 55,
};
void MyRTC_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if(BKP_ReadBackupRegister(BKP_DR1) != 0x5050)//检查是否第一次配置
    {

        RCC_LSEConfig(RCC_LSE_ON);//启动LSE晶振
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);//等待LSE晶振就绪

        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);//选择LSE晶振作为RTC时钟源
        RCC_RTCCLKCmd(ENABLE);//使能RTC时钟

        RTC_WaitForLastTask();//等待最后一个写入到RTC的操作完成
        RTC_WaitForSynchro();//等待RTC的时间和日期寄存器同步

        RTC_SetPrescaler(30000);//设置RTC预分频值，使RTC时钟为1Hz
        RTC_WaitForLastTask();//等待上一次操作完成
    
        RTC_SetCounter(1704067200);//设置RTC计数器的值
        RTC_WaitForLastTask();//等待上一次操作完成

        BKP_WriteBackupRegister(BKP_DR1, 0x5050);//标记已经配置过
    
    }
    else
    {
        RCC_RTCCLKCmd(ENABLE);//使能RTC时钟
        RTC_WaitForSynchro();//等待RTC的时间和日期寄存器同步
    }
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;//RTC闹钟中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);//初始化NVIC



    RTC_ITConfig(RTC_IT_ALR|RTC_IT_SEC, ENABLE);//使能RTC闹钟中断
    RTC_WaitForLastTask();//等待上一次操作完成
    RTC_ClearITPendingBit(RTC_IT_ALR);//清除RTC闹钟中断标志
    RTC_WaitForLastTask();//等待上一次操作完成 
}
void MyRTC_SetTime(Calender *MyRTC_Time)//需要在main函数中定义一个结构体，用于存放时间,结构体名直接传入函数即可
{
    time_t time_cnt;//时间变量
    struct tm time_date;//时间结构体
    time_date.tm_year=MyRTC_Time->year+100;//年
    time_date.tm_mon=MyRTC_Time->month-1;//月
    time_date.tm_mday=MyRTC_Time->day;//日
    time_date.tm_hour=MyRTC_Time->hour;//时
    time_date.tm_min=MyRTC_Time->min;//分
    time_date.tm_sec=MyRTC_Time->sec;//秒
    time_cnt=mktime(&time_date);//将时间结构体转换为时间变量
    RTC_WaitForLastTask();//等待上一次操作完成
    RTC_SetCounter(time_cnt);//设置RTC计数器的值
    RTC_WaitForLastTask();//等待上一次操作完成
}
void MyRTC_ReadTime(Calender *MyRTC_Time)//需要在main函数中定义一个数组，用于存放时间,数组名直接传入函数即可
{
    time_t time_cnt;//时间变量
    struct tm *time_date;//时间结构体 指针
    time_cnt=RTC_GetCounter();//获取RTC计数器的值
    time_date=localtime(&time_cnt);//将时间变量转换为时间结构体
    MyRTC_Time->year=time_date->tm_year-100;//年
    MyRTC_Time->month=time_date->tm_mon+1;//月
    MyRTC_Time->day=time_date->tm_mday;//日
    MyRTC_Time->week=time_date->tm_wday+1;//星期
    MyRTC_Time->hour=time_date->tm_hour;//时
    MyRTC_Time->min=time_date->tm_min;//分
    MyRTC_Time->sec=time_date->tm_sec;//秒
}
void MyRTC_AlarmSet(Calender *MyRTC_Time)//需要在main函数中定义一个数组，用于存放时间,数组名直接传入函数即可
{
    time_t time_cnt;//时间变量
    struct tm time_date;//时间结构体
    time_date.tm_year=MyRTC_Time->year+100;//年
    time_date.tm_mon=MyRTC_Time->month-1;//月
    time_date.tm_mday=MyRTC_Time->day;//日
    time_date.tm_hour=MyRTC_Time->hour;//时
    time_date.tm_min=MyRTC_Time->min;//分
    time_date.tm_sec=MyRTC_Time->sec;//秒
    time_cnt=mktime(&time_date);//将时间结构体转换为时间变量
    RTC_WaitForLastTask();//等待上一次操作完成
    RTC_SetAlarm(time_cnt);//设置闹钟的值
    RTC_WaitForLastTask();//等待上一次操作完成

}




//RTC闹钟中断服务函数
void RTC_IRQHandler(void)
{
    if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
        RTC_ClearITPendingBit(RTC_IT_ALR);//清除RTC闹钟中断标志
        SET_FLAG(update_flag, ALARM1_RING_MSK);
        Buzzer_ON();
        OLED_UIChangeCurrentPage(&ring_page);
        
    }
    if(RTC_GetITStatus(RTC_IT_SEC) != RESET)
    {
        RTC_ClearITPendingBit(RTC_IT_SEC);//清除RTC秒中断标志
        //这里添加秒中断服务函数
    }
}

