#ifndef FFT_H
#define FFT_H
void FFT_RealDatedeal(float *fft_inputbuf,float *fft_outputbuf);
void FFT_GetMax(float *fft_outputbuf,float *maxValue,uint32_t *maxIndex);
#endif