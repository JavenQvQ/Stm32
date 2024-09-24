/*woshinidie*/
#include "ADXL355.h"
#include "MySPI.h"
#include "stm32f10x.h"




int32_t volatile i32SensorX;
int32_t volatile i32SensorY;
int32_t volatile i32SensorZ;
int32_t volatile i32SensorT;
uint32_t volatile ui32SensorX;
uint32_t volatile ui32SensorY;
uint32_t volatile ui32SensorZ;
uint32_t volatile ui32SensorT;

/*******************************
 * 函    数：ADXL355_Select
 * 参    数：无
 * 返 回 值：无
 * 描    述：选择ADXL355
 ******************************/
void ADXL355_Select(void)
{
    MySPI_W_SS(0);//选择片选
}
/*******************************
 * 函    数：ADXL355_Unselect
 * 参    数：无
 * 返 回 值：无
 * 描    述：取消选择ADXL355
 
 ******************************/
void ADXL355_Unselect(void)
{
    MySPI_W_SS(1);//取消片选
}


/******************************
 * 函    数：ADXL355_Init
 * 参    数：无
 * 返 回 值：无
 * 描    述：初始化ADXL355
 ****************************/
void ADXL355_Init(ADXL355_HandleTypeDef *ADXL355_t)
{
    MySPI_Hardware_Init();//硬件SPI2初始化
    ADXL355_Range(ADXL355_t->ADXL355_Range);//设置量程
    ADXL355_Filter(ADXL355_t->ADXL355_HighPass, ADXL355_t->ADXL355_LowPass);//设置滤波器
    ADXL355_Startup();//启动ADXL355
}

/********************************
 * 函    数：ADXL355_WriteRegister
 * 参    数：ui8address:要写入的寄存器地址
 *         ui8Data:要写入的数据
 * 返 回 值：无
 * 描    述：写入ADXL355数据
*******************************/ 
void ADXL355_WriteRegister(uint8_t ui8address,	uint8_t ui8Data)
{
	uint8_t data[2];
	data[0] = ((ui8address << 1) | ADXL355_WRITE);//写入地址
	data[1] = ui8Data;
    ADXL355_Select();
    MySPI_Hardware_WriteBytes(data, 2);//写入数据
    ADXL355_Unselect();
}



/********************************
 * 函    数：ADXL355_ReadData
 * 参    数：ui8address:要读取的寄存器地址
 *         enRegs:要读取的寄存器个数
 * 返 回 值：读取的数据
 * 描    述：读取ADXL355数据
 *******************************/
uint32_t ADXL355_ReadRegister(uint8_t ui8address,enRegsNum enRegs) 
{
    uint8_t ui24Result[3];//存放读取的数据
	uint32_t ui32Result;
	uint8_t ui8writeAddress;

    ui8writeAddress = ((ui8address << 1) | ADXL355_READ);//读取地址
    ADXL355_Select();
    MySPI_Hardware_WriteBytes(&ui8writeAddress, 1);//写入地址

    if (enRegs ==SPI_READ_ONE_REG)//读取一个寄存器
    {
        MySPI_Hardware_ReadBytes(ui24Result, 1);//读取一个字节
        ui32Result = ui24Result[0];
    }
    if(enRegs == SPI_READ_TWO_REG)///* 仅用于温度、X、Y、z8偏移和阈值寄存器*/
    {
        MySPI_Hardware_ReadBytes(ui24Result, 2);//读取两个字节
        ui32Result = (ui24Result[0] << 8) | ui24Result[1];
    }
    else//仅用于 X，Y，Z 轴数据寄存器
    {
        MySPI_Hardware_ReadBytes(ui24Result, 3);//读取三个字节
        ui32Result = (ui24Result[0] << 16) | (ui24Result[1] << 8) | ui24Result[2];
    }
    ADXL355_Unselect();
    return ui32Result;
}


/********************************
 * 函    数：ADXL355_Startup
 * 参    数：无
 * 返 回 值：无
 * 描    述：启动ADXL355进入测量模式
 *******************************/
void ADXL355_Startup(void)
{
    uint8_t ui8temp;
	ui8temp = (uint8_t) ADXL355_ReadRegister(ADXL355_POWER_CTL, SPI_READ_ONE_REG); //读取POWER_CTL寄存器
    ui8temp &= ~(0x01);//清除POWER_CTL寄存器的低位,测量模式
    ADXL355_WriteRegister(ADXL355_POWER_CTL, ui8temp);//写入POWER_CTL寄存器
}

/********************************
 * 函    数：ADXL355_Standby
 * 参    数：无
 * 返 回 值：无
 * 描    述：启动ADXL355进入待机模式
 *******************************/
void ADXL355_Standby(void) 
{
	uint8_t ui8temp;

	ui8temp = (uint8_t) ADXL355_ReadRegister(ADXL355_POWER_CTL, SPI_READ_ONE_REG); //读取POWER_CTL寄存器

	ui8temp |= 0x01; //设置POWER_CTL寄存器的低位,待机模式

	ADXL355_WriteRegister( ADXL355_POWER_CTL, ui8temp); //写入POWER_CTL寄存器
}


/********************************
 * 函    数：ADXL355_ReadData
 * 参    数：无
 * 返 回 值：无
 * 描    述：读取ADXL355数据
 *******************************/
void ADXL355_ReadData()
 {

	/* 接收无符号整型原始数据 */
	ui32SensorX = ADXL355_ReadRegister(ADXL355_XDATA3, SPI_READ_THREE_REG);
    ui32SensorY = ADXL355_ReadRegister(ADXL355_YDATA3, SPI_READ_THREE_REG);
    ui32SensorZ = ADXL355_ReadRegister(ADXL355_ZDATA3, SPI_READ_THREE_REG);
    ui32SensorT = ADXL355_ReadRegister(ADXL355_TEMP2, SPI_READ_TWO_REG);


	/* 接收有符号整型原始数据 */
	i32SensorX = ADXL355_AccDataConversion(ui32SensorX);
	i32SensorY = ADXL355_AccDataConversion(ui32SensorY);
	i32SensorZ = ADXL355_AccDataConversion(ui32SensorZ);
	i32SensorT = ADXL355_AccDataConversion(ui32SensorT);
}

/********************************
 * 函    数：ADXL355_AccDataConversion
 * 参    数：ui32SensorData:传感器数据
 * 返 回 值：转换后的数据
 * 描    述：转换ADXL355数据
 *******************************/
int32_t ADXL355_AccDataConversion(uint32_t ui32SensorData)
{
	int32_t volatile i32Conversion = 0;

	ui32SensorData = (ui32SensorData >> 4);
	ui32SensorData = (ui32SensorData & 0x000FFFFF);

	if ((ui32SensorData & 0x00080000) == 0x00080000) {

		i32Conversion = (ui32SensorData | 0xFFF00000);

	} else {
		i32Conversion = ui32SensorData;
	}

	return i32Conversion;
}

/**
*函数名：ADXL355_Filter
*功能：设置ADXL355的滤波器
*参数：hpf:高通滤波器
*      lpf:低通滤波器
*返回值：无
 **/
void ADXL355_Filter( uint8_t hpf, uint8_t lpf) 
{
	uint8_t filter = 0;

	filter = (hpf << 4) | lpf; /* Combine high pass and low pass filter values to send */
	ADXL355_Standby();
	ADXL355_WriteRegister(ADXL355_FILTER, filter);/* Set filter values within FILTER register */
	ADXL355_Startup();

}

/**
 * 函数名：ADXL355_Range
 * 功能：设置ADXL355的量程
 * 参数：range:量程,2g,4g,8g
 * 返回值：无
 **/
void ADXL355_Range(uint8_t range) 
{
	ADXL355_WriteRegister(ADXL355_RANGE, range); /* Set sensor range within RANGE register */
}




