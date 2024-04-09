/**************************************************************************
*************************前后台系统移植与使用说明**************************
***************************************************************************
注意事项：小于512Byte 的RAM不能使用该前后台系统
移植步骤：
1、编写定时中断程序，定时时间TASK_BASE_INTERVAL = 1ms 或者10ms。
任务调度时间必须为TASK_BASE_INTERVAL的整数倍。
2、在中断服务函数里调用void TASK_OS(void)；切记函数返回值，参数必须为void类型。
3、系统提供了delay_us，delay_ms函数，此函数最好不要在任何中断函数里使用。
这些函数在os_delay.c文件里实现。
4、设置系统支持的最大任务个数TASK_MAX_NUM，一般16个任务就可以了。
这里和单片机的RAM大小有关系。RAM越大，支持的任务越多。
使用步骤：
1、通过宏定义设置任务ID，用来区分不同的任务，任务ID起始编号从1开始，
编号0是系统保留编号，前后台系统至少要设置1个用户任务。
2、编写任务函数，函数类型需符合 void Func(void);格式。如果必须使用参数，
那么，可以按照下面形式来使用。
void mytest(uint16_t val)
{
	for(uint16_t i = 0;i < val;i++);
}
void TASK1(void)
{
	mytest(12);
}
//在系统里添加任务，任务ID,多久执行一次任务，任务函数
bool AddTask(BYTE Task_ID, DWORD Task_Time,TASK_FUN fun);
//从系统里删除任务，任务ID
bool RemoveTask(BYTE Task_ID);
//复位任务计数器CNT,重新开始倒计时
bool ResetTaskTime(BYTE Task_ID);
//重新设置任务调用周期
bool ChangeTaskTime(BYTE Task_ID,DWORD Task_Time );

B站：奇遇单片机
作者：龙虾哥

***************************************************************************
**************************************************************************/
#include "TimeEven.h"

Task_Typedef TaskInfoArray[TASK_MAX_NUM] = {0};

BYTE g_nTimerInfoNum = 0; //当前队列的元素个数

//AddTask()
//ID:input,输入的任务ID，ID的范围：1至 TASK_MAX_NUM-1；
//Task_Time：input,输入时间多久之后触发，如果时间1S之后触发，那么Task_Time=1000；
//fun:input,函数指针，具体的看TASK_FUN是如何定义
//加入任务ID和多久之后触发这个任务（就是任务间隔）

bool AddTask(BYTE Task_ID, DWORD Task_Time,TASK_FUN fun){
  Task_Typedef ti;
  if( Task_Time == 0|| Task_ID==0){
    return FALSE1;
  }
  if(fun == NULL){
    return FALSE1;
  }
  if(Task_Time % TASK_BASE_INTERVAL != 0){ //定时间隔必须是TASK_BASE_INTERVAL的整数倍
    return FALSE1;
  }
  if(FindID(Task_ID) != TASK_MAX_NUM){ //若已经有相同的ID存在
    return FALSE1;
  }
  ti.Task_ID = Task_ID;
  ti.Task_Time = Task_Time;
  ti.CNT = Task_Time;
  ti.pFunction = fun;
  if(!AddTail(&ti)){
    return FALSE1;
  }
  return TRUE1;

}
bool ChangeTaskTime(BYTE Task_ID,DWORD Task_Time ){
  Task_Typedef ti;
  if(Task_ID==0){
    return FALSE1;
  }
  ti.Task_ID = Task_ID;
  ti.Task_Time = Task_Time;
  if(!ChangeTail(&ti)){
    return FALSE1;
  }
  return TRUE1;
}
bool ResetTaskTime(BYTE Task_ID)
{
	BYTE delNumber=0;
	
  delNumber=FindID(Task_ID);//根据Task_ID找出需要的任务号
	
  if(delNumber == TASK_MAX_NUM )
	{
     return FALSE1;
  }
	TaskInfoArray[delNumber].CNT = TaskInfoArray[delNumber].Task_Time;
	
	return TRUE1;
}
///////////////RemoveTask/////////
//将需要移除的任务ID送给DelTail
//Task_ID，需要移除的任务
bool RemoveTask(BYTE Task_ID){
  Task_Typedef ti;
  if(Task_ID==0){
    return FALSE1;
  }
  ti.Task_ID = Task_ID;
  if(!DelTail(&ti)){
    return FALSE1;
  }
  return TRUE1;
}

bool ChangeTail(Task_Typedef* pTaskPara){
  unsigned char changeNumber;
  changeNumber=FindID(pTaskPara->Task_ID);
  if(changeNumber == TASK_MAX_NUM ){
    return FALSE1;
  }
  if(pTaskPara->Task_ID!=TaskInfoArray[changeNumber].Task_ID)
    return FALSE1;
  pTaskPara->CNT = TaskInfoArray[changeNumber].CNT;
 
  pTaskPara->pFunction = TaskInfoArray[changeNumber].pFunction;
  memcpy(&TaskInfoArray[changeNumber],pTaskPara,sizeof(Task_Typedef));
  g_nTimerInfoNum=g_nTimerInfoNum;   
  return TRUE1;
}

 static BYTE FindID(BYTE Task_ID){
  BYTE i = 0;
  for(i = 0; i < TASK_MAX_NUM; i++)  {
    if(TaskInfoArray[i].Task_ID == Task_ID){
      return i;
    }
  }
  return TASK_MAX_NUM;
} 
/////////////DelTail/////////////
//将最后一个任务复制到需要移除的任务中
 bool DelTail( Task_Typedef* pTaskPara){
  unsigned char delNumber;
  delNumber=FindID(pTaskPara->Task_ID);//根据Task_ID找出需要移除的任务号
  if(delNumber == TASK_MAX_NUM ){
    return FALSE1;
  }
  if(delNumber<g_nTimerInfoNum )  {
    memcpy(&TaskInfoArray[delNumber], &TaskInfoArray[g_nTimerInfoNum-1], sizeof(Task_Typedef));
  }else{
    return FALSE1;
  }
  pTaskPara->Task_ID = 0;
  pTaskPara->CNT = 0;
  pTaskPara->Task_Time = 0;
  pTaskPara->pFunction = NULL;
  memcpy(&TaskInfoArray[g_nTimerInfoNum-1], pTaskPara, sizeof(Task_Typedef));  
  g_nTimerInfoNum=g_nTimerInfoNum-1;//由于g_nTimerInfoNum总是指向下一个任务，所以需要减1     
  return TRUE1;
}   
static bool AddTail(const Task_Typedef* pTaskPara){
  if(g_nTimerInfoNum == TASK_MAX_NUM || pTaskPara == NULL){
    return FALSE1;
  }
  memcpy(&TaskInfoArray[g_nTimerInfoNum], pTaskPara, sizeof(Task_Typedef));
  g_nTimerInfoNum++;
  return TRUE1;
} 
/***********************************************
此处为前后台系统核心代码，放在定时中断服务函数里，
一般使用SysTick定时器，定时10ms或者1ms
************************************************/
void TASK_OS(void)
{
	for(int i = 0; i < g_nTimerInfoNum; i++)  
	{
		TaskInfoArray[i].CNT -= TASK_BASE_INTERVAL;      
		if((TaskInfoArray[i].CNT == 0)||(TaskInfoArray[i].CNT>TaskInfoArray[i].Task_Time))
		{
			TaskInfoArray[i].CNT = TaskInfoArray[i].Task_Time;//重新装载定时器            
			(*(TaskInfoArray[i].pFunction))();//执行函数
		}
	}
}
