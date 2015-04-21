// CPEG222 - Project 2 Code
// Author: Byron Lambrou, Andrew Lipman
// Input: Keypad PMod, Microphone Pmod
// Output:  2xSSD pmods
// Comments:

/* TODO:1.configure pin mapping*/
/* TODO:2.set up the timer interrupts*/
/* TODO:3.set up the adc*/
/* TODO:4.program in the state logic*/
/* TODO:5.set up change notice interrupts for keypad*/
/* TODO:6.set up interrupt priority order */

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
#define fftc fft16c1024

/* defines the sample frequency*/
#define SAMPLE_FREQ 2048

/* number of FFT points (must be power of 2) */
#define N 1024

// 7 Segment Display pmod using the TOP JA & JB jumpers
// Segments
#define SegA_R LATEbits.LATE0
#define SegB_R LATEbits.LATE1
#define SegC_R LATEbits.LATE2
#define SegD_R LATEbits.LATE3
#define SegE_R LATGbits.LATG9
#define SegF_R LATGbits.LATG8
#define SegG_R LATGbits.LATG7
// Display selection. 0 = right, 1 = left (Cathode)
#define DispSel_R LATGbits.LATG6

// 7 Segment Display pmod using the TOP JC and JD jumpers
#define SegA_L LATGbits.LATG12
#define SegB_L LATGbits.LATG13
#define SegC_L LATGbits.LATG14
#define SegD_L LATGbits.LATG15
#define SegE_L LATDbits.LATD7
#define SegF_L LATDbits.LATD1
#define SegG_L LATDbits.LATD9
// Display selection. 0 = right, 1 = left (Cathode)
#define DispSel_L LATCbits.LATC1

//Keypad pmod using the JJ input
//Column pins, should be inputs?
#define Col4 PORTBbits.RB0
#define Col3 PORTBbits.RB1
#define Col2 PORTBbits.RB2
#define Col1 PORTBbits.RB3
//Row pins, should be outputs?
#define Row4 LATBbits.LATB4
#define Row3 LATBbits.LATB5
#define Row2 LATBbits.LATB8
#define Row1 LATBbits.LATB9

//On Board LED pins
#define BLed1   LATBbits.LATB10
#define BLed2   LATBbits.LATB11
#define BLed3   LATBbits.LATB12
#define BLed4   LATBbits.LATB13

//Port mapping: Mic- port JK and Keypad

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
int freq = 0;

// Function definitions
int computeFFT(int16c *sampleBuffer);

//Timer 5 interrupt, we can use this for the first interrupt

void __ISR(_TIMER_5_VECTOR, ipl4) _T5Interrupt(void) {

    IFS0CLR = 0x100000; // Clear Timer5 interrupt status flag (bit 20)
}

//Timer 4 interrupt, we can use this for the second interrupt
//Note that we'll want to change either the Timer 5 ipl or Timer4 ipl

void __ISR(_TIMER_4_VECTOR, ipl4) _T4Interrupt(void) {

    IFS0bits.T4IF = 0; // Clear Timer4 interrupt status flag
}

void initADC(int amask) {
    //ADC automatic confug
    AD1PCFG = 0xEFFF; //All PORTB = digital but RB12 = analog
    AD1CON1 = 0x00E0; //Automatic conversion after sampling
    AD1CHS = 0x000C0000; //Connect RB12/AN12 as CH0 input
    AD1CSSL = 0; //No scanning required
    AD1CON2 = 0; //Use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F3F; //Tad=128 x Tpb, Sample time=31 Tad
    AD1CON1bits.ADON = 1; //turn on the ADC
}

/* This is the ISR for the keypad CN interrupts */

/* The priority here needs to be changed */
void __ISR(_CHANGE_NOTICE_VECTOR, ipl5) ChangeNotice_Handler(void) {
    // 1. Disable interrupts
    INTDisableInterrupts();
    // 2. Debounce keys
    int forLoop = 0;
    while (forLoop < 1000) {
        forLoop++;
    }
    forLoop = 0;
    // 3. Decode which key was pressed
    //First, read the inputs to clear the CN mismatch condition
    dummy = PORTB;
    //Now, walk through the row variables setting them equal to zero

    number_of_keys = 0;
    pressed_key = 0;



    Row1 = 0;
    Row2 = Row3 = Row4 = 1;

    if (Col1 == 0) {
        pressed_key = 0;
        number_of_keys++;
    }
    if (Col2 == 0) {
        pressed_key = 1;
        number_of_keys++;
    }
    if (Col3 == 0) {
        pressed_key = 2;
        number_of_keys++;
    }
    if (Col4 == 0) {
        pressed_key = 3;
        number_of_keys++;
    }

    Row2 = 0;
    Row1 = Row3 = Row4 = 1;

    if (Col1 == 0) {
        pressed_key = 4;
        number_of_keys++;
    }
    if (Col2 == 0) {
        pressed_key = 5;
        number_of_keys++;
    }
    if (Col3 == 0) {
        pressed_key = 6;
        number_of_keys++;
    }
    if (Col4 == 0) {
        pressed_key = 7;
        number_of_keys++;
    }

    Row3 = 0;
    Row1 = Row2 = Row4 = 1;

    if (Col1 == 0) {
        pressed_key = 8;
        number_of_keys++;
    }
    if (Col2 == 0) {
        pressed_key = 9;
        number_of_keys++;
    }
    if (Col3 == 0) {
        pressed_key = 10;
        number_of_keys++;
    }
    if (Col4 == 0) {
        pressed_key = 11;
        number_of_keys++;
    }
    Row4 = 0;
    Row1 = Row3 = Row2 = 1;

    if (Col1 == 0) {
        pressed_key = 12;
        number_of_keys++;
    }
    if (Col2 == 0) {
        pressed_key = 13;
        number_of_keys++;
    }
    if (Col3 == 0) {
        pressed_key = 14;
        number_of_keys++;
    }
    if (Col4 == 0) {
        pressed_key = 15;
        number_of_keys++;
    }

    if (number_of_keys > 1) {
        pressed_key = 0;
    }
    if (number_of_keys == 1) {
        key_detected = pressed_key;
        key_to_react = 1;
    } else {
        pressed_key = 0;
    }
    number_of_keys = 0;
    Row1 = Row2 = Row3 = Row4 = 0;
    PORTB;
    //Clears interrupt flag
    IFS1CLR = 0x0001;
    // 5. Enable interrupts
    INTEnableInterrupts();

}

int readADC(int ch) {
    AD1CON1bits.SAMP = 1; //1. start sampling
    while (!AD1CON1bits.DONE); //2. Wait until done
    return ADC1BUF0; //3. read conversion result
}

int mode = 1;
int passcode = 0;
unsigned int dummy;
int key_to_react = 0;
int pressed_key;
int number_of_keys = 0;
int key_detected;

main() {

    initADC(1);

    int i;
    INTDisableInterrupts();
    /* These values should be changed to reflect our timer config!*/
    // Configure Timer 5.
    T5CONbits.ON = 0; // Stop timer, clear control registers
    TMR5 = 0; // Timer counter
    PR5 = 0xC000; //Timer count amount for interupt to occur
    IPC5bits.T5IP = 0; //prioirty 4
    IFS0bits.T5IF = 0; // clear interrupt flag
    T5CONbits.TCKPS = 0; // prescaler at 1:256, internal clock sourc
    T5CONbits.ON = 1; // Timer 5 module is enabled
    IEC0bits.T5IE = 1; //enable Timer 5


    //Configure Timer 4
    T4CONbits.ON = 0; //Turn Timer 4 off
    TMR4 = 0; //Clear Timer 4 register
    T4CONbits.TCKPS = 3; //Select prescaler = 256
    T4CONbits.TCS = 0; //Select internal clock
    PR4 = 901250; //Load period Register
    IFS0bits.T4IF = 0; //Clear Timer 4 interupt flag
    IPC4bits.T4IP = 4; //Set priority level to 4
    IEC0bits.T4IE = 1; // Enable Timer 4
    T4CONbits.ON = 1; //Turn Timer  4 on.


    //Configure Change Notice for the keypad
    // 1. Configure CNCON, CNEN, CNPUE
    //First, turn on Change interrupts
    CNCON = 0x8000;
    //Then we want CN enable on pins 2,3,4,5
    CNEN = 0x003C;
    //We also want to enable the pull up resistors on the board
    CNPUE = 0x003C;

    // 2. Perform a dummy read to clear mismatch
    PORTB;

    // 3. Configure IPC5, IFS1, IEC1
    //These set the priority and subpriority to 5 and 3 respectively
    //This priority needs to be changed
    IPC6SET = 0x00140000;
    IPC6SET = 0x00030000;
    //Clear the Interrupt flag status bit
    IFS1CLR = 0x0001;
    //Enable Change Notice Interrupts
    IEC1SET = 0x0001;

    // 4. Enable vector interrupt
    INTEnableSystemMultiVectoredInt();
    //We want all the Row pins at zero so we can detect any inputs on the buttons.
    Row1 = Row2 = Row3 = Row4 = 0;

    /* assign values to sampleBuffer[] */
    for (i = 0; i < N; i++) {
        sampleBuffer[i].re = i;
        sampleBuffer[i].im = 0;
    }
    /* compute the corresponding frequency of each data point in frequency domain*/
    for (i = 0; i < N / 2; i++) {
        freqVector[i] = i * (SAMPLE_FREQ / 2) / ((N / 2) - 1);
    }

    while (1) {
        /* get the dominant frequency */
        freq = freqVector[computeFFT(sampleBuffer)];

        /* Current state logic here*/
        switch (mode)
            case 1:
            /* Pmod msd should show 's', and last 3 digits should show passcode */

            break;
        case 2:
        /* Pmod msd should show 'u', and last 3 digits should show off */
        break;
        case 3:
        /* Pmod msd should show 'L', and last 3 digits should show entered pass */
        break;
        case 4:
        /* Pmod msd should Flash "AAAA" */
        break;


        /* Next state logic here*/
        switch (mode)
            case 1:
            /* 0-9 should input digit, 'C' should clear, 'D' should delete, */
            /* 'E' enter if valid input pass*/
            break;
        case 2:
        /* if a button is pressed and held for >=1 second, go back to mode 1 */
        /* if a button is pressed and held for <1 second, go to mode 3 */
        break;
        case 3:
        /* 0-9 should input digit, 'C' should clear, 'D' should delete, */
        /* 'E' enter if valid input pass. If pass is correct, enter mode 2*/
        /* if pass incorrect, enter mode 4. If microphone frequency is equal */
        /* to pass frequency, enter mode 2, else to mode 4. */
        break;
        case 4:
        /* If SSDs have been flashing for 5 seconds, then enter mode 3.*/
        break;
    }
}

int computeFFT(int16c *sampleBuffer) {
    int i;
    int dominant_freq = 1;

    /* computer N point FFT, taking sampleBuffer[] as time domain inputs
     * and storing generated frequency domain outputs in dout[] */
    mips_fft16(dout, sampleBuffer, fftc, scratch, log2N);

    /* compute single sided fft */
    for (i = 0; i < N / 2; i++) {
        singleSidedFFT[i] = 2 * ((dout[i].re * dout[i].re) + (dout[i].im * dout[i].im));
    }

    /* find the index of dominant frequency, which is the index of the largest data points */
    for (i = 1; i < N / 2; i++) {
        if (singleSidedFFT[dominant_freq] < singleSidedFFT[i])
            dominant_freq = i;
    }

    return dominant_freq;
}
