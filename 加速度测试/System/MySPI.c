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
    //初始化SPI端口,PA5:SCK,PA6:MISO,PA7:MOSI,PA4:片选
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_7;
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
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;//全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;//8位数据帧格式
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;//第二个时钟沿采样数据
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;//软件控制片选
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;//波特率预分频16
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;//高位在前
    SPI_InitStructure.SPI_CRCPolynomial = 7;//CRC多项式
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
    MySPI_W_SS(1);//片选拉高
}

/*******************************************
 * 函    数：MySPI_Hardware_SwapByte
 * 参    数：Byte:要交换的字节
 * 返 回 值：交换后的字节
 * 描    述：硬件SPI交换一个字节
 * *****************************************/
uint8_t MySPI_Hardware_SwapByte(uint8_t Byte)
{
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);//等待发送缓冲区空
    SPI_I2S_SendData(SPI1, Byte);//发送数据
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);//等待接收缓冲区接收完成
    return SPI_I2S_ReceiveData(SPI1);
}

void MySPI_Hardware_ReadBytes(uint8_t *buf, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        buf[i] = MySPI_Hardware_SwapByte(0xff);
    }
}
void MySPI_Hardware_WriteBytes(uint8_t *buf, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        MySPI_Hardware_SwapByte(buf[i]);
    }
}
/*******************************************
 * 函    数:PB4_ExintSet
 * 参    数:en:1为使能,0为关闭
 * 返 回 值:无
 * 描    述:PB4外部中断设置,下降沿触发可以在主机SPI发送完成后让MISO引脚变为中断引脚,
 *         当从机准备好数据后,拉低MISO引脚,触发中断,主机读取数据,这样可以避免主机
 *        一直读取数据,浪费时间
 * ****************************************/
void PB4_ExintSet(u8 en)
{
        EXTI_InitTypeDef EXTI_InitStructure;
        if(en)
        {
                GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource4);//PB4中断线
                EXTI_InitStructure.EXTI_Line = EXTI_Line4;//中断线
                EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//下降沿触发
                EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                EXTI_ClearITPendingBit(EXTI_Line4);//清除中断标志

                NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//设置优先级分组
                NVIC_InitTypeDef NVIC_InitStructure;
                NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
                NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
                NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;;
                NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
                NVIC_Init(&NVIC_InitStructure);
        }
        else
        {
                EXTI_InitStructure.EXTI_LineCmd = DISABLE;

        }
        EXTI_Init(&EXTI_InitStructure);
}

//中断服务函数,在主机SPI发送完成后,拉低MISO引脚,触发中断,主机读取数据
void EXTI4_IRQHandler(void)
{
        if(EXTI_GetITStatus(EXTI_Line4) != RESET)
        {
                //中断处理
                PB4_ExintSet(0);//关闭中断
                EXTI_ClearITPendingBit(EXTI_Line4);
        }
}

//**************************硬件SPI2的初始化********************************
//硬件SPI2的初始化

/*******************************************
 * 函    数：MySPI2_Hardware_SelectSS
 * 参    数：x:1为不选中,0为选中
 * 返 回 值：无
 * 描    述：硬件SPI2选择片选
 ******************************************/
void MySPI2_Hardware_SelectSS(uint8_t x)
{
    if(x)
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
    else
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}




/*******************************************
 * 函    数：MySPI2_Harware_Init
 * 参    数：无
 * 返 回 值：无
 * 描    述：硬件SPI2的初始化
 ******************************************/  
void MySPI2_Harware_Init(void)
{
    //初始化SPI2端口
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//使能GPIOB时钟
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_15;//PB13:SCK,PB15:MOSI
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//输入上拉
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;//PB14:MISO
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;//PB12:片选
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    //初始化SPI2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);//使能SPI2时钟
    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;//全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;//主机模式
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;//8位数据帧格式
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;//时钟极性,空闲时为低电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;//时钟相位,第一个时钟沿采样数据
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;//软件控制片选
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;//波特率预分频128
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;//高位在前
    SPI_Init(SPI2, &SPI_InitStructure);
   
    SPI_Cmd(SPI2, ENABLE);//使能SPI2
    MySPI2_Hardware_SelectSS(1);//片选拉高

}




/*******************************************
 * 函    数：MySPI2_Hardware_SwapByte
 * 参    数：Byte:要交换的字节
 * 返 回 值：交换后的字节
 * 描    述：硬件SPI2交换一个字节
 ******************************************/    
uint8_t MySPI2_Hardware_SwapByte(uint8_t Byte)
{
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);//等待发送缓冲区空
    SPI_I2S_SendData(SPI2, Byte);//发送数据
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);//等待接收缓冲区接收完成
    return SPI_I2S_ReceiveData(SPI2);
}



/*******************************************
 * 函    数：MySPI2_Hardware_ReadBytes
 * 参    数：buf:存放读取数据的缓冲区,len:要读取的字节数
 * 返 回 值：无
 * 描    述：硬件SPI2读取多个字节
 ******************************************/
void MySPI2_Hardware_ReadBytes(uint8_t *buf, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        buf[i] = MySPI2_Hardware_SwapByte(0xff);
    }
}


/*******************************************
 * 函    数：MySPI2_Hardware_WriteBytes
 * 参    数：buf:存放要写入数据的缓冲区,len:要写入的字节数
 * 返 回 值：无
 * 描    述：硬件SPI2写入多个字节
 ******************************************/
void MySPI2_Hardware_WriteBytes(uint8_t *buf, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        MySPI2_Hardware_SwapByte(buf[i]);
    }
}

