#include "stm32f10x.h"  

void OLEDI2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//使能GPIOB时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);//使能I2C1时钟
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;//PB6
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;//开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;//占空比
    I2C_InitStructure.I2C_OwnAddress1 = 0x30;//自身地址
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;//使能应答
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;//7位地址
    I2C_InitStructure.I2C_ClockSpeed = 700000;//时钟速度400K
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1,ENABLE);//使能I2C1


}
/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
    I2C_GenerateSTART(I2C1, ENABLE);//产生起始信号
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)!=SUCCESS);//等待起始信号产生
    I2C_Send7bitAddress(I2C1, 0x78,I2C_Direction_Transmitter);//发送设备地址
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)!=SUCCESS);//等待设备地址发送
    I2C_SendData(I2C1, 0x00);//发送要写入的寄存器地址
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)!=SUCCESS);//等待数据发送完成
    I2C_SendData(I2C1, Command);//发送数据
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)!=SUCCESS);//等待数据发送完成
    I2C_GenerateSTOP(I2C1, ENABLE);//产生停止信号
}


/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
    I2C_GenerateSTART(I2C1, ENABLE);//产生起始信号
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)!=SUCCESS);//等待起始信号产生
    I2C_Send7bitAddress(I2C1, 0x78,I2C_Direction_Transmitter);//发送设备地址
     while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)!=SUCCESS);//等待设备地址发送
    I2C_SendData(I2C1, 0x40);//发送要写入的寄存器地址
     while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)!=SUCCESS);//等待数据发送完成
    I2C_SendData(I2C1, Data);//发送数据
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)!=SUCCESS);//等待数据发送完成
    I2C_GenerateSTOP(I2C1, ENABLE);//产生停止信号
    
}




/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}
void OLED_SendBuff(uint8_t buff[8][128])
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++)
        {
            OLED_WriteData(buff[j][i]);//发送数据
        }
    }
}



void OLED_Init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLEDI2C_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}


