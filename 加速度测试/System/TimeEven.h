#ifndef __TIMEEVEN_H
#define __TIMEEVEN_H

#include "string.h"

typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned long DWORD;//定义DWORD为32位无符号整数
typedef float FLOAT;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned long ULONG;

typedef void VOID;
typedef void *PVOID;

typedef enum {FALSE1 = 0, TRUE1 = !FALSE1} bool;

#define TASK_MAX_NUM 16       //任务最大个数(至少为2)
#define FIRST_TASK 0
#define TASK_BASE_INTERVAL 1 //单位：毫秒
#ifndef NULL
#define NULL 0
#endif
//这里TASK_FUN是一个数据类型，该类型可以用来定义一个指向返回值为void,参数为void函数的指针。
typedef void (*TASK_FUN)(void);  //指向函数的指针，该指针指向的函数形式严格规定为:void fun(void)

typedef struct 
{
  BYTE Task_ID;        //任务ID
  WORD Task_Time;      //任务调用周期
  WORD CNT;            //任务时间计数器，通过此变量可以查询任务剩余的时间
  TASK_FUN pFunction;  //指向函数的指针，定时时间到了CNT=0，调用pFunction指向的函数
}Task_Typedef;

bool AddTask(BYTE Task_ID, DWORD Task_Time,TASK_FUN fun);
bool RemoveTask(BYTE Task_ID);
bool ResetTaskTime(BYTE Task_ID);
bool ChangeTaskTime(BYTE Task_ID,DWORD Task_Time );

static bool AddTail(const Task_Typedef* pTaskPara);
bool DelTail( Task_Typedef* pTaskPara);
static BYTE FindID(BYTE Task_ID);
bool ChangeTail(Task_Typedef* pTaskPara);
void TASK_OS(void);

#endif




