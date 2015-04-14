#include <p32xxxx.h>
#include <plib.h>
#include "dsplib_dsp.h"
#include "fftc.h"

/* SYSCLK = 8MHz Crystal/ FPLLIDIV * FPLLMUL/ FPLLODIV = 80MHz
PBCLK = SYSCLK /FPBDIV = 80MHz*/
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1

/* Input array with 16-bit complex fixed-point twiddle factors.
 this is for 16-point FFT. For other configurations, for example 32 point FFT,
 Change it to fft16c32*/
#define fftc fft16c16

/* defines the sample frequency*/
#define SAMPLE_FREQ 2048

/* number of FFT points (must be power of 2) */
#define N 16

/* log2(16)=4 */
int log2N = 4;

/* Input array with 16-bit complex fixed-point elements. */
/* int 16c is data struct defined as following:
  typedef struct
{
	int16 re;
	int16 im;
} int16c; */
int16c sampleBuffer[N];

/* intermediate array */
int16c scratch[N];

/* intermediate array holds computed FFT until transmission*/
int16c dout[N]; 

/* intermediate array holds computed single side FFT */
int singleSidedFFT[N];

/* array that stores the corresponding frequency of each point in frequency domain*/
short freqVector[N];

/* indicates the dominant frequency */
int freq=0;

// Function definitions
int computeFFT(int16c *sampleBuffer);

main(){
        int i;
        /* assign values to sampleBuffer[] */
        for (i=0; i<N; i++)
        {
            sampleBuffer[i].re=i;
            sampleBuffer[i].im=0;
        }
        /* compute the corresponding frequency of each data point in frequency domain*/
        for (i=0; i<N/2; i++)
	{
            freqVector[i] = i*(SAMPLE_FREQ/2)/((N/2) - 1);
	}
        
while(1){
        /* get the dominant frequency */
        freq=freqVector[computeFFT(sampleBuffer)];
}
}


int computeFFT(int16c *sampleBuffer)
{
	int i;
        int dominant_freq=1;
        
	/* computer N point FFT, taking sampleBuffer[] as time domain inputs
         * and storing generated frequency domain outputs in dout[] */
	mips_fft16(dout, sampleBuffer, fftc, scratch, log2N);

	/* compute single sided fft */
	for(i = 0; i < N/2; i++)
	{
		singleSidedFFT[i] = 2 * ((dout[i].re*dout[i].re) + (dout[i].im*dout[i].im));
	}

        /* find the index of dominant frequency, which is the index of the largest data points */
        for(i = 1; i < N/2; i++)
	{
            if (singleSidedFFT[dominant_freq]<singleSidedFFT[i])
                    dominant_freq=i;
        }

        return dominant_freq;
}
