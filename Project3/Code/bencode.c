#include <p32xxxx.h>
#include <plib.h>
#include "dsplib_dsp.h"
#include "fftc.h"

/* SYSCLK = 8MHz Crystal/ FPLLIDIV * FPLLMUL/ FPLLODIV = 80MHz
PBCLK = SYSCLK /FPBDIV = 80MHz*/
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1

// The counter speed
#define DEBOUNCE_TIME 200
// The refresh rate of the segments
#define REFRESH_RATE 100

// Keypad in JJ port

#define C4  PORTBbits.RB0
#define C3  PORTBbits.RB1
#define C2  PORTBbits.RB2
#define C1  PORTBbits.RB3
#define R4  PORTBbits.RB4
#define R3  PORTBbits.RB5
#define R2  PORTBbits.RB8
#define R1  PORTBbits.RB9


// 7 Segment Display pmod using the TOP JA & JB jumpers
// Segments
#define SegA LATEbits.LATE0
#define SegB LATEbits.LATE1
#define SegC LATEbits.LATE2
#define SegD LATEbits.LATE3
#define SegE LATGbits.LATG9
#define SegF LATGbits.LATG8
#define SegG LATGbits.LATG7
// Display selection. 0 = right, 1 = left (Cathode)
#define DispSel LATGbits.LATG6

// 7 Segment Display pmod using the TOP JC & JD jumpers
// Segments
#define SegA0 LATGbits.LATG12
#define SegB0 LATGbits.LATG13
#define SegC0 LATGbits.LATG14
#define SegD0 LATGbits.LATG15
#define SegE0 LATDbits.LATD7
#define SegF0 LATDbits.LATD1
#define SegG0 LATDbits.LATD9
// Display selection. 0 = right, 1 = left (Cathode)
#define DispSel0 LATCbits.LATC1

/* Input array with 16-bit complex fixed-point twiddle factors.
 this is for 16-point FFT. For other configurations, for example 32 point FFT,
 Change it to fft16c32*/
#define fftc fft16c512

/* defines the sample frequency*/
#define SAMPLE_FREQ 1024

/* number of FFT points (must be power of 2) */
#define N 512

/* log2(16)=4 */
int log2N = 9;


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

int readADC()
{
    AD1CON1bits.SAMP = 1; // 1. start sampling
    for (TMR1=0; TMR1<100; TMR1++); //2. wait for sampling time
    AD1CON1bits.SAMP = 0; // 3. start the conversion
    while (!AD1CON1bits.DONE); // 4. wait conversion complete
    return ADC1BUF0; // 5. read result
}
// Function definitions
int computeFFT(int16c *sampleBuffer);

int ones;
int tens;
int hundreds;

int ones_Digit;
int tens_Digit;

int ones_Left;
int tens_Left;
int ones_Right;
int tens_Right;

int password = 1000;
int key_value = 16;


//0-9 numbers
unsigned char SSD_number[]={
0b0111111,	//0
0b0000110,	//1
0b1011011,	//2
0b1001111,	//3
0b1100110,	//4
0b1101101,	//5
0b1111101,	//6
0b0000111,	//7
0b1111111,	//8
0b1101111,	//9
0b0000000,	//clear
};

unsigned char SSD_letter[]={
0b1101101,	//S
0b1111001,	//E
0b0111000,	//L
0b1110111,	//A
0b0000000,      //Clr
};

//reset interrupt function

//remember to change back to displaydigitRight
void displayDigitRight (unsigned char value, int sel){ // Display right 7 segment score

SegA = value & 1;
SegB = (value >> 1) & 1;
SegC = (value >> 2) & 1;
SegD = (value >> 3) & 1;
SegE = (value >> 4) & 1;
SegF = (value >> 5) & 1;
SegG = (value >> 6) & 1;
DispSel = sel;
}

void displayDigitLeft (unsigned char value, int sel){ // Display left 7 segment score

SegA0 = value & 1;
SegB0 = (value >> 1) & 1;
SegC0 = (value >> 2) & 1;
SegD0 = (value >> 3) & 1;
SegE0 = (value >> 4) & 1;
SegF0 = (value >> 5) & 1;
SegG0 = (value >> 6) & 1;
DispSel0 = sel;
}

void showLetterLeft(int letterValue){
    int j = 0;

    unsigned char letterVal1;
    unsigned char letterVal2;

    if (letterValue == 0){      //CO
        letterVal1 = 0b0111001;
        letterVal2 = 0b0111111;
    }

    else if (letterValue == 1){      //HH
        letterVal1 = letterVal2 = 0b1110110;
    }

    else if (letterValue == 2){      //LL
        letterVal1 = letterVal2 = 0b0111000;
    }

    else if (letterValue == 3){      //EE
        letterVal1 = letterVal2 = 0b1111001;
    }

    for (j=0;j<(15 * REFRESH_RATE);j++){
        displayDigitLeft(letterVal1,1);
    }
    for (j=0;j<REFRESH_RATE;j++){
        displayDigitLeft(letterVal2,0);
    }
}


void showNumberLeft(int left_num){
    int j = 0;

    if (left_num == 1000) {
        ones_Left = 10;
        tens_Left = 10;
    }
    else {
        ones_Left = (left_num % 10);
        tens_Left = (left_num / 10);
    }

    for (j=0; j < (15 *REFRESH_RATE); j++){
        displayDigitLeft(SSD_number[tens_Left],1);
    }
    for (j=0; j < REFRESH_RATE;j++){
        displayDigitLeft(SSD_number[ones_Left],0);
    }
}

void showNumberRight(int right_num){
    int j = 0;

    if (right_num == 1000) {
        ones_Right = 10;
        tens_Right = 10;
    }
    else{
        ones_Right = (right_num % 10);
        tens_Right = (right_num / 10);
    }

    for (j=0;j<(REFRESH_RATE);j++){
        displayDigitRight(SSD_number[tens_Right],1);
    }
    for (j=0;j< 15 * REFRESH_RATE;j++){
        displayDigitRight(SSD_number[ones_Right],0);
    }
}


void resetInterrupt() {
    IFS1CLR = 0x0001;
    R1 = R2 = R3 = R4 = 0;
    INTEnableInterrupts();
}

int i = 0;

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


void __ISR(_CHANGE_NOTICE_VECTOR, ipl5) CN_Handler(void){

    INTDisableInterrupts();

    for(i=0;i<DEBOUNCE_TIME;i++); // ignore bouncing signals

    // goes through each row to figure out what key was pressed
    R1 = 0; //probes row 1, all other rows are off
    R2 = R3 = R4 = 1;
    if (C1 == 0){
        key_value = 1;
        resetInterrupt();
        return;
    } //probes this column to see if this was the column that was pressed
    else if (C2 == 0){
        key_value = 2;
        resetInterrupt();
        return;
    }
    else if (C3 == 0){
        key_value = 3;
        resetInterrupt();
        return;
    }
    else if (C4 == 0){
        key_value = 10;
        resetInterrupt();
        return;
    }
    R2 = 0;
    R1 = R3 = R4 = 1;
    if (C1 == 0){
        key_value = 4;
        resetInterrupt();
        return;
    }
    else if (C2 == 0){
        key_value = 5;
        resetInterrupt();
        return;
    }
    else if (C3 == 0){
        key_value = 6;
        resetInterrupt();
        return;
    }
    else if (C4 == 0){
        key_value = 11;
        resetInterrupt();
        return;
    }
    R3 = 0;
    R1 = R2 = R4 = 1;
    if (C1 == 0){
        key_value = 7;
        resetInterrupt();
        return;
    }
    else if (C2 == 0){
        key_value = 8;
        resetInterrupt();
        return;
    }
    else if (C3 == 0){
        key_value = 9;
        resetInterrupt();
        return;
    }
    else if (C4 == 0){
        key_value = 12;
        resetInterrupt();
        return;
    }
    R4 = 0;
    R1 = R2 = R3 = 1;
    if (C1 == 0){
        key_value = 0;
        resetInterrupt();
        return;
    }
    else if (C2 == 0){
        key_value = 15;
        resetInterrupt();
        return;
    }
    else if (C3 == 0){
        key_value = 14;
        resetInterrupt();
        return;
    }
    else if (C4 == 0){
        key_value = 13;
        resetInterrupt();
        return;
    }
    resetInterrupt();
}



main(){

    //Configure ports C~G to be output ports
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0;
    TRISB = 0x80F;

    // initialize C~G to 0/
    PORTA = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;
    PORTF = 0x00;
    PORTG = 0x00;

    AD1PCFG = 0xF7FF; // all PORTB = digital but RB7 = analog
    AD1CON1 = 0; // manual conversion sequence control
    AD1CHS = 0x000B0000; // Connect RB7/AN7 as CH0 input
    AD1CSSL = 0; // no scanning required
    AD1CON2 = 0; // use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F02; // Tad = 128 x Tpb, Sample time = 31 Tad
    AD1CON1bits.ADON = 1; // turn on the ADC

    INTDisableInterrupts();

    T5CON = 0x0; // Stop timer, clear control registers
    TMR5 = 0x0; // Timer counter
    PR5 = 0x7A11;
    IPC5SET = 0x0010; //prioirty 4
    IFS0CLR = 0x100000; // clear interrupt flag
    IEC0SET = 0x100000; //enable Timer 5
    T5CON = 0x0070;  // prescaler at 1:256, internal clock source
    T5CONSET = 0x8000;  // Timer 5 module is enabled
    INTEnableSystemMultiVectoredInt();

    CNCON = 0x8000;   // Enable Change Notice module
    CNEN = 0x3C;      // Enable CN 2-5
    CNPUE = 0x3C;     // Enable weak pull ups

    // read port(s) to clear mismatch on change notice pins
    PORTB;
    PORTBCLR = 0xFFF0;
    IPC6SET = 0x00140000;   // Set priority level=5
    IPC6SET = 0x00030000;   // Set Subpriority level=3
                            // Could have also done this in single
                            // operation by assigning IPC6SET = 0x00170000

    IFS1CLR = 0x1;       // Clear the interrupt flag status bit
    IEC1SET = 0x1;       // Enable Change Notice interrupts

    INTEnableSystemMultiVectoredInt();

    // enable interrupts again


    int i;
    while(1){

        /* assign values to sampleBuffer[] */
        for (i=0;  i < N; i++)
        {
            sampleBuffer[i].re=readADC(); //add this function
            sampleBuffer[i].im=0;
        }
        /* compute the corresponding frequency of each data point in frequency domain*/
        for (i=0; i < N/2; i++)
	{
            freqVector[i] = i*(SAMPLE_FREQ/2)/((N/2) - 1);
	}

        /* get the dominant frequency */
        freq = freqVector[computeFFT(sampleBuffer)];

        if(readADC() > 270){
            //showNumberLeft(freq/100);
            //showNumberRight(freq%100);
            showNumberLeft(readADC()/100);
            showNumberRight(readADC()%100);
        }
        else{
            showNumberLeft(1000);
            showNumberRight(1000);
        }
    }
}