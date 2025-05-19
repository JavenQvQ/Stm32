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





//-----------------------------------------------------------------
// �ĵ����ݲɼ�
//-----------------------------------------------------------------
u32 ch1_data;		// ͨ��1������
u32 ch2_data;		// ͨ��2������
u8 flog;				// �����жϱ�־λ
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
	SPI3_Init();										// SPI1��ʼ��
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
	
	
	OS_CRITICAL_EXIT();	//�˳��ٽ���


}


void ecg_task(void *p_arg)
{
    arm_fir_instance_f32 S1;
	arm_fir_instance_f32 S2;
    OS_ERR err;
	
	u32 p_num=0;	  	// ����ˢ�����ֵ����Сֵ
	u32 min[2]={0xFFFFFFFF,0xFFFFFFFF};
	u32 max[2]={0,0};
	u32 Peak;					// ���ֵ
	u32 BPM_LH[3];		// �����жϲ���
	float BPM;				// ����
	
	flog=0;
	
	// ��ʼ���ṹ��S1
	arm_fir_init_f32(&S1, NumTaps, (float32_t *)LPF_2Hz, firState1, blockSize);
	// ��ʼ���ṹ��S2
	arm_fir_init_f32(&S2, NumTaps, (float32_t *)BPF_5Hz_40Hz, firState2, blockSize);
	
	CS_L;
	delay_us(10);
    SPI3_ReadWriteByte(RDATAC);		// ��������������ȡ��������
    delay_us(10);
	CS_H;						
    START_H; 				// ����ת��
	CS_L;
     while (1)
    {	
        // �ȴ��ź�������ʾ�������ݿ���
        OSSemPend(&ECGDataSem, 
                 0,             // �����ڵȴ�
                 OS_OPT_PEND_BLOCKING,
                 NULL,          // ����ȡʱ���
                 &err);

		Input_data1=(float32_t)(ch1_data^0x800000);
		// ʵ��FIR�˲�
		arm_fir_f32(&S1, &Input_data1, &Output_data1, blockSize);
		Input_data2=(float32_t)(ch2_data^0x800000);
		// ʵ��FIR�˲�
		arm_fir_f32(&S2, &Input_data2, &Output_data2, blockSize);
		// �Ƚϴ�С
		if(min[1]>Output_data2)
			min[1]=Output_data2;
		if(max[1]<Output_data2)
			max[1]=Output_data2;
			
		BPM_LH[0]=BPM_LH[1];
		BPM_LH[1]=BPM_LH[2];
		BPM_LH[2]=Output_data2;
		if((BPM_LH[0]<BPM_LH[1])&(BPM_LH[1]>max[0]-Peak/3)&(BPM_LH[2]<BPM_LH[1]))
		{
			BPM=(float)60000.0/(point_cnt*4);
			point_cnt=0;
		}
			
		// ÿ��2000�������²���һ�������Сֵ
		p_num++;
		if(p_num>2000)
		{
			min[0]=min[1];			
			max[0]=max[1];
			min[1]=0xFFFFFFFF;
			max[1]=0;
			Peak=max[0]-min[0];
			p_num=0;
		}
		// ���ݣ����������ĵ��źš�����
		//printf("B: %8d",(u32)Output_data1);
		printf("A: %8d\n",((u32)Output_data2));
		//printf("C: %6.2f",(BPM));

     }
}

