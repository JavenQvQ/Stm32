#include "stm32f10x.h"  
#include "MySPI.h"
//软件初始化SPI

/*
写片选,1为不选中,0为选中
*/
void MySPI_W_SS(uint8_t x)
{
    if(x)
        GPIO_SetBits(GPIOA, GPIO_Pin_4);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_4);
}
/*
写时钟,1为高电平,0为低电平
*/
void MySPI_W_SCK(uint8_t x)
{
    if(x)
        GPIO_SetBits(GPIOA, GPIO_Pin_5);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_5);
}
/*
写数据,1为高电平,0为低电平
*/
void MySPI_W_MOSI(uint8_t x)
{
    if(x)
        GPIO_SetBits(GPIOA, GPIO_Pin_7);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
}
/*
读数据
*/
uint8_t MySPI_R_MISO(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);
}


void MySPI_Software_Init(void)//软件初始化SPI
{
    //初始化SPI端口
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//输入上拉
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //初始化SPI
    MySPI_W_SS(1);//片选拉高
    MySPI_W_SCK(0);//时钟线拉低

}

void MySPI_Start(void)
{
    MySPI_W_SS(0);
}
void MySPI_Stop(void)
{
    MySPI_W_SS(1);
}
/*
软件SPI交换一个字节,模式1
*/
uint8_t MySPI_Software_SwapByte(uint8_t Byte)
{
    uint8_t i;
    for(i = 0; i < 8; i++)//交换一个字节
    {
        MySPI_W_SCK(1);
        MySPI_W_MOSI(Byte & 0x80 );
        Byte <<= 1;//左移一位
        MySPI_W_SCK(0);
        //读取数据,并存入Byte的最低位,即Byte的最低位为MISO的值,下一次循环左移一位,接收下一个数据
        if(MySPI_R_MISO()==1)
            Byte |= 0x01;
    }
    return Byte;
}





//硬件SPI
void MySPI_Hardware_Init(void)
{
    //初始化SPI端口
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_7|GPIO_Pin_4;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//输入上拉
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //初始化SPI
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;//8位数据帧格式
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;//第二个时钟沿采样数据
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;//软件控制片选
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;//高位在前
    SPI_InitStructure.SPI_CRCPolynomial = 7;//CRC多项式
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
    MySPI_W_SS(1);//片选拉高
}
uint8_t MySPI_Hardware_SwapByte(uint8_t Byte)
{
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, Byte);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}
