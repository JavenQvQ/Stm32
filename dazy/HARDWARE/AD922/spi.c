
//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------
#include "spi.h"	

SPI_InitTypeDef  SPI_InitStructure;
SPI_InitTypeDef  SPI_InitStructure;

//-----------------------------------------------------------------
// void SPI3_Init(void)
//-----------------------------------------------------------------
//
// 函数功能: SPI3初始化
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void SPI3_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
  
    // 使能SPI3和GPIOC时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
 
    // PC10->ADX922_SCLK, PC11->ADX922_DOUT, PC12->ADX922_DIN
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;      // 复用功能 
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 高速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      // 上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);            // 初始化

    // 引脚复用映射
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3); // SCK
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3); // MISO
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3); // MOSI

    // 引脚初始电平设置
    GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // 设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                       // 设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                   // 设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                          // 选择了串行时钟的稳态:时钟拉低
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                        // 数据捕获于第二个时钟沿
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                           // NSS信号由软件控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; // 定义波特率预分频的值:波特率预分频值为32
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                  // 指定数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial = 7;                            // CRC值计算的多项式
    SPI_Init(SPI3, &SPI_InitStructure);                                 // 根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器
 
    SPI_Cmd(SPI3, ENABLE); // 使能SPI3外设
}   


//-----------------------------------------------------------------
// u8 SPI3_ReadWriteByte(u8 TxData)
//-----------------------------------------------------------------
//
// 函数功能: SPI3 读写一个字节
// 入口参数: 要写入的字节
// 返 回 值: 读取到的字节
// 注意事项: 无
//
//-----------------------------------------------------------------
u8 SPI3_ReadWriteByte(u8 TxData)
{		
    u16 retry=0;	
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
        {
        retry++;
        if(retry>20000)return 0;
        }			  
    SPI_I2S_SendData(SPI3, TxData); //通过外设SPIx发送一个数据		
    retry=0;	
        
    while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET)//检查指定的SPI标志位设置与否:接受缓存非空标志位
        {
        retry++;
        if(retry>20000)return 0;
        }	  	
    return SPI_I2S_ReceiveData(SPI3); //返回通过SPIx最近接收的数据				
}


