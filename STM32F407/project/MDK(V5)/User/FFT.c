 #include "board.h"
 #include "arm_math.h"

#define FFT_LENGTH		1024 		//FFT长度
float fft_inputbuf[FFT_LENGTH*2];	//FFT输入数组
float fft_outputbuf[FFT_LENGTH];	//FFT输出数组
float fft_outputbufhalf[FFT_LENGTH/2];

u32 i=0;
int newvalue = 0;
int a;





