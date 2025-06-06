#define __FPU_PRESENT 1
#define __FPU_USED 1
#include "arm_math.h"  // CMSIS DSP�����ͷ�ļ�
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

volatile float current_BPM = 0.0f;  // ��ǰ����ֵ
volatile u8 bmp_updated = 0;        // BPM���±�־
//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		1024
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);


//�ĵ����ݴ�������
#define ECG_TASK_PRIO     3      // �������ȼ�
#define ECG_STK_SIZE      2048    // �����ջ��С
OS_TCB ECGTaskTCB;                // ������ƿ�
CPU_STK ECG_TASK_STK[ECG_STK_SIZE]; // �����ջ
void ecg_task(void *p_arg);       // ����������
OS_SEM ECGDataSem;    // �ĵ����ݴ����ź���


//LCD��ʾ����
#define DISP_TASK_PRIO     4      // �������ȼ�����ECG�����һ��
#define DISP_STK_SIZE      1024   // �����ջ��С
OS_TCB DISPTaskTCB;               // ������ƿ�
CPU_STK DISP_TASK_STK[DISP_STK_SIZE]; // �����ջ
void disp_task(void *p_arg);      // ����������

#define WAVE_START_X 0          
#define WAVE_END_X 160          
#define WAVE_START_Y 64         
#define WAVE_END_Y 128          
#define WAVE_HEIGHT 64          
#define WAVE_BASELINE 96        // ֻ������һ������
void draw_coordinate_grid(void);
void clear_wave_column(u16 x_pos);
    
// ���ECG���ݷ�Χ100000-200000��������
#define ECG_DATA_CENTER 135000  // ECG��������ֵ
#define ECG_DATA_SCALE 0.0018f  // �������ӣ�(ECG_range/2) / (WAVE_HEIGHT/2) = 50000/32 �� 0.0006
    
// �����������
#define GRID_COLOR GRAYBLUE     // ������ɫ
#define AXIS_COLOR BLACK        // ��������ɫ
#define GRID_STEP_X 20          // X�������ࣨ���أ�
#define GRID_STEP_Y 16          // Y�������ࣨ���أ�
    
// ���ڴ洢�������ݵĻ��λ�����
float32_t ecg_queue_buffer[32]; // �㹻��Ļ��������ڶ���

// ���ڴ���ECG���ݵ�����
OS_Q ECGDataQ;

//__________________________________________________________________________________________________________________________________
// ��ӿ��Ź���ض���
#define WATCHDOG_TIMEOUT_MS     15000   // ���ӵ�15�볬ʱ
#define WATCHDOG_FEED_PERIOD    5000    // ι�����ڸ�Ϊ5��
#define WATCHDOG_CHECK_PERIOD   5       // ���Ź��������5��

// ���Ź�������
#define WATCHDOG_TASK_PRIO      2       // ������ȼ��������ж�����
#define WATCHDOG_STK_SIZE       512     // �����ջ��С
OS_TCB WatchdogTaskTCB;                 // ������ƿ�
CPU_STK WATCHDOG_TASK_STK[WATCHDOG_STK_SIZE]; // �����ջ
void watchdog_task(void *p_arg);        // ����������

// ���Ź���ʱ���ͼ�����
OS_TMR WatchdogTimer;                   // uC/OS��ʱ��
volatile u32 watchdog_counter;          // ���Ź�������
volatile u8 system_alive_flag;          // ϵͳ��Ծ��־

// �������ι����־
typedef struct {
    u8 ecg_task_alive;      // ECG�����Ծ��־
    u8 disp_task_alive;     // ��ʾ�����Ծ��־
    u8 start_task_alive;    // ���������Ծ��־
} task_watchdog_t;

volatile task_watchdog_t task_watchdog = {0, 0, 0};

// ���Ź���ʱ���ص�����
void watchdog_timer_callback(void *p_tmr, void *p_arg);

// ι������
void feed_watchdog_from_task(u8 task_id);

// ����ID����
#define TASK_ID_START   0
#define TASK_ID_ECG     1  
#define TASK_ID_DISP    2

//__________________________________________________________________________________________________________________________________


//__________________________________________________________________________________________________________________________________
//-----------------------------------------------------------------
// �ĵ����ݲɼ�
//-----------------------------------------------------------------
u32 ch1_data;		// ͨ��1������
u32 ch2_data;		// ͨ��2������
u16 point_cnt;	// ������ֵ֮��Ĳɼ����������ڼ�������

#define Samples_Number  1    											// ��������
#define Block_Size      1     										// ����һ��arm_fir_f32����Ĳ��������
#define NumTaps        	129     									// �˲���ϵ������

uint32_t blockSize = Block_Size;									// ����һ��arm_fir_f32����Ĳ��������
uint32_t numBlocks = Samples_Number/Block_Size;   // ��Ҫ����arm_fir_f32�Ĵ���

float32_t Input_data1; 														// ���뻺����
float32_t Output_data1;         									// ���������
float32_t firState1[Block_Size + NumTaps - 1]; 		// ״̬���棬��СnumTaps + blockSize - 1
float32_t Input_data2; 														// ���뻺����
float32_t Output_data2;         									// ���������
float32_t firState2[Block_Size + NumTaps - 1]; 		// ״̬���棬��СnumTaps + blockSize - 1

// ��ͨ�˲���ϵ��������Ƶ��Ϊ250Hz����ֹƵ��Ϊ5Hz~40Hz ͨ��filterDesigner��ȡ
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

// ��ͨ�˲���ϵ��������Ƶ��Ϊ250Hz����ֹƵ��Ϊ2Hz ͨ��filterDesigner��ȡ
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





//DRDY����
void EXTI9_5_IRQHandler(void)
{
	u8 j;
	u8 read_data[9];    
    OS_ERR err;

    OSIntEnter();         // �����ж�
   if(EXTI_GetITStatus(EXTI_Line8) != RESET)
  {
    EXTI_ClearITPendingBit(EXTI_Line8);
    // ԭEXTI3_IRQHandler�еĴ���...
    for (j = 0; j < 9; j++)		// һ�ζ�ȡ9���ֽڡ�����ǰ3���ֽڰ����˵缫״̬������3+3���ֽڷֱ��ʾ����ͨ��������
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
	// �����ź�����֪ͨECG����������
    OSSemPost(&ECGDataSem, OS_OPT_POST_1, &err);
  }
  OSIntExit();  // �˳��ж�

 }


// �ⲿ�жϳ�ʼ��
void EXTIX_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);  // ʹ��SYSCFGʱ��
    
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


// �ⲿ�ж�0�������
void EXTI0_IRQHandler(void)
{


    OS_ERR err;

    OSIntEnter();         // �����ж�

    if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    OSIntExit();  // �˳��ж�
}





int main(void)
{

	OS_ERR err;
	CPU_SR_ALLOC();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); //ʹ��PWRʱ��
	delay_init(168);  	//ʱ�ӳ�ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�жϷ�������
	LED_Init();         //LED��ʼ��
	EXTIX_Init();       //�ⲿ�жϳ�ʼ��
    uart_init(115200); //���ڳ�ʼ��

	DBGMCU_Config(DBGMCU_STOP, ENABLE); //����MCU����


    LCD_Init();//LCD��ʼ��
	LCD_Fill(0,0,LCD_W,LCD_H,WHITE);//��䱳��ɫ

    GPIO_ADX922_Configuration();		// ADX922���ų�ʼ��
	SPI3_Init();										// SPI3��ʼ��
    ADX922_PowerOnInit();						// ADX922�ϵ��ʼ��
    
    // ��ʼ��uC/OS-III
	OSInit(&err);		//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();//�����ٽ���
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII

}


//��ʼ������
void start_task(void *p_arg)
{
    OS_ERR err;
    CPU_SR_ALLOC();
    p_arg = p_arg;

    CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
    
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif

#if OS_CFG_APP_HOOKS_EN				//ʹ�ù��Ӻ���
    App_OS_SetAllHooks();			
#endif
    
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
     //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
    OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
    
    OS_CRITICAL_ENTER();	//�����ٽ���

    // �����ź���
    OSSemCreate(&ECGDataSem,
                "ECG Data Semaphore",
                0,                    // ��ʼֵΪ0����ʾû�����ݿɴ���
                &err);
                
    // �������䣬���ڴ����ĵ�����
    OSQCreate(&ECGDataQ,
          "ECG Data Queue",
          32,             // ���г���
          &err);

    // �������Ź���ʱ��
    OSTmrCreate(&WatchdogTimer,
                "Watchdog Timer",
                0,                              // ��ʼ��ʱ
                300,                           // 1������ (����ϵͳ����100Hz����10msһ������)
                OS_OPT_TMR_PERIODIC,           // �����Զ�ʱ��
                watchdog_timer_callback,       // �ص�����
                NULL,                          // �ص�����
                &err);
        OSTmrStart(&WatchdogTimer, &err);

    // �������Ź�����
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

    //�����ĵ����ݴ�������
    OSTaskCreate((OS_TCB    * )&ECGTaskTCB,         //������ƿ�
                 (CPU_CHAR  * )"ecg task",          //��������
                 (OS_TASK_PTR )ecg_task,            //������
                 (void      * )0,                   //���ݸ��������Ĳ���
                 (OS_PRIO     )ECG_TASK_PRIO,       //�������ȼ�
                 (CPU_STK   * )&ECG_TASK_STK[0],    //�����ջ����ַ
                 (CPU_STK_SIZE)ECG_STK_SIZE/10,     //�����ջ�����λ
                 (CPU_STK_SIZE)ECG_STK_SIZE,        //�����ջ��С
                 (OS_MSG_QTY  )0,                   //�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ
                 (OS_TICK     )0,                   //ʱ��Ƭ����
                 (void       * )0,                  //�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR    * )&err);               //��Ÿú�������ʱ�ķ���ֵ
                 
     // ������ʾ����
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
    
    // ��ʼ�����Ź���ر���
    watchdog_counter = 0;
    system_alive_flag = 1;
    task_watchdog.ecg_task_alive = 0;
    task_watchdog.disp_task_alive = 0;
    printf("System watchdog initialized\n");
    
    OS_CRITICAL_EXIT();	//�˳��ٽ���

}

// ���Ź�����ʵ�� - ֻ���ECG����ʾ����
void watchdog_task(void *p_arg) 
{
    OS_ERR err;
    u32 error_count = 0;
    
    printf("Watchdog task started - monitoring ECG and DISP tasks only\n");
    
    // �����������������ʱ��
    OSTimeDlyHMSM(0, 0, 5, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    
    while(1) {
        // ֻ���ECG����ʾ����Ļ�Ծ״̬
        u8 critical_tasks_alive = task_watchdog.ecg_task_alive && 
                                 task_watchdog.disp_task_alive;
        
        if(critical_tasks_alive && system_alive_flag) {
            // �ؼ��������������ô������
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
            
            // ����3�μ��ʧ�ܲ�ִ�и�λ�������󴥷���
            if(error_count >= 3) {
                printf("CRITICAL TASKS TIMEOUT! System will reset in 3 seconds...\n");
                
                // ��˸LED����
                for(int i = 0; i < 6; i++) {
                    LED0 = 0;  // ����LED
                    OSTimeDlyHMSM(0, 0, 0, 250, OS_OPT_TIME_HMSM_STRICT, &err);
                    LED0 = 1;  // Ϩ��LED
                    OSTimeDlyHMSM(0, 0, 0, 250, OS_OPT_TIME_HMSM_STRICT, &err);
                }
                
                // ִ�������λ
                NVIC_SystemReset();
            }
        }
        
        // ֻ����ؼ�����Ļ�Ծ��־
        task_watchdog.ecg_task_alive = 0;
        task_watchdog.disp_task_alive = 0; 
        system_alive_flag = 0;
        
        // ������ڸ�Ϊ3�룬�����Ӧ�ٶ�
        OSTimeDlyHMSM(0, 0, 3, 0, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}
// ���Ź���ʱ���ص�����
void watchdog_timer_callback(void *p_tmr, void *p_arg)
{
    // ��ʱ���ص��в�Ҫִ�и��Ӳ�����ֻ���¼�����
    watchdog_counter++;
    system_alive_flag = 1;  // ���ϵͳʱ������
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
    arm_fir_instance_f32 S2;  // ֻ����S2�˲���
    OS_ERR err;
    
    u32 p_num=0;     // ����ˢ�����ֵ����Сֵ
    float32_t min[2]={0xFFFFFFFF,0xFFFFFFFF};  // ��Ϊfloat32_t����
    float32_t max[2]={0,0};                    // ��Ϊfloat32_t����
    float32_t Peak = 0;                        // ���ֵ����Ϊfloat32_t
    float32_t BPM_LH[3] = {0};                 // �����жϲ���Ļ�������
    float BPM;        // ����
    
    // ���ʼ����ر���
    static u32 sample_count = 0;               // ���������������ԭ����point_cnt+j-Samples_Number
    
    // ֻ��ʼ��S2�˲���
    arm_fir_init_f32(&S2, NumTaps, (float32_t *)BPF_5Hz_40Hz, firState2, blockSize);
    
    CS_L;
    delay_us(10);
    SPI3_ReadWriteByte(RDATAC);  // ��������������ȡ��������
    delay_us(10);
    CS_H;                      
    START_H;             // ����ת��
    CS_L;
    
    while (1)
    {   
        // �ȴ��ź�������ʾ�������ݿ���
        OSSemPend(&ECGDataSem, 
                 0,             // �����ڵȴ�
                 OS_OPT_PEND_BLOCKING,
                 NULL,          // ����ȡʱ���
                 &err);
        static u32 ecg_feed_counter = 0;
        ecg_feed_counter++;
        if(ecg_feed_counter % 100 == 0) {
            feed_watchdog_from_task(TASK_ID_ECG);
        }
        
        // ֻ����ch2_data
        Input_data2=(float32_t)(ch2_data^0x800000);
        // ʵ��FIR�˲�
        arm_fir_f32(&S2, &Input_data2, &Output_data2, blockSize);
        
        // �Ƚϴ�С - ��Ϊfloat32_t���ͱȽ�
        if(min[1] > Output_data2)
            min[1] = Output_data2;
        if(max[1] < Output_data2)
            max[1] = Output_data2;
            
        // ���»������� - ��ԭ�����߼�һ��
        BPM_LH[0] = BPM_LH[1];
        BPM_LH[1] = BPM_LH[2];
        BPM_LH[2] = Output_data2;
        
        // ÿ��2000�������²���һ�������Сֵ
        p_num++;
        if(p_num > 2000)
        {
            min[0] = min[1];          
            max[0] = max[1];
            min[1] = 0xFFFFFFFF;
            max[1] = 0;
            Peak = max[0] - min[0];
            p_num = 0;
            
            // �������
            if(Peak > 1000) {
                printf("Updated Peak: %.0f, Max: %.0f, Min: %.0f\n", Peak, max[0], min[0]);
            }
        }
        
        // ���ʼ���㷨 - ����ԭ�����߼�
        if(p_num > 100 && Peak > 1000) {  // ȷ�����㹻�ĳ�ʼ��ʱ�����Ч��ֵ
            // ����ֵ������ֲ����ֵ�ҳ�����̬��ֵ
            // ԭ�߼���(BPM_LH[0]<BPM_LH[1])&(BPM_LH[1]>max[0]-Peak/3)&(BPM_LH[2]<BPM_LH[1])
            if((BPM_LH[0] < BPM_LH[1]) && 
               (BPM_LH[1] > (max[0] - Peak/3)) && 
               (BPM_LH[2] < BPM_LH[1]))
            {
                // �������� - ����ԭ���빫ʽ����������Ƶ��
                // ԭ��ʽ��BPM=(float)60000.0/(float)((point_cnt+j-Samples_Number)*4);
                // ����Ϊ250Hz����Ƶ�ʣ�BPM = 60*250 / sample_count = 15000 / sample_count
                if(sample_count > 50) {  // �������͹�������
                    BPM = 15000.0f / (float)sample_count;  // 250Hz����Ƶ���µ����ʼ���
                    
                    // ���ʷ�Χ���� - �ο�ԭ�����BPM<200���ƣ������ϸ�
                    if(BPM >= 40.0f && BPM <= 180.0f) {
                        current_BPM = BPM;
                        bmp_updated = 1;
                        printf("Peak detected! Sample_count: %d, Heart Rate: %.3f BPM\n", sample_count, BPM);
                    }
                }
                
                // ���ò��������� 
                sample_count = 0;
            }
        }
        
        // ���Ӳ���������
        sample_count++;
        
        // ��ʱ�������ʼ�⣬���� - ��ֹ���
        if(sample_count > 2500) {  // 10�������ʼ��
            current_BPM = 0.0f;
            bmp_updated = 1;
            sample_count = 0;
            printf("Heart rate detection timeout, resetting...\n");
        }
        
        // ������������� - �������Ƶ��
        if(p_num % 50 == 0) {
            printf("ECG: %.0f, Peak: %.0f, Threshold: %.0f\n", Output_data2, Peak, max[0] - Peak/3);
        }
        
        // �������ݵ���ʾ����
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
    
    // ���λ�����ر���
    static u16 draw_x = 0;          // ��ǰ���Ƶ�X����
    static u16 last_y = 96;         // ��һ�����Y���꣨����λ�ã�64+64/2=96��
    u16 current_y;                  // ��ǰ���Y����
    u8 str[30];                     // �����ַ���������
    static u32 point_count = 0;     // �������
    static u32 bmp_display_counter = 0;  // BPM��ʾ���¼�����
    static float last_displayed_bmp = 0;  // �ϴ���ʾ��BPMֵ
    
    // ��ʼ����ʾ����
    LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
    
    // ��ʾ����ͻ�����Ϣ
    LCD_ShowString(2, 5, "ECG Monitor", RED, WHITE, 12, 0);

    // ���Ƴ�ʼ��������
    draw_coordinate_grid();
    
    // ��ʼ��draw_xΪ��ʼλ��+1������߽�����
    draw_x = WAVE_START_X + 1;

    // ��ѭ�� - ���ղ���ʾ����
    while (1)
    {
        // �ȴ������е����� (���ȴ�100ms)
        void *msg_ptr = OSQPend(&ECGDataQ,
                              100,
                              OS_OPT_PEND_BLOCKING,
                              &msg_size,
                              NULL,
                              &err);
        
        if(err == OS_ERR_NONE) {
            // �ɹ����յ�����
            ecg_data = *(float32_t*)msg_ptr;
            
            // ��ECG����ת��Ϊ��ĻY����
            s16 offset = (s16)((ecg_data - ECG_DATA_CENTER) * ECG_DATA_SCALE);
            current_y = WAVE_BASELINE - offset;
            
            // ����Y���귶Χ��ָ��������
            if(current_y < WAVE_START_Y + 1) current_y = WAVE_START_Y + 1;
            if(current_y > WAVE_END_Y - 2) current_y = WAVE_END_Y - 2;
            
            // �޸�����㷨 - �����������߽߱�
            u16 clear_offset = 10;  // ��ǰ���������
            u16 clear_x = draw_x + clear_offset;
            
            // ����ѭ���߽�
            if(clear_x >= WAVE_END_X) {
                clear_x = clear_x - (WAVE_END_X - WAVE_START_X - 1) + WAVE_START_X + 1;
            }
            
            // ȷ��������߽���
            if(clear_x > WAVE_START_X && clear_x < WAVE_END_X - 1) {
                // �����������
                clear_wave_column(clear_x);
            }
            
            // ���Ʋ�������
            if(draw_x > WAVE_START_X + 1) {
                LCD_DrawLine(draw_x-1, last_y, draw_x, current_y, BLUE);  // ��Ϊ��ɫ������
            } else {
                // ��һ���㣬ֻ����
                LCD_DrawPoint(draw_x, current_y, BLUE);
            }
            
            // ��������
            last_y = current_y;
            draw_x++;
            point_count++;
            
            // �޸��߽������߼� - ������˸
            if(draw_x >= WAVE_END_X - 1) {
                draw_x = WAVE_START_X + 1;  // ���õ���ʼλ��+1
                last_y = WAVE_BASELINE;     // ���õ�����
                
                // ��Ҫ�������ػ�߿򣬱�����˸
                // ֻ�ڱ�Ҫʱ�ػ�߿�
                static u32 border_redraw_counter = 0;
                border_redraw_counter++;
                if(border_redraw_counter >= 500) {  // ÿ500�����ػ�һ�α߿�
                    LCD_DrawRectangle(WAVE_START_X, WAVE_START_Y, WAVE_END_X-1, WAVE_END_Y-1, AXIS_COLOR);
                    border_redraw_counter = 0;
                }
            }
        }

        // ��ʾ����ι�� - ÿ25�������һ��
        if(bmp_display_counter % 25 == 0) {
            feed_watchdog_from_task(TASK_ID_DISP);
        }

        // ������ʾ��Ϣ - ���͸���Ƶ��
        bmp_display_counter++;
        if(bmp_display_counter % 25 == 0) {  // ÿ25�������һ����ʾ
            // ֻ�е�BPMֵ�ı�ʱ�Ÿ�����ʾ
            if(current_BPM != last_displayed_bmp || bmp_updated) {
                if(current_BPM > 0) {
                    sprintf((char*)str, "Rate: %.1f BPM     ", current_BPM);
                    LCD_ShowString(2, 20, str, BLUE, WHITE, 12, 0);
                }
                last_displayed_bmp = current_BPM;
                bmp_updated = 0;  // ������±�־
            }
            
            // ��ʾ��ǰECG��ֵ
            sprintf((char*)str, "Val: %d     ", (s32)(ecg_data - ECG_DATA_CENTER));
            LCD_ShowString(0, 35, str, BLUE, WHITE, 12, 0);
        }
    }
}

// ����������������������˸
void clear_wave_column(u16 x_pos) {
    if(x_pos <= WAVE_START_X || x_pos >= WAVE_END_X - 1) {
        return;  // ������߽�
    }
    
    for(u16 y = WAVE_START_Y + 1; y < WAVE_END_Y - 1; y++) {
        u8 is_grid_point = 0;
        
        // ����Ƿ��Ǵ�ֱ������
        if((x_pos % GRID_STEP_X) == 0 && (y % 4) == 0) {
            LCD_DrawPoint(x_pos, y, GRID_COLOR);
            is_grid_point = 1;
        }
        // ����Ƿ���ˮƽ������
        else if((y % GRID_STEP_Y) == 0 && (x_pos % 4) == 0) {
            LCD_DrawPoint(x_pos, y, GRID_COLOR);
            is_grid_point = 1;
        }
        // ����Ƿ��ǻ���
        else if(y == WAVE_BASELINE && (x_pos % 2) == 0) {
            LCD_DrawPoint(x_pos, y, RED);
            is_grid_point = 1;
        }
        
        // �����������㣬�����Ϊ����ɫ
        if(!is_grid_point) {
            LCD_DrawPoint(x_pos, y, WHITE);
        }
    }
}

// �Ż���������ƺ��� - ��Ӹ������ı߽�
void draw_coordinate_grid(void) {
    u16 i, x, y;
    u8 str[20];
    
    // �������߿� - ʹ��˫�߱߿򣬸�����
    LCD_DrawRectangle(WAVE_START_X, WAVE_START_Y, WAVE_END_X-1, WAVE_END_Y-1, AXIS_COLOR);
    
    // ���ƴ�ֱ�����ߣ�ʱ���ᣩ
    for(i = WAVE_START_X + GRID_STEP_X; i < WAVE_END_X - 1; i += GRID_STEP_X) {
        // ���̶��ߣ�ÿ1�룩
        for(y = WAVE_START_Y + 2; y < WAVE_END_Y - 2; y += 4) {
            LCD_DrawPoint(i, y, GRID_COLOR);
        }
        
        // �ڵײ���עʱ��̶�
        if(i <= WAVE_END_X - 15) {
            sprintf((char*)str, "%ds", (i - WAVE_START_X) / GRID_STEP_X);
            LCD_ShowString(i - 4, WAVE_END_Y + 3, str, AXIS_COLOR, WHITE, 8, 0);
        }
    }
    
    // ����ˮƽ�����ߣ������ᣩ
    for(i = WAVE_START_Y + GRID_STEP_Y; i < WAVE_END_Y - 1; i += GRID_STEP_Y) {
        // ���̶���
        for(x = WAVE_START_X + 2; x < WAVE_END_X - 2; x += 4) {
            LCD_DrawPoint(x, i, GRID_COLOR);
        }
    }
    
    // ���Ƽ�ǿ�Ļ��ߣ�0V�ο��ߣ�
    for(i = WAVE_START_X + 1; i < WAVE_END_X - 1; i += 2) {
        LCD_DrawPoint(i, WAVE_BASELINE, RED);
    }
    
    // ����Y��̶ȱ�ǩ
    LCD_ShowString(WAVE_END_X + 2, WAVE_START_Y + GRID_STEP_Y - 4, "+0.5", AXIS_COLOR, WHITE, 8, 0);
    LCD_ShowString(WAVE_END_X + 2, WAVE_BASELINE - 4, " 0 ", RED, WHITE, 8, 0);
    LCD_ShowString(WAVE_END_X + 2, WAVE_BASELINE + GRID_STEP_Y - 4, "-0.5", AXIS_COLOR, WHITE, 8, 0);
    
    // ��ӵ�λ��ǩ
    LCD_ShowString(WAVE_END_X + 2, WAVE_START_Y - 10, "mV", AXIS_COLOR, WHITE, 8, 0);
    LCD_ShowString(WAVE_START_X, WAVE_END_Y + 15, "Time(s)", AXIS_COLOR, WHITE, 8, 0);
}