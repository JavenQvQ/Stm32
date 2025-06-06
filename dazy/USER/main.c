#define __FPU_PRESENT 1
#define __FPU_USED 1
#include "arm_math.h"  // CMSIS DSP库的主头文件
#include "stdint.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "includes.h"
#include "os_app_hooks.h"
#include "led.h"
#include "lcd_init.h"
#include "lcd.h"
#include "pic.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_pwr.h"
#include "misc.h"
#include "ADX922.h"
#include "spi.h"

volatile float current_BPM = 0.0f;  // 当前心率值
volatile u8 bmp_updated = 0;        // BPM更新标志
//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		1024
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);


//心电数据处理任务
#define ECG_TASK_PRIO     3      // 任务优先级
#define ECG_STK_SIZE      2048    // 任务堆栈大小
OS_TCB ECGTaskTCB;                // 任务控制块
CPU_STK ECG_TASK_STK[ECG_STK_SIZE]; // 任务堆栈
void ecg_task(void *p_arg);       // 任务函数声明
OS_SEM ECGDataSem;    // 心电数据处理信号量


//LCD显示任务
#define DISP_TASK_PRIO     4      // 任务优先级，比ECG任务低一级
#define DISP_STK_SIZE      1024   // 任务堆栈大小
OS_TCB DISPTaskTCB;               // 任务控制块
CPU_STK DISP_TASK_STK[DISP_STK_SIZE]; // 任务堆栈
void disp_task(void *p_arg);      // 任务函数声明

#define WAVE_START_X 0          
#define WAVE_END_X 160          
#define WAVE_START_Y 64         
#define WAVE_END_Y 128          
#define WAVE_HEIGHT 64          
#define WAVE_BASELINE 96        // 只保留这一个定义
void draw_coordinate_grid(void);
void clear_wave_column(u16 x_pos);
    
// 针对ECG数据范围100000-200000调整参数
#define ECG_DATA_CENTER 135000  // ECG数据中心值
#define ECG_DATA_SCALE 0.0018f  // 缩放因子：(ECG_range/2) / (WAVE_HEIGHT/2) = 50000/32 ≈ 0.0006
    
// 坐标网格参数
#define GRID_COLOR GRAYBLUE     // 网格颜色
#define AXIS_COLOR BLACK        // 坐标轴颜色
#define GRID_STEP_X 20          // X轴网格间距（像素）
#define GRID_STEP_Y 16          // Y轴网格间距（像素）
    
// 用于存储波形数据的环形缓冲区
float32_t ecg_queue_buffer[32]; // 足够大的缓冲区用于队列

// 用于传递ECG数据的邮箱
OS_Q ECGDataQ;

//__________________________________________________________________________________________________________________________________
// 添加看门狗相关定义
#define WATCHDOG_TIMEOUT_MS     15000   // 增加到15秒超时
#define WATCHDOG_FEED_PERIOD    5000    // 喂狗周期改为5秒
#define WATCHDOG_CHECK_PERIOD   5       // 看门狗检测周期5秒

// 看门狗任务定义
#define WATCHDOG_TASK_PRIO      2       // 最高优先级（除了中断任务）
#define WATCHDOG_STK_SIZE       512     // 任务堆栈大小
OS_TCB WatchdogTaskTCB;                 // 任务控制块
CPU_STK WATCHDOG_TASK_STK[WATCHDOG_STK_SIZE]; // 任务堆栈
void watchdog_task(void *p_arg);        // 任务函数声明

// 看门狗定时器和计数器
OS_TMR WatchdogTimer;                   // uC/OS定时器
volatile u32 watchdog_counter;          // 看门狗计数器
volatile u8 system_alive_flag;          // 系统活跃标志

// 各任务的喂狗标志
typedef struct {
    u8 ecg_task_alive;      // ECG任务活跃标志
    u8 disp_task_alive;     // 显示任务活跃标志
    u8 start_task_alive;    // 启动任务活跃标志
} task_watchdog_t;

volatile task_watchdog_t task_watchdog = {0, 0, 0};

// 看门狗定时器回调函数
void watchdog_timer_callback(void *p_tmr, void *p_arg);

// 喂狗函数
void feed_watchdog_from_task(u8 task_id);

// 任务ID定义
#define TASK_ID_START   0
#define TASK_ID_ECG     1  
#define TASK_ID_DISP    2

//__________________________________________________________________________________________________________________________________


//__________________________________________________________________________________________________________________________________
//-----------------------------------------------------------------
// 心电数据采集
//-----------------------------------------------------------------
u32 ch1_data;		// 通道1的数据
u32 ch2_data;		// 通道2的数据
u16 point_cnt;	// 两个峰值之间的采集点数，用于计算心率

#define Samples_Number  1    											// 采样点数
#define Block_Size      1     										// 调用一次arm_fir_f32处理的采样点个数
#define NumTaps        	129     									// 滤波器系数个数

uint32_t blockSize = Block_Size;									// 调用一次arm_fir_f32处理的采样点个数
uint32_t numBlocks = Samples_Number/Block_Size;   // 需要调用arm_fir_f32的次数

float32_t Input_data1; 														// 输入缓冲区
float32_t Output_data1;         									// 输出缓冲区
float32_t firState1[Block_Size + NumTaps - 1]; 		// 状态缓存，大小numTaps + blockSize - 1
float32_t Input_data2; 														// 输入缓冲区
float32_t Output_data2;         									// 输出缓冲区
float32_t firState2[Block_Size + NumTaps - 1]; 		// 状态缓存，大小numTaps + blockSize - 1

// 带通滤波器系数：采样频率为250Hz，截止频率为5Hz~40Hz 通过filterDesigner获取
const float32_t BPF_5Hz_40Hz[NumTaps]  = {
  3.523997657e-05,0.0002562592272,0.0005757701583,0.0008397826459, 0.000908970891,
  0.0007304374012,0.0003793779761,4.222582356e-05,-6.521392788e-05,0.0001839015895,
  0.0007320778677, 0.001328663086, 0.001635892317, 0.001413777587,0.0006883906899,
  -0.0002056905651,-0.0007648666506,-0.0005919140531,0.0003351111372, 0.001569915912,
   0.002375603188, 0.002117323689,0.0006689901347,-0.001414557919,-0.003109993879,
  -0.003462586319, -0.00217742566,8.629632794e-05, 0.001947802957, 0.002011778764,
  -0.0002987752669,-0.004264956806, -0.00809297245,-0.009811084718,-0.008411717601,
  -0.004596390296,-0.0006214127061,0.0007985962438,-0.001978532877,-0.008395017125,
   -0.01568987407, -0.02018531598, -0.01929843985, -0.01321159769,-0.005181713495,
  -0.0001112028476,-0.001950757345, -0.01125541423,  -0.0243169684, -0.03460548073,
   -0.03605531529, -0.02662901953, -0.01020727865, 0.004513713531, 0.008002913557,
  -0.004921500571, -0.03125274926, -0.05950148031, -0.07363011688, -0.05986980721,
   -0.01351031102,  0.05752891302,   0.1343045086,   0.1933406889,   0.2154731899,
     0.1933406889,   0.1343045086,  0.05752891302, -0.01351031102, -0.05986980721,
   -0.07363011688, -0.05950148031, -0.03125274926,-0.004921500571, 0.008002913557,
   0.004513713531, -0.01020727865, -0.02662901953, -0.03605531529, -0.03460548073,
    -0.0243169684, -0.01125541423,-0.001950757345,-0.0001112028476,-0.005181713495,
   -0.01321159769, -0.01929843985, -0.02018531598, -0.01568987407,-0.008395017125,
  -0.001978532877,0.0007985962438,-0.0006214127061,-0.004596390296,-0.008411717601,
  -0.009811084718, -0.00809297245,-0.004264956806,-0.0002987752669, 0.002011778764,
   0.001947802957,8.629632794e-05, -0.00217742566,-0.003462586319,-0.003109993879,
  -0.001414557919,0.0006689901347, 0.002117323689, 0.002375603188, 0.001569915912,
  0.0003351111372,-0.0005919140531,-0.0007648666506,-0.0002056905651,0.0006883906899,
   0.001413777587, 0.001635892317, 0.001328663086,0.0007320778677,0.0001839015895,
  -6.521392788e-05,4.222582356e-05,0.0003793779761,0.0007304374012, 0.000908970891,
  0.0008397826459,0.0005757701583,0.0002562592272,3.523997657e-05
	};

// 低通滤波器系数：采样频率为250Hz，截止频率为2Hz 通过filterDesigner获取
const float32_t LPF_2Hz[NumTaps]  = {
  -0.0004293085367,-0.0004170549801,-0.0004080719373,-0.0004015014856,-0.0003963182389,
  -0.000391335343,-0.0003852125083,-0.0003764661378,-0.0003634814057,-0.0003445262846,
  -0.0003177672043,-0.0002812864841,-0.0002331012802,-0.0001711835939,-9.348169988e-05,
  2.057720394e-06,0.0001174666468, 0.000254732382,0.0004157739459,0.0006024184986,
   0.000816378044, 0.001059226575, 0.001332378131,  0.00163706555,  0.00197432097,
   0.002344956854, 0.002749550389, 0.003188427072, 0.003661649302, 0.004169005435,
    0.00471000094, 0.005283853505, 0.005889489781, 0.006525543984, 0.007190360688,
   0.007882000878, 0.008598247543, 0.009336617775,  0.01009437256,  0.01086853724,
    0.01165591553,  0.01245311089,  0.01325654797,  0.01406249963,  0.01486710832,
    0.01566641964,  0.01645640284,  0.01723298989,  0.01799209975,  0.01872966997,
    0.01944169216,  0.02012423798,  0.02077349275,  0.02138578519,  0.02195761539,
     0.0224856846,  0.02296692133,  0.02339850739,  0.02377789468,  0.02410283685,
    0.02437139489,  0.02458196506,  0.02473328263,  0.02482444048,  0.02485488541,
    0.02482444048,  0.02473328263,  0.02458196506,  0.02437139489,  0.02410283685,
    0.02377789468,  0.02339850739,  0.02296692133,   0.0224856846,  0.02195761539,
    0.02138578519,  0.02077349275,  0.02012423798,  0.01944169216,  0.01872966997,
    0.01799209975,  0.01723298989,  0.01645640284,  0.01566641964,  0.01486710832,
    0.01406249963,  0.01325654797,  0.01245311089,  0.01165591553,  0.01086853724,
    0.01009437256, 0.009336617775, 0.008598247543, 0.007882000878, 0.007190360688,
   0.006525543984, 0.005889489781, 0.005283853505,  0.00471000094, 0.004169005435,
   0.003661649302, 0.003188427072, 0.002749550389, 0.002344956854,  0.00197432097,
    0.00163706555, 0.001332378131, 0.001059226575, 0.000816378044,0.0006024184986,
  0.0004157739459, 0.000254732382,0.0001174666468,2.057720394e-06,-9.348169988e-05,
  -0.0001711835939,-0.0002331012802,-0.0002812864841,-0.0003177672043,-0.0003445262846,
  -0.0003634814057,-0.0003764661378,-0.0003852125083,-0.000391335343,-0.0003963182389,
  -0.0004015014856,-0.0004080719373,-0.0004170549801,-0.0004293085367
	};
//__________________________________________________________________________________________________________________________________





//DRDY引脚
void EXTI9_5_IRQHandler(void)
{
	u8 j;
	u8 read_data[9];    
    OS_ERR err;

    OSIntEnter();         // 进入中断
   if(EXTI_GetITStatus(EXTI_Line8) != RESET)
  {
    EXTI_ClearITPendingBit(EXTI_Line8);
    // 原EXTI3_IRQHandler中的代码...
    for (j = 0; j < 9; j++)		// 一次读取9个字节。其中前3个字节包含了电极状态，后面3+3个字节分别表示两个通道的数据
	{
		read_data[j] = SPI3_ReadWriteByte(0xFF);
	}
	ch1_data=0;
	ch2_data=0;
	ch1_data |= (uint32_t)read_data[3] << 16;
	ch1_data |= (uint32_t)read_data[4] << 8;
	ch1_data |= (uint32_t)read_data[5] << 0;
	ch2_data |= (uint32_t)read_data[6] << 16;
	ch2_data |= (uint32_t)read_data[7] << 8;
	ch2_data |= (uint32_t)read_data[8] << 0;
	point_cnt++;
	// 发布信号量，通知ECG任务处理数据
    OSSemPost(&ECGDataSem, OS_OPT_POST_1, &err);
  }
  OSIntExit();  // 退出中断

 }


// 外部中断初始化
void EXTIX_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);  // 使能SYSCFG时钟
    
    // KEY0 - PA0
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


// 外部中断0服务程序
void EXTI0_IRQHandler(void)
{


    OS_ERR err;

    OSIntEnter();         // 进入中断

    if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    OSIntExit();  // 退出中断
}





int main(void)
{

	OS_ERR err;
	CPU_SR_ALLOC();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); //使能PWR时钟
	delay_init(168);  	//时钟初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//中断分组配置
	LED_Init();         //LED初始化
	EXTIX_Init();       //外部中断初始化
    uart_init(115200); //串口初始化

	DBGMCU_Config(DBGMCU_STOP, ENABLE); //调试MCU配置


    LCD_Init();//LCD初始化
	LCD_Fill(0,0,LCD_W,LCD_H,WHITE);//填充背景色

    GPIO_ADX922_Configuration();		// ADX922引脚初始化
	SPI3_Init();										// SPI3初始化
    ADX922_PowerOnInit();						// ADX922上电初始化
    
    // 初始化uC/OS-III
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII

}


//开始任务函数
void start_task(void *p_arg)
{
    OS_ERR err;
    CPU_SR_ALLOC();
    p_arg = p_arg;

    CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
    
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif

#if OS_CFG_APP_HOOKS_EN				//使用钩子函数
    App_OS_SetAllHooks();			
#endif
    
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
     //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
    OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
    
    OS_CRITICAL_ENTER();	//进入临界区

    // 创建信号量
    OSSemCreate(&ECGDataSem,
                "ECG Data Semaphore",
                0,                    // 初始值为0，表示没有数据可处理
                &err);
                
    // 创建邮箱，用于传递心电数据
    OSQCreate(&ECGDataQ,
          "ECG Data Queue",
          32,             // 队列长度
          &err);

    // 创建看门狗定时器
    OSTmrCreate(&WatchdogTimer,
                "Watchdog Timer",
                0,                              // 初始延时
                300,                           // 1秒周期 (假设系统节拍100Hz，即10ms一个节拍)
                OS_OPT_TMR_PERIODIC,           // 周期性定时器
                watchdog_timer_callback,       // 回调函数
                NULL,                          // 回调参数
                &err);
        OSTmrStart(&WatchdogTimer, &err);

    // 创建看门狗任务
    OSTaskCreate((OS_TCB    * )&WatchdogTaskTCB,
                 (CPU_CHAR  * )"watchdog task",
                 (OS_TASK_PTR )watchdog_task,
                 (void      * )0,
                 (OS_PRIO     )WATCHDOG_TASK_PRIO,
                 (CPU_STK   * )&WATCHDOG_TASK_STK[0],
                 (CPU_STK_SIZE)WATCHDOG_STK_SIZE/10,
                 (CPU_STK_SIZE)WATCHDOG_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       * )0,
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR    * )&err);

    //创建心电数据处理任务
    OSTaskCreate((OS_TCB    * )&ECGTaskTCB,         //任务控制块
                 (CPU_CHAR  * )"ecg task",          //任务名字
                 (OS_TASK_PTR )ecg_task,            //任务函数
                 (void      * )0,                   //传递给任务函数的参数
                 (OS_PRIO     )ECG_TASK_PRIO,       //任务优先级
                 (CPU_STK   * )&ECG_TASK_STK[0],    //任务堆栈基地址
                 (CPU_STK_SIZE)ECG_STK_SIZE/10,     //任务堆栈深度限位
                 (CPU_STK_SIZE)ECG_STK_SIZE,        //任务堆栈大小
                 (OS_MSG_QTY  )0,                   //任务内部消息队列能够接收的最大消息数目
                 (OS_TICK     )0,                   //时间片长度
                 (void       * )0,                  //用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR    * )&err);               //存放该函数错误时的返回值
                 
     // 创建显示任务
    OSTaskCreate((OS_TCB    * )&DISPTaskTCB,
             (CPU_CHAR  * )"disp task",
             (OS_TASK_PTR )disp_task,
             (void      * )0,
             (OS_PRIO     )DISP_TASK_PRIO,
             (CPU_STK   * )&DISP_TASK_STK[0],
             (CPU_STK_SIZE)DISP_STK_SIZE/10,
             (CPU_STK_SIZE)DISP_STK_SIZE,
             (OS_MSG_QTY  )10,
             (OS_TICK     )0,
             (void       * )0,
             (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
             (OS_ERR    * )&err);            
    
    // 初始化看门狗相关变量
    watchdog_counter = 0;
    system_alive_flag = 1;
    task_watchdog.ecg_task_alive = 0;
    task_watchdog.disp_task_alive = 0;
    printf("System watchdog initialized\n");
    
    OS_CRITICAL_EXIT();	//退出临界区

}

// 看门狗任务实现 - 只监控ECG和显示任务
void watchdog_task(void *p_arg) 
{
    OS_ERR err;
    u32 error_count = 0;
    
    printf("Watchdog task started - monitoring ECG and DISP tasks only\n");
    
    // 给其他任务更多启动时间
    OSTimeDlyHMSM(0, 0, 5, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    
    while(1) {
        // 只检查ECG和显示任务的活跃状态
        u8 critical_tasks_alive = task_watchdog.ecg_task_alive && 
                                 task_watchdog.disp_task_alive;
        
        if(critical_tasks_alive && system_alive_flag) {
            // 关键任务正常，重置错误计数
            error_count = 0;
            printf("Critical tasks OK - ECG:%d, DISP:%d, SYS:%d\n", 
                   task_watchdog.ecg_task_alive,
                   task_watchdog.disp_task_alive,
                   system_alive_flag);
        } else {
            error_count++;
            printf("Watchdog ERROR - ECG:%d, DISP:%d, SYS:%d, ErrCnt:%d\n", 
                   task_watchdog.ecg_task_alive,
                   task_watchdog.disp_task_alive, 
                   system_alive_flag,
                   error_count);
            
            // 连续3次检测失败才执行复位（降低误触发）
            if(error_count >= 3) {
                printf("CRITICAL TASKS TIMEOUT! System will reset in 3 seconds...\n");
                
                // 闪烁LED警告
                for(int i = 0; i < 6; i++) {
                    LED0 = 0;  // 点亮LED
                    OSTimeDlyHMSM(0, 0, 0, 250, OS_OPT_TIME_HMSM_STRICT, &err);
                    LED0 = 1;  // 熄灭LED
                    OSTimeDlyHMSM(0, 0, 0, 250, OS_OPT_TIME_HMSM_STRICT, &err);
                }
                
                // 执行软件复位
                NVIC_SystemReset();
            }
        }
        
        // 只清除关键任务的活跃标志
        task_watchdog.ecg_task_alive = 0;
        task_watchdog.disp_task_alive = 0; 
        system_alive_flag = 0;
        
        // 检查周期改为3秒，提高响应速度
        OSTimeDlyHMSM(0, 0, 3, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}
// 看门狗定时器回调函数
void watchdog_timer_callback(void *p_tmr, void *p_arg)
{
    // 定时器回调中不要执行复杂操作，只更新计数器
    watchdog_counter++;
    system_alive_flag = 1;  // 标记系统时钟正常
}

void feed_watchdog_from_task(u8 task_id)
{
    switch(task_id) {
        case TASK_ID_ECG:
            task_watchdog.ecg_task_alive = 1;
            break;
        case TASK_ID_DISP:
            task_watchdog.disp_task_alive = 1;
            break;
        default:
            break;
    }
}

void ecg_task(void *p_arg)
{
    arm_fir_instance_f32 S2;  // 只保留S2滤波器
    OS_ERR err;
    
    u32 p_num=0;     // 用于刷新最大值和最小值
    float32_t min[2]={0xFFFFFFFF,0xFFFFFFFF};  // 改为float32_t类型
    float32_t max[2]={0,0};                    // 改为float32_t类型
    float32_t Peak = 0;                        // 峰峰值，改为float32_t
    float32_t BPM_LH[3] = {0};                 // 用于判断波峰的滑动窗口
    float BPM;        // 心率
    
    // 心率检测相关变量
    static u32 sample_count = 0;               // 采样计数器，替代原来的point_cnt+j-Samples_Number
    
    // 只初始化S2滤波器
    arm_fir_init_f32(&S2, NumTaps, (float32_t *)BPF_5Hz_40Hz, firState2, blockSize);
    
    CS_L;
    delay_us(10);
    SPI3_ReadWriteByte(RDATAC);  // 发送启动连续读取数据命令
    delay_us(10);
    CS_H;                      
    START_H;             // 启动转换
    CS_L;
    
    while (1)
    {   
        // 等待信号量，表示有新数据可用
        OSSemPend(&ECGDataSem, 
                 0,             // 无限期等待
                 OS_OPT_PEND_BLOCKING,
                 NULL,          // 不获取时间戳
                 &err);
        static u32 ecg_feed_counter = 0;
        ecg_feed_counter++;
        if(ecg_feed_counter % 100 == 0) {
            feed_watchdog_from_task(TASK_ID_ECG);
        }
        
        // 只处理ch2_data
        Input_data2=(float32_t)(ch2_data^0x800000);
        // 实现FIR滤波
        arm_fir_f32(&S2, &Input_data2, &Output_data2, blockSize);
        
        // 比较大小 - 改为float32_t类型比较
        if(min[1] > Output_data2)
            min[1] = Output_data2;
        if(max[1] < Output_data2)
            max[1] = Output_data2;
            
        // 更新滑动窗口 - 与原代码逻辑一致
        BPM_LH[0] = BPM_LH[1];
        BPM_LH[1] = BPM_LH[2];
        BPM_LH[2] = Output_data2;
        
        // 每隔2000个点重新测量一次最大最小值
        p_num++;
        if(p_num > 2000)
        {
            min[0] = min[1];          
            max[0] = max[1];
            min[1] = 0xFFFFFFFF;
            max[1] = 0;
            Peak = max[0] - min[0];
            p_num = 0;
            
            // 调试输出
            if(Peak > 1000) {
                printf("Updated Peak: %.0f, Max: %.0f, Min: %.0f\n", Peak, max[0], min[0]);
            }
        }
        
        // 心率检测算法 - 基于原代码逻辑
        if(p_num > 100 && Peak > 1000) {  // 确保有足够的初始化时间和有效峰值
            // 检测峰值：三点局部最大值且超过动态阈值
            // 原逻辑：(BPM_LH[0]<BPM_LH[1])&(BPM_LH[1]>max[0]-Peak/3)&(BPM_LH[2]<BPM_LH[1])
            if((BPM_LH[0] < BPM_LH[1]) && 
               (BPM_LH[1] > (max[0] - Peak/3)) && 
               (BPM_LH[2] < BPM_LH[1]))
            {
                // 计算心率 - 基于原代码公式但调整采样频率
                // 原公式：BPM=(float)60000.0/(float)((point_cnt+j-Samples_Number)*4);
                // 调整为250Hz采样频率：BPM = 60*250 / sample_count = 15000 / sample_count
                if(sample_count > 50) {  // 避免除零和过高心率
                    BPM = 15000.0f / (float)sample_count;  // 250Hz采样频率下的心率计算
                    
                    // 心率范围限制 - 参考原代码的BPM<200限制，但更严格
                    if(BPM >= 40.0f && BPM <= 180.0f) {
                        current_BPM = BPM;
                        bmp_updated = 1;
                        printf("Peak detected! Sample_count: %d, Heart Rate: %.3f BPM\n", sample_count, BPM);
                    }
                }
                
                // 重置采样计数器 
                sample_count = 0;
            }
        }
        
        // 增加采样计数器
        sample_count++;
        
        // 长时间无心率检测，重置 - 防止溢出
        if(sample_count > 2500) {  // 10秒无心率检测
            current_BPM = 0.0f;
            bmp_updated = 1;
            sample_count = 0;
            printf("Heart rate detection timeout, resetting...\n");
        }
        
        // 输出处理后的数据 - 减少输出频率
        if(p_num % 50 == 0) {
            printf("ECG: %.0f, Peak: %.0f, Threshold: %.0f\n", Output_data2, Peak, max[0] - Peak/3);
        }
        
        // 发送数据到显示任务
        if(p_num % 2 == 0) {
            static u8 buf_index = 0;
            buf_index = (buf_index + 1) % 32;
            ecg_queue_buffer[buf_index] = Output_data2;
            OSQPost(&ECGDataQ, (void*)&ecg_queue_buffer[buf_index], sizeof(float32_t), OS_OPT_POST_FIFO, &err);
        } 
    }
}


void disp_task(void *p_arg)
{
    OS_ERR err;
    float32_t ecg_data;
    OS_MSG_SIZE msg_size;
    
    // 波形绘制相关变量
    static u16 draw_x = 0;          // 当前绘制的X坐标
    static u16 last_y = 96;         // 上一个点的Y坐标（基线位置：64+64/2=96）
    u16 current_y;                  // 当前点的Y坐标
    u8 str[30];                     // 增大字符串缓冲区
    static u32 point_count = 0;     // 点计数器
    static u32 bmp_display_counter = 0;  // BPM显示更新计数器
    static float last_displayed_bmp = 0;  // 上次显示的BPM值
    
    // 初始化显示区域
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    
    // 显示标题和基本信息
    LCD_ShowString(2, 5, "ECG Monitor", RED, WHITE, 12, 0);

    // 绘制初始坐标网格
    draw_coordinate_grid();
    
    // 初始化draw_x为起始位置+1，避免边界问题
    draw_x = WAVE_START_X + 1;

    // 主循环 - 接收并显示数据
    while (1)
    {
        // 等待队列中的数据 (最多等待100ms)
        void *msg_ptr = OSQPend(&ECGDataQ,
                              100,
                              OS_OPT_PEND_BLOCKING,
                              &msg_size,
                              NULL,
                              &err);
        
        if(err == OS_ERR_NONE) {
            // 成功接收到数据
            ecg_data = *(float32_t*)msg_ptr;
            
            // 将ECG数据转换为屏幕Y坐标
            s16 offset = (s16)((ecg_data - ECG_DATA_CENTER) * ECG_DATA_SCALE);
            current_y = WAVE_BASELINE - offset;
            
            // 限制Y坐标范围在指定区域内
            if(current_y < WAVE_START_Y + 1) current_y = WAVE_START_Y + 1;
            if(current_y > WAVE_END_Y - 2) current_y = WAVE_END_Y - 2;
            
            // 修复清除算法 - 避免清除最左边边界
            u16 clear_offset = 10;  // 提前清除的列数
            u16 clear_x = draw_x + clear_offset;
            
            // 处理循环边界
            if(clear_x >= WAVE_END_X) {
                clear_x = clear_x - (WAVE_END_X - WAVE_START_X - 1) + WAVE_START_X + 1;
            }
            
            // 确保不清除边界线
            if(clear_x > WAVE_START_X && clear_x < WAVE_END_X - 1) {
                // 智能清除函数
                clear_wave_column(clear_x);
            }
            
            // 绘制波形线条
            if(draw_x > WAVE_START_X + 1) {
                LCD_DrawLine(draw_x-1, last_y, draw_x, current_y, BLUE);  // 改为蓝色更清晰
            } else {
                // 第一个点，只画点
                LCD_DrawPoint(draw_x, current_y, BLUE);
            }
            
            // 更新坐标
            last_y = current_y;
            draw_x++;
            point_count++;
            
            // 修复边界重置逻辑 - 避免闪烁
            if(draw_x >= WAVE_END_X - 1) {
                draw_x = WAVE_START_X + 1;  // 重置到起始位置+1
                last_y = WAVE_BASELINE;     // 重置到基线
                
                // 不要在这里重绘边框，避免闪烁
                // 只在必要时重绘边框
                static u32 border_redraw_counter = 0;
                border_redraw_counter++;
                if(border_redraw_counter >= 500) {  // 每500个点重绘一次边框
                    LCD_DrawRectangle(WAVE_START_X, WAVE_START_Y, WAVE_END_X-1, WAVE_END_Y-1, AXIS_COLOR);
                    border_redraw_counter = 0;
                }
            }
        }

        // 显示任务喂狗 - 每25个点更新一次
        if(bmp_display_counter % 25 == 0) {
            feed_watchdog_from_task(TASK_ID_DISP);
        }

        // 更新显示信息 - 降低更新频率
        bmp_display_counter++;
        if(bmp_display_counter % 25 == 0) {  // 每25个点更新一次显示
            // 只有当BPM值改变时才更新显示
            if(current_BPM != last_displayed_bmp || bmp_updated) {
                if(current_BPM > 0) {
                    sprintf((char*)str, "Rate: %.1f BPM     ", current_BPM);
                    LCD_ShowString(2, 20, str, BLUE, WHITE, 12, 0);
                }
                last_displayed_bmp = current_BPM;
                bmp_updated = 0;  // 清除更新标志
            }
            
            // 显示当前ECG数值
            sprintf((char*)str, "Val: %d     ", (s32)(ecg_data - ECG_DATA_CENTER));
            LCD_ShowString(0, 35, str, BLUE, WHITE, 12, 0);
        }
    }
}

// 添加智能清除函数，避免闪烁
void clear_wave_column(u16 x_pos) {
    if(x_pos <= WAVE_START_X || x_pos >= WAVE_END_X - 1) {
        return;  // 不清除边界
    }
    
    for(u16 y = WAVE_START_Y + 1; y < WAVE_END_Y - 1; y++) {
        u8 is_grid_point = 0;
        
        // 检查是否是垂直网格线
        if((x_pos % GRID_STEP_X) == 0 && (y % 4) == 0) {
            LCD_DrawPoint(x_pos, y, GRID_COLOR);
            is_grid_point = 1;
        }
        // 检查是否是水平网格线
        else if((y % GRID_STEP_Y) == 0 && (x_pos % 4) == 0) {
            LCD_DrawPoint(x_pos, y, GRID_COLOR);
            is_grid_point = 1;
        }
        // 检查是否是基线
        else if(y == WAVE_BASELINE && (x_pos % 2) == 0) {
            LCD_DrawPoint(x_pos, y, RED);
            is_grid_point = 1;
        }
        
        // 如果不是网格点，则清除为背景色
        if(!is_grid_point) {
            LCD_DrawPoint(x_pos, y, WHITE);
        }
    }
}

// 优化的网格绘制函数 - 添加更清晰的边界
void draw_coordinate_grid(void) {
    u16 i, x, y;
    u8 str[20];
    
    // 绘制主边框 - 使用双线边框，更清晰
    LCD_DrawRectangle(WAVE_START_X, WAVE_START_Y, WAVE_END_X-1, WAVE_END_Y-1, AXIS_COLOR);
    
    // 绘制垂直网格线（时间轴）
    for(i = WAVE_START_X + GRID_STEP_X; i < WAVE_END_X - 1; i += GRID_STEP_X) {
        // 主刻度线（每1秒）
        for(y = WAVE_START_Y + 2; y < WAVE_END_Y - 2; y += 4) {
            LCD_DrawPoint(i, y, GRID_COLOR);
        }
        
        // 在底部标注时间刻度
        if(i <= WAVE_END_X - 15) {
            sprintf((char*)str, "%ds", (i - WAVE_START_X) / GRID_STEP_X);
            LCD_ShowString(i - 4, WAVE_END_Y + 3, str, AXIS_COLOR, WHITE, 8, 0);
        }
    }
    
    // 绘制水平网格线（幅度轴）
    for(i = WAVE_START_Y + GRID_STEP_Y; i < WAVE_END_Y - 1; i += GRID_STEP_Y) {
        // 主刻度线
        for(x = WAVE_START_X + 2; x < WAVE_END_X - 2; x += 4) {
            LCD_DrawPoint(x, i, GRID_COLOR);
        }
    }
    
    // 绘制加强的基线（0V参考线）
    for(i = WAVE_START_X + 1; i < WAVE_END_X - 1; i += 2) {
        LCD_DrawPoint(i, WAVE_BASELINE, RED);
    }
    
    // 绘制Y轴刻度标签
    LCD_ShowString(WAVE_END_X + 2, WAVE_START_Y + GRID_STEP_Y - 4, "+0.5", AXIS_COLOR, WHITE, 8, 0);
    LCD_ShowString(WAVE_END_X + 2, WAVE_BASELINE - 4, " 0 ", RED, WHITE, 8, 0);
    LCD_ShowString(WAVE_END_X + 2, WAVE_BASELINE + GRID_STEP_Y - 4, "-0.5", AXIS_COLOR, WHITE, 8, 0);
    
    // 添加单位标签
    LCD_ShowString(WAVE_END_X + 2, WAVE_START_Y - 10, "mV", AXIS_COLOR, WHITE, 8, 0);
    LCD_ShowString(WAVE_START_X, WAVE_END_Y + 15, "Time(s)", AXIS_COLOR, WHITE, 8, 0);
}