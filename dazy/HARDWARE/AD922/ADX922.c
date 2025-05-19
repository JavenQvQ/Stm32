//-----------------------------------------------------------------
// 程序描述:
//     ADX922驱动程序
// 作    者: 凌智电子
// 开始日期: 2022-06-21
// 完成日期: 2022-06-21
// 当前版本: V1.0.0
// 历史版本:
// - V1.0.0: (2022-06-21) ADX922驱动
// 调试工具: 凌智STM32核心开发板、LZE_ST_LINK2
// 说    明:
//
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "ADX922.h"	
#include "Delay.h"
#include "spi.h"	


//-----------------------------------------------------------------
// void GPIO_ADX922_Configuration(void)
//-----------------------------------------------------------------
//
// 函数功能: ADX922引脚初始化
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void GPIO_ADX922_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  // 使能IO口时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOG, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // 使能SYSCFG时钟，用于外部中断

  // ADX922_DRDY -> PC8 (配置为输入，带中断)
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;    // 输入模式
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;    // 上拉输入
  GPIO_Init(GPIOC, &GPIO_InitStructure);           // 初始化
  
  // ADX922_PWDN  -> PG6
  // ADX922_START -> PG7
  // ADX922_CS    -> PG8
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;   // 输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // 推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 高速
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;    // 上拉
  GPIO_Init(GPIOG, &GPIO_InitStructure);           // 初始化

  // 配置PC8的外部中断
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);

  // 配置EXTI8线
  EXTI_InitStructure.EXTI_Line = EXTI_Line8;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; // 中断模式
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; // 上升沿触发
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  // 配置NVIC
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; // PC8对应EXTI8，使用EXTI9_5_IRQn通道
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; // 抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; // 子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


//-----------------------------------------------------------------
// void ADX922_Write_Reg(u8 com, u8 data)
//-----------------------------------------------------------------
//
// 函数功能: 对ADX922的内部寄存器进行写操作
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void ADX922_Write_Reg(u8 addr, u8 data)
{
	CS_L;				// 片选拉低
  SPI3_ReadWriteByte(addr);	// 包含命令操作码和寄存器地址
  delay_us(10);
  SPI3_ReadWriteByte(0x00);	// 要读取的寄存器数+1
  delay_us(10);
  SPI3_ReadWriteByte(data);	// 写入的数据
	delay_us(10);
	CS_H;				// 片选置高
}

//-----------------------------------------------------------------
// u8 ADX922_Read_Reg(u8 addr)
//-----------------------------------------------------------------
//
// 函数功能: 对ADX922的内部寄存器进行读操作
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
u8 ADX922_Read_Reg(u8 addr)
{
  u8 Rxdata;
	CS_L;					// 片选拉低
  SPI3_ReadWriteByte(addr); 			// 包含命令操作码和寄存器地址
  delay_us(10);
  SPI3_ReadWriteByte(0x00); 			// 要读取的寄存器数+1
  delay_us(10);
  Rxdata = SPI3_ReadWriteByte(0xFF); 	// 读取的数据
	delay_us(10);
	CS_H;					// 片选置高
  return Rxdata;
}

//-----------------------------------------------------------------
// void ADX922_PowerOnInit(void)
//-----------------------------------------------------------------
//
// 函数功能: ADX922上电复位
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void ADX922_PowerOnInit(void)
{
	u8 device_id;

  START_H;
  CS_H;
  PWDN_L; // 进入掉电模式
  delay_us(1000);
  PWDN_H; // 退出掉电模式
  delay_us(1000);   // 等待稳定
  PWDN_L; // 发出复位脉冲
  delay_us(10);
  PWDN_H;
  delay_us(1000); // 等待稳定，可以开始使用ADX922
	
	START_L;
	CS_L;
	delay_us(10);
  SPI3_ReadWriteByte(SDATAC); // 发送停止连续读取数据命令
	delay_us(10);
	CS_H;
	
	// 获取芯片ID
	device_id = ADX922_Read_Reg(RREG | ID);
	while(device_id != 0xF3)
	{
		device_id = ADX922_Read_Reg(RREG | ID);
		delay_us(1000);
    printf("device_id = %x\r\n错误", device_id);

	}
	delay_us(10);
  ADX922_Write_Reg(WREG | CONFIG2,  	0XE0); 	// 使用内部参考电压
  delay_ms(10);                            	// 等待内部参考电压稳定
  ADX922_Write_Reg(WREG | CONFIG1,  	0X01); 	// 设置转换速率为250SPS
  delay_us(10);
  ADX922_Write_Reg(WREG | LOFF,     	0XF0);	// 该寄存器配置引出检测操作
  delay_us(10);
  ADX922_Write_Reg(WREG | CH1SET,   	0X00); 	// 增益6，连接到电极
  delay_us(10);
  ADX922_Write_Reg(WREG | CH2SET,   	0X00); 	// 增益6，连接到电极
  delay_us(10);
  ADX922_Write_Reg(WREG | RLD_SENS, 	0x2F);// 选择右腿驱动增益
  delay_us(10);
  ADX922_Write_Reg(WREG | LOFF_SENS,	0x40);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_STAT,	0x00);
  delay_us(10);
  ADX922_Write_Reg(WREG | RESP1,    	0x02);
  delay_us(10);
  ADX922_Write_Reg(WREG | RESP2,    	0x03);
  delay_us(10);
  ADX922_Write_Reg(WREG | GPIO,     	0x0C);
  delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG3,  	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG4,  	0x03);
  delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG5,  	0x70);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_ISTEP,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_RLD,		0x0F);
  delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_AC1,		0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | LON_CFG,		0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | ERM_CFG,		0x40);
  delay_us(10);
	ADX922_Write_Reg(WREG | EPMIX_CFG,	0x14);
  delay_us(10);
	ADX922_Write_Reg(WREG | PACE_CFG1,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG1,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT1,	0x00);
  delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT2,	0xC0);
  delay_us(10);
	ADX922_Write_Reg(WREG | OP_STAT_CMD,0x27);
  delay_us(10);
	ADX922_Write_Reg(WREG | DEV_CONFG1,	0x38);
  delay_us(10);
	
}

//-----------------------------------------------------------------
// u8 ADX922_Read_Data(u8 addr)
//-----------------------------------------------------------------
//
// 函数功能: 读取ADX922的数据
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void ADX922_Read_Data(u8 *data)
{
  u8 i;
  CS_L;
  delay_us(10);
  SPI3_ReadWriteByte(RDATAC);    // 发送启动连续读取数据命令
  delay_us(10);
  CS_H;                          
  START_H;                // 启动转换
  while (DRDY == 1);      // 修改为使用DRDY宏，等待DRDY信号拉低
  CS_L;
  delay_us(10);
  for (i = 0; i < 9; i++)        // 连续读取9个数据
  {
    *data = SPI3_ReadWriteByte(0xFF);
    data++;
  }
  START_L;                // 停止转换
  SPI3_ReadWriteByte(SDATAC);    // 发送停止连续读取数据命令
  delay_us(10);
  CS_H;
}
//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
