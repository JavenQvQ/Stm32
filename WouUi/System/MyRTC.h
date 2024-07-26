#ifndef _MYRTC_H
#define _MYRTC_H
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

typedef struct
{
    uint16_t year;
    uint16_t month;
    uint16_t day;
	uint16_t week;
    uint16_t hour;
    uint16_t min;
    uint16_t sec;
}Alarm;
void MyRTC_Init(void);
void MyRTC_SetTime(Calender *MyRTC_Time);
void MyRTC_ReadTime(Calender *MyRTC_Time);
void MyRTC_AlarmSet(Alarm *MyRTC_Time);

#endif