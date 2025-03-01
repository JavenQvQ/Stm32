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
#include "Serial.h"
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

  // 使能IO口时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// ADX922_DRDY -> PA3
//	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;					// 配置ADX922_DRDY
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		// 高速
//  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;			// 上拉输入
//  GPIO_Init(GPIOA, &GPIO_InitStructure);							// 初始化
  
	// ADX922_PWDN  -> PA0
	// ADX922_START -> PA1
  // ADX922_CS  	 -> PA2
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		// 高速
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;		// 推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);							// 初始化
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
  SPI1_ReadWriteByte(addr);	// 包含命令操作码和寄存器地址
 Delay_us(10);
  SPI1_ReadWriteByte(0x00);	// 要读取的寄存器数+1
 Delay_us(10);
  SPI1_ReadWriteByte(data);	// 写入的数据
	Delay_us(10);
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
  SPI1_ReadWriteByte(addr); 			// 包含命令操作码和寄存器地址
 Delay_us(10);
  SPI1_ReadWriteByte(0x00); 			// 要读取的寄存器数+1
 Delay_us(10);
  Rxdata = SPI1_ReadWriteByte(0xFF); 	// 读取的数据
	Delay_us(10);
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
  Delay_ms(20);
  PWDN_H; // 退出掉电模式
  Delay_ms(20);   // 等待稳定
  PWDN_L; // 发出复位脉冲
 Delay_us(10);
  PWDN_H;
  Delay_ms(20); // 等待稳定，可以开始使用ADX922
	
	START_L;
	CS_L;
	Delay_us(10);
  SPI1_ReadWriteByte(SDATAC); // 发送停止连续读取数据命令
	Delay_us(10);
	CS_H;
	
	// 获取芯片ID
	device_id = ADX922_Read_Reg(RREG | ID);
	while(device_id != 0xF3)
	{
		printf("ERROR ID:%02x\r\n",device_id);
		device_id = ADX922_Read_Reg(RREG | ID);
		Delay_ms(20);
	}
	
	Delay_us(10);
  ADX922_Write_Reg(WREG | CONFIG2,  	0XE0); 	// 使用内部参考电压
  Delay_ms(10);                            	// 等待内部参考电压稳定
  ADX922_Write_Reg(WREG | CONFIG1,  	0X01); 	// 设置转换速率为250SPS
 Delay_us(10);
  ADX922_Write_Reg(WREG | LOFF,     	0XF0);	// 该寄存器配置引出检测操作
 Delay_us(10);
  ADX922_Write_Reg(WREG | CH1SET,   	0X00); 	// 增益6，连接到电极
 Delay_us(10);
  ADX922_Write_Reg(WREG | CH2SET,   	0X00); 	// 增益6，连接到电极
 Delay_us(10);
  ADX922_Write_Reg(WREG | RLD_SENS, 	0x2F);
 Delay_us(10);
  ADX922_Write_Reg(WREG | LOFF_SENS,	0x40);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_STAT,	0x00);
 Delay_us(10);
  ADX922_Write_Reg(WREG | RESP1,    	0x02);
 Delay_us(10);
  ADX922_Write_Reg(WREG | RESP2,    	0x03);
 Delay_us(10);
  ADX922_Write_Reg(WREG | GPIO,     	0x0C);
 Delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG3,  	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG4,  	0x03);
 Delay_us(10);
	ADX922_Write_Reg(WREG | CONFIG5,  	0x70);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_ISTEP,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_RLD,		0x0F);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LOFF_AC1,		0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | LON_CFG,		0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | ERM_CFG,		0x40);
 Delay_us(10);
	ADX922_Write_Reg(WREG | EPMIX_CFG,	0x14);
 Delay_us(10);
	ADX922_Write_Reg(WREG | PACE_CFG1,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG1,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | FIFO_CFG2,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT1,	0x00);
 Delay_us(10);
	ADX922_Write_Reg(WREG | MOD_STAT2,	0xC0);
 Delay_us(10);
	ADX922_Write_Reg(WREG | OP_STAT_CMD,0x27);
 Delay_us(10);
	ADX922_Write_Reg(WREG | DEV_CONFG1,	0x38);
 Delay_us(10);
	
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
	Delay_us(10);
  SPI1_ReadWriteByte(RDATAC);		// 发送启动连续读取数据命令
 Delay_us(10);
	CS_H;						
  START_H; 				// 启动转换
  while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3) == 1);	// 等待DRDY信号拉低
  CS_L;
	Delay_us(10);
  for (i = 0; i < 9; i++)		// 连续读取9个数据
  {
    *data = SPI1_ReadWriteByte(0xFF);
    data++;
  }
  START_L;				// 停止转换
  SPI1_ReadWriteByte(SDATAC);		// 发送停止连续读取数据命令
	Delay_us(10);
	CS_H;
}
//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
