/*woshinidie*/
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <math.h>
#include "bsp_i2c.h"
#include "TIM.h"
#include "Serial.h"
#include "arm_math.h"
#include "ADXL355.h"
#include "MySPI.h"
#include "DHT11.h"
#include "esp8266.h"
#include "ADX922.h"	
#include "spi.h"
#include "led.h"
#include "EXTInterrupt.h"

//-----------------------------------------------------------------
// 心电数据采集
//-----------------------------------------------------------------
u32 ch1_data;		// 通道1的数据
u32 ch2_data;		// 通道2的数据
u8 flog;				// 触发中断标志位
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




    int main(void)
    {
        // 删除重复定义的变量，使用全局变量
        arm_fir_instance_f32 S1;
        arm_fir_instance_f32 S2;
        
        u32 p_num=0;	  	// 用于刷新最大值和最小值
        u32 min[2]={0xFFFFFFFF,0xFFFFFFFF};
        u32 max[2]={0,0};
        u32 Peak;					// 峰峰值
        u32 BPM_LH[3];		// 用于判断波峰
        float BPM;				// 心率        
        flog=0;

        Delay_ms(1000);
        Serial_Init();						// 串口初始化
        GPIO_ADX922_Configuration();		// ADX922引脚初始化
        EXTInterrupt_Init(); 				// 外部中断初始化
        SPI1_Init();						// SPI1初始化
        ADX922_PowerOnInit();				// ADX922上电初始化
        
        // 初始化结构体S1
        arm_fir_init_f32(&S1, NumTaps, (float32_t *)LPF_2Hz, firState1, blockSize);
        // 初始化结构体S2
        arm_fir_init_f32(&S2, NumTaps, (float32_t *)BPF_5Hz_40Hz, firState2, blockSize);
        
        CS_L;
        Delay_us(10);
        SPI1_ReadWriteByte(RDATAC);		// 发送启动连续读取数据命令
        Delay_us(10);
        CS_H;						
        START_H; 				// 启动转换
        CS_L;
        
        // 初始测试串口是否工作
        printf("System initialized!\r\n");
        Delay_ms(500);
        
        while (1)
        {	
            
            if(flog==1)
            {
                Input_data1=(float32_t)(ch1_data^0x800000);
                // 实现FIR滤波
                arm_fir_f32(&S1, &Input_data1, &Output_data1, blockSize);
                Input_data2=(float32_t)(ch2_data^0x800000);
                // 实现FIR滤波
                arm_fir_f32(&S2, &Input_data2, &Output_data2, blockSize);
                // 比较大小
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
                
                // 每隔2000个点重新测量一次最大最小值
                p_num++; // 修正多余的分号
                if(p_num>2000)
                {
                    min[0]=min[1];			
                    max[0]=max[1];
                    min[1]=0xFFFFFFFF;
                    max[1]=0;
                    Peak=max[0]-min[0];
                    p_num=0;
                }
                // 数据：呼吸波、心电信号、心率
                //printf("B: %8d",(u32)Output_data1);
                printf("%8d\n",((u32)Output_data2));
                //printf("C: %6.2f",(BPM));
                flog=0;
            }
        }
    }



    void EXTI3_IRQHandler(void)
    {
        u8 j;
        u8 read_data[9];
        
        EXTI_ClearITPendingBit(EXTI_Line3);
        for (j = 0; j < 9; j++)		// 连续读取9个数据
        {
            read_data[j] = SPI1_ReadWriteByte(0xFF);
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
        flog=1;
    }