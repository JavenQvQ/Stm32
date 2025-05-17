#include "lcd_init.h"
#include "delay.h"


void LCD_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef SPI_InitStructure;
    
  // 使能相关时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOE, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
 
  // 配置SPI1的SCK和MOSI引脚(PA5,PA7)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     // 复用功能
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // 推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // 引脚复用映射
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1); // SCK
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1); // MOSI
  
  // 配置LCD控制引脚: RES(PD4), DC(PD15), CS(PD1)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;    // 普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   // 推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;     // 上拉
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  // 配置LCD背光引脚: BLK(PE8)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
    // 设置控制引脚默认状态
  LCD_CS_Set();
  LCD_DC_Set();
  LCD_RES_Set();
  LCD_BLK_Set();
  
  // SPI1配置
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;      // 单线发送模式
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                  // 主SPI
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;              // 8位数据
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;   // 空闲时钟为低电平
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; // 第一个边沿采样(上升沿)
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                      // NSS由软件控制
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 预分频4，根据需要调整速度
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;             // 高位先发
  SPI_InitStructure.SPI_CRCPolynomial = 7;                       // CRC多项式
  SPI_Init(SPI1, &SPI_InitStructure);
  
  SPI_Cmd(SPI1, ENABLE); // 使能SPI1
}

/******************************************************************************
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat) 
{	
  LCD_CS_Clr();  // 片选拉低
  
  // 等待发送区空
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
  
  // 发送数据
  SPI_I2S_SendData(SPI1, dat);
  
  // 等待传输完成
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
  
  LCD_CS_Set();  // 片选拉高
}

// 其余函数保持不变
/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
  LCD_Writ_Bus(dat);
}

/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
  LCD_Writ_Bus(dat>>8);
  LCD_Writ_Bus(dat);
}

/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
  LCD_DC_Clr();  // 写命令
  LCD_Writ_Bus(dat);
  LCD_DC_Set();  // 写数据
}


/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	if(USE_HORIZONTAL==0)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+2);
		LCD_WR_DATA(x2+2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+1);
		LCD_WR_DATA(y2+1);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+2);
		LCD_WR_DATA(x2+2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+1);
		LCD_WR_DATA(y2+1);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+1);
		LCD_WR_DATA(x2+1);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+2);
		LCD_WR_DATA(y2+2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+1);
		LCD_WR_DATA(x2+1);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+2);
		LCD_WR_DATA(y2+2);
		LCD_WR_REG(0x2c);//储存器写
	}
}

void LCD_Init(void)
{
	LCD_GPIO_Init();//初始化GPIO
	
	LCD_RES_Clr();//复位
	delay_ms(100);
	LCD_RES_Set();
	delay_ms(100);
	
	LCD_BLK_Set();//打开背光
  delay_ms(100);
	
	//************* Start Initial Sequence **********//
	LCD_WR_REG(0x11); //Sleep out 
	delay_ms(120);              //Delay 120ms 
	//------------------------------------ST7735S Frame Rate-----------------------------------------// 
	LCD_WR_REG(0xB1); 
	LCD_WR_DATA8(0x05); 
	LCD_WR_DATA8(0x3C); 
	LCD_WR_DATA8(0x3C); 
	LCD_WR_REG(0xB2); 
	LCD_WR_DATA8(0x05);
	LCD_WR_DATA8(0x3C); 
	LCD_WR_DATA8(0x3C); 
	LCD_WR_REG(0xB3); 
	LCD_WR_DATA8(0x05); 
	LCD_WR_DATA8(0x3C); 
	LCD_WR_DATA8(0x3C); 
	LCD_WR_DATA8(0x05); 
	LCD_WR_DATA8(0x3C); 
	LCD_WR_DATA8(0x3C); 
	//------------------------------------End ST7735S Frame Rate---------------------------------// 
	LCD_WR_REG(0xB4); //Dot inversion 
	LCD_WR_DATA8(0x03); 
	//------------------------------------ST7735S Power Sequence---------------------------------// 
	LCD_WR_REG(0xC0); 
	LCD_WR_DATA8(0x28); 
	LCD_WR_DATA8(0x08); 
	LCD_WR_DATA8(0x04); 
	LCD_WR_REG(0xC1); 
	LCD_WR_DATA8(0XC0); 
	LCD_WR_REG(0xC2); 
	LCD_WR_DATA8(0x0D); 
	LCD_WR_DATA8(0x00); 
	LCD_WR_REG(0xC3); 
	LCD_WR_DATA8(0x8D); 
	LCD_WR_DATA8(0x2A); 
	LCD_WR_REG(0xC4); 
	LCD_WR_DATA8(0x8D); 
	LCD_WR_DATA8(0xEE); 
	//---------------------------------End ST7735S Power Sequence-------------------------------------// 
	LCD_WR_REG(0xC5); //VCOM 
	LCD_WR_DATA8(0x1A); 
	LCD_WR_REG(0x36); //MX, MY, RGB mode 
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
	else LCD_WR_DATA8(0xA0); 
	//------------------------------------ST7735S Gamma Sequence---------------------------------// 
	LCD_WR_REG(0xE0); 
	LCD_WR_DATA8(0x04); 
	LCD_WR_DATA8(0x22); 
	LCD_WR_DATA8(0x07); 
	LCD_WR_DATA8(0x0A); 
	LCD_WR_DATA8(0x2E); 
	LCD_WR_DATA8(0x30); 
	LCD_WR_DATA8(0x25); 
	LCD_WR_DATA8(0x2A); 
	LCD_WR_DATA8(0x28); 
	LCD_WR_DATA8(0x26); 
	LCD_WR_DATA8(0x2E); 
	LCD_WR_DATA8(0x3A); 
	LCD_WR_DATA8(0x00); 
	LCD_WR_DATA8(0x01); 
	LCD_WR_DATA8(0x03); 
	LCD_WR_DATA8(0x13); 
	LCD_WR_REG(0xE1); 
	LCD_WR_DATA8(0x04); 
	LCD_WR_DATA8(0x16); 
	LCD_WR_DATA8(0x06); 
	LCD_WR_DATA8(0x0D); 
	LCD_WR_DATA8(0x2D); 
	LCD_WR_DATA8(0x26); 
	LCD_WR_DATA8(0x23); 
	LCD_WR_DATA8(0x27); 
	LCD_WR_DATA8(0x27); 
	LCD_WR_DATA8(0x25); 
	LCD_WR_DATA8(0x2D); 
	LCD_WR_DATA8(0x3B); 
	LCD_WR_DATA8(0x00); 
	LCD_WR_DATA8(0x01); 
	LCD_WR_DATA8(0x04); 
	LCD_WR_DATA8(0x13); 
	//------------------------------------End ST7735S Gamma Sequence-----------------------------// 
	LCD_WR_REG(0x3A); //65k mode 
	LCD_WR_DATA8(0x05); 
	LCD_WR_REG(0x29); //Display on 
} 








