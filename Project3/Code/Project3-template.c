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
#include <ctype.h>
#include <stdlib.h>
#include "dsplib_dsp.h"
#include "fftc.h"

/* SYSCLK = 8MHz Crystal/ FPLLIDIV * FPLLMUL/ FPLLODIV = 80MHz
PBCLK = SYSCLK /FPBDIV = 80MHz*/
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1

/* Input array with 16-bit complex fixed-point twiddle factors.
 this is for 16-point FFT. For other configurations, for example 32 point FFT,
 Change it to fft16c32 */
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

//Port mapping: Mic- port JK input

/* [Var]Variable Declarations */
int for_Loop = 0;
int SSD_blink = 0;
int trigger = 0;

int left_ssd_val = 1;
int test = 1;

unsigned int dummy;
int key_to_react = 1;
int key_pressed = 0;
int key_released = 0;
int pressed_key;
int last_key=0;
int key_detected;
int button_lock = 0;

int passcode=0;
int guess=0;
int length_held=0;

int samp = 0;
int mode = 1;
int time_counter_ssd = 0;
int time_counter_seconds = 0;

int sample_counter = 0;

/* log2(16)=4 */
int log2N = 10;

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
int computeFFT(int16c sampleBuffer[]);

unsigned char SSD_number[] = {
    0b0111111, //0
    0b0000110, //1
    0b1011011, //2
    0b1001111, //3
    0b1100110, //4
    0b1101101, //5
    0b1111101, //6
    0b0000111, //7
    0b1111111, //8
    0b1101111, //9
    0b1110111, //A
    0b1111100, //B
    0b0111001, //C
    0b1011110, //D
    0b1111001, //E
    0b1110001, //F
    0b1110110, //H
    0b0111000, //L
    0b0011100, //u
    0b0000000 //clear
};
char keypad_number[] = {
    1, //0
    2, //1
    3, //2
    'A', //3
    4, //4
    5, //5
    6, //6
    'B', //7
    7, //8
    8, //9
    9, //10
    'C', //11
    0, //12
    'F', //13
    'E', //14
    'D', //15
    'Q' //error16
};

//Timer 5 interrupt, we can use this for the first interrupt

void __ISR(_TIMER_5_VECTOR, ipl5) _T5Interrupt(void) {
    //This timer should occur on a 2048Hz frequency, and is used for the ADC and FFT
    //first we should read from the ADC to find the value of the sample
    //readADC();
    //Because we're listening to a real valued signal, we will only write to the 're' value
    sampleBuffer[sample_counter].re = readADC();
    sample_counter++;
    //Once we have the voltage value of the sample, we must put that into the buffer at the corresponding spot
    //We are taking 2048 samples per second- Each time we take a sample, we write it to the FFT buffer
    //The FFT buffer has a length N of 1024
    //Thus, every time we take in 1024 samples, we must call the FFT to find the frequency

    if (sample_counter == 1024) {
        //call computeFFT() here;
        freq = freqVector[computeFFT(&sampleBuffer)];

        sample_counter = 0;
    }


    IFS0CLR = 0x100000; // Clear Timer5 interrupt status flag (bit 20)
}

//Timer 2/3 type B interrupt: we can use this for the SSD/1Sec and 2Sec displays
//Note that this uses interrupt flag for Timer 3

void __ISR(_TIMER_3_VECTOR, ipl6) _T3Interrupt(void) {
    //This timer occurs on a 80Hz frequency- every time it occurs, we should switch the SSD
    //side
    left_ssd_val = !(left_ssd_val);
    time_counter_ssd++;
    //Then we need to set up counting such that every 80th interrupt it increases the seconds timer:
    if (time_counter_ssd % 80 == 0) {
        //Now, in our logic, when we press a button, we simply set both time_counter_ssd and _seconds to 0
        //and check their values on release
        SSD_blink=!SSD_blink;
        time_counter_seconds++;
        time_counter_ssd = 0;
    }
            switch (mode) {
            case 1:
                /* Pmod msd should show 's', and last 3 digits should show passcode */
                displaySSD(freq,5);
                break;
            case 2:
                /* Pmod msd should show 'u', and last 3 digits should show off */
                displaySSD(0,18);
                break;
            case 3:
                /* Pmod msd should show 'L', and last 3 digits should show entered pass */
                displaySSD(guess,17);
                break;
            case 4:
                /* Pmod msd should Flash "AAAA" */
                displaySSD(0,4);
                break;
        }
    IFS0bits.T3IF = 0; // Clear Timer4 interrupt status flag
}


/* This is the ISR for the keypad CN interrupts */

/* The priority here needs to be changed, I believe */
void __ISR(_CHANGE_NOTICE_VECTOR, ipl4) ChangeNotice_Handler(void) {
    // 1. Disable interrupts
    // 2. Debounce keys
    int forLoop = 0;
    while (forLoop < 1) {
        forLoop++;
    }
    forLoop = 0;
    // 3. Decode which key was pressed
    //First, read the inputs to clear the CN mismatch condition
    dummy = PORTB;
    //Now, walk through the row variables setting them equal to zero

    pressed_key = 16;



    Row1 = 0;
    Row2 = Row3 = Row4 = 1;

    if (Col1 == 0) {
        pressed_key = 0;
    }
    if (Col2 == 0) {
        pressed_key = 1;
    }
    if (Col3 == 0) {
        pressed_key = 2;
    }
    if (Col4 == 0) {
        pressed_key = 3;
    }

    Row2 = 0;
    Row1 = Row3 = Row4 = 1;

    if (Col1 == 0) {
        pressed_key = 4;
    }
    if (Col2 == 0) {
        pressed_key = 5;
    }
    if (Col3 == 0) {
        pressed_key = 6;
    }
    if (Col4 == 0) {
        pressed_key = 7;
    }

    Row3 = 0;
    Row1 = Row2 = Row4 = 1;

    if (Col1 == 0) {
        pressed_key = 8;
    }
    if (Col2 == 0) {
        pressed_key = 9;
    }
    if (Col3 == 0) {
        pressed_key = 10;
    }
    if (Col4 == 0) {
        pressed_key = 11;
    }
    Row4 = 0;
    Row1 = Row3 = Row2 = 1;

    if (Col1 == 0) {
        pressed_key = 12;
    }
    if (Col2 == 0) {
        pressed_key = 13;
    }
    if (Col3 == 0) {
        pressed_key = 14;
    }
    if (Col4 == 0) {
        pressed_key = 15;
    }

    if (pressed_key!=16 && !(button_lock)) {
        //If we had a key press and not release, activate buttonlock
        button_lock=1;
        //Start counting how long the button is held
        length_held=0;
        time_counter_ssd = 0;
        time_counter_seconds = 0;
        //Store the pressed key in key_detected
        key_detected = pressed_key;
        //Store the fact a key was pressed in a flag
        key_pressed=1;
        //Store the fact that we should react to the press in a flag
        key_to_react = 1;
    }
    if (pressed_key==16 && button_lock) {
        //If we had a key release and not press, turn off buttonlock
        button_lock=0;
        //We started the time counter at zero on the last press, thus length held
        //is equal to the timer
        length_held=time_counter_seconds;
        //Store the key release in a flag
        key_released = 1;
        key_to_react = 1;
        //Keep track of what the key was that we released
        last_key=key_detected;
    }
    Row1 = Row2 = Row3 = Row4 = 0;
    PORTB;
    //Clears interrupt flag
    IFS1CLR = 0x0001;
    // 5. Enable interrupts
}

int key_detected_toregint(int a) {
    //Translated the key_pressed number from the keypad to an integer based on
    // the values coded into the ISR function
    int to_Return = 0;
    if (a >= 0 && a <= 2) {
        to_Return = a + 1;
    } else if (a >= 4 && a <= 6) {
        to_Return = a;
    } else if (a >= 8 && a <= 10) {
        to_Return = a - 1;
    }
    return to_Return;
}


int readADC() {
//    AD1CON1bits.SAMP = 1; //1. start sampling
//    while (!AD1CON1bits.DONE); //2. Wait until done
//    return ADC1BUF0; //3. read conversion result
    AD1CON1bits.SAMP = 1; // 1. start sampling
    for (TMR1=0; TMR1<100; TMR1++); //2. wait for sampling time
    AD1CON1bits.SAMP = 0; // 3. start the conversion
    while (!AD1CON1bits.DONE); // 4. wait conversion complete
    return ADC1BUF0; // 5. read result

}

void displayDigit(unsigned char value, unsigned int left_ssd, unsigned int leftdisp) {
    //For left=1, display on left SSD, else, on right SSD
    if (left_ssd == 1) {
        if (leftdisp == 1) {
            SegA_L = value & 1;
            SegB_L = (value >> 1) & 1;
            SegC_L = (value >> 2) & 1;
            SegD_L = (value >> 3) & 1;
            SegE_L = (value >> 4) & 1;
            SegF_L = (value >> 5) & 1;
            SegG_L = (value >> 6) & 1;
            DispSel_L = 0;
        } else {
            SegA_L = value & 1;
            SegB_L = (value >> 1) & 1;
            SegC_L = (value >> 2) & 1;
            SegD_L = (value >> 3) & 1;
            SegE_L = (value >> 4) & 1;
            SegF_L = (value >> 5) & 1;
            SegG_L = (value >> 6) & 1;
            DispSel_L = 1;
        }

    } else {
        if (leftdisp == 1) {
            SegA_R = value & 1;
            SegB_R = (value >> 1) & 1;
            SegC_R = (value >> 2) & 1;
            SegD_R = (value >> 3) & 1;
            SegE_R = (value >> 4) & 1;
            SegF_R = (value >> 5) & 1;
            SegG_R = (value >> 6) & 1;
            DispSel_R = 0;
        } else {
            SegA_R = value & 1;
            SegB_R = (value >> 1) & 1;
            SegC_R = (value >> 2) & 1;
            SegD_R = (value >> 3) & 1;
            SegE_R = (value >> 4) & 1;
            SegF_R = (value >> 5) & 1;
            SegG_R = (value >> 6) & 1;
            DispSel_R = 1;
        }

    }
}

void showNumber(int digit, unsigned int left_ssd_num, unsigned int leftdisp) {
    displayDigit(SSD_number[digit % 20], left_ssd_num, leftdisp);
}
void clearSSDS() {
    displayDigit(0b0000000, 0, 0);
    displayDigit(0b0000000, 1, 0);
    displayDigit(0b0000000, 0, 1);
    displayDigit(0b0000000, 1, 1);
}
void displaySSD(int input, int mode_input) {
    //input 0 - 999 range
    if(mode==1 || mode ==3){
        if (left_ssd_val) {
            showNumber(input % 10, 0, 1);
            showNumber((input / 100) % 10, 1, 1);
        }
        else {
            showNumber((input / 10) % 10, 0, 0);
            showNumber(mode_input, 1, 0);
        }
    }
    if(mode==2){
            showNumber(18, 1, 0);
    }
    if(mode==4){
        if(SSD_blink){
            if (left_ssd_val) {
                showNumber(10, 0, 1);
                showNumber(10, 1, 1);
            }
            else {
                showNumber(10, 0, 0);
                showNumber(10, 1, 0);
            }
        }
        else{
            clearSSDS();
        }
    }
}

main() {


    INTDisableInterrupts();
    TRISB = 0x80F;
    //ADC automatic config
//    AD1PCFG = 0xEFFF; //All PORTB = digital but RB12 = analog
//    AD1CON1 = 0x00E0; //Automatic conversion after sampling
//    AD1CHS = 0x000C0000; //Connect RB12/AN12 as CH0 input
//    AD1CSSL = 0; //No scanning required
//    AD1CON2 = 0; //Use MUXA, AVss/AVdd used as Vref+/-
//    AD1CON3 = 0x1F3F; //Tad=128 x Tpb, Sample time=31 Tad
//    AD1CON1SET = 0x8000; //turn on the ADC

    //ADC manual config
    AD1PCFG = 0xF7FF; // all PORTB = digital but RB7 = analog
    AD1CON1 = 0; // manual conversion sequence control
    AD1CHS = 0x000B0000; // Connect RB7/AN7 as CH0 input
    AD1CSSL = 0; // no scanning required
    AD1CON2 = 0; // use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F02; // Tad = 128 x Tpb, Sample time = 31 Tad
    AD1CON1bits.ADON = 1; // turn on the ADC


    //Configure ports C~G to be output ports
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0;
    // initialize C~G to 0
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;
    PORTF = 0x00;
    PORTG = 0x00;
    //Set the pins that correspond to the Column pins to inputs

    //Then clear everything but their pins
    int i;

    /* These values should be changed to reflect our timer config!*/
    // Configure Timer for the ADC sampling
    // This will be a type A timer
    T5CONbits.ON = 0; // Stop timer, clear control registers
    TMR5 = 0; // Timer counter
    PR5 = 39062; //Timer count amount for interupt to occur - 2048Hz frequency
    IPC5bits.T5IP = 5; //prioirty 5
    IFS0bits.T5IF = 0; // clear interrupt flag
    T5CONbits.TCKPS = 0; // prescaler at 1:256, internal clock sourc
    T5CONbits.ON = 1; // Timer 5 module is enabled
    IEC0bits.T5IE = 1; //enable Timer 5


    //Configure Timer for the SSD display and 1 and 5 second timers respectively
    //This will be a Type B timer of TMR 2 and TMR 3
    T2CONbits.ON = 0; //Turn Timer 2 off
    T3CONbits.ON = 0; //Turn Timer 3 off
    T2CONbits.T32 = 1; //Enable 32 bit mode
    T3CONbits.TCKPS = 0; //Select prescaler = 1
    TMR2 = 0; //Clear Timer 4 register
    T3CONbits.TCS = 0; //Select internal clock
    PR2 = 1000000; //Load period Register - 80Hz frequency
    IFS0bits.T3IF = 0; //Clear Timer 2 interupt flag
    IPC3bits.T3IP = 6; //Set priority level to 6
    IPC3bits.T3IS = 2;
    IPC2bits.T2IP = 6;
    IPC2bits.T2IS = 2;
    IEC0bits.T3IE = 1; // Enable Timer 2
    T2CONbits.ON = 1; //Turn Timer  2 on.



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
    //These set the priority and subpriority to 4 and 3 respectively
    //This priority needs to be lower than the timer interrupts, at 5 and 6
    IPC6bits.CNIP=4;
    IPC6bits.CNIS=3;
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


        /* Current state logic here*/
//        switch (mode) {
//            case 1:
//                /* Pmod msd should show 's', and last 3 digits should show passcode */
//                displaySSD(passcode,5);
//                break;
//            case 2:
//                /* Pmod msd should show 'u', and last 3 digits should show off */
//                displaySSD(0,18);
//                break;
//            case 3:
//                /* Pmod msd should show 'L', and last 3 digits should show entered pass */
//                displaySSD(guess,17);
//                break;
//            case 4:
//                /* Pmod msd should Flash "AAAA" */
//                displaySSD(0,4);
//                break;
//        }

        /* Next state logic here*/
        switch (mode) {
            case 1:
                /* 0-9 should input digit, 'C' should clear, 'D' should delete, */
                /* 'E' enter if valid input pass*/
                if (key_to_react) {
                //perform various tests and actions on key accordingly
                    if(key_pressed){
                        key_pressed=0;
                        key_to_react = 0;
                }
                else if(key_released){
                    if ((key_detected >= 0 && key_detected <= 2) || (key_detected >= 4 && key_detected <= 6) || (key_detected >= 8 && key_detected <= 10) || key_detected == 12) {
                            if (passcode == 0) {
                                passcode = key_detected_toregint(key_detected);
                            } else if (passcode > 0 && passcode < 10) {
                                passcode = (10 * passcode) + key_detected_toregint(key_detected);
                            }
                            else if (passcode >=10 && passcode <100){
                                passcode = (10 * passcode) + key_detected_toregint(key_detected);
                            }
                        }
                        else if (isalpha(keypad_number[key_detected])) {

                            if (keypad_number[key_detected] == 'D') {
                                passcode = (passcode - (passcode % 10)) / 10;
                            }
                            if (keypad_number[key_detected] == 'C') {
                                passcode = 0;
                            }
                            if (keypad_number[key_detected] == 'E') {
                                if (passcode > 300 && passcode < 999) {
                                    guess = 0;
                                    mode = 2;
                                    clearSSDS();
                                }
                            }
                        }
                        key_released=0;
                        key_to_react = 0;
                }
            }
                break;
            case 2:
                /* if a button is pressed and held for >=1 second, go back to mode 1 */
                /* if a button is pressed and held for <1 second, go to mode 3 */
                if (key_to_react)
                {
                    if (key_released)
                    {
                        if(length_held<1){
                            mode=1;
                            key_to_react=0;
                            key_released=0;
                        }
                        if(length_held>=1){
                            mode=3;
                            key_to_react=0;
                            key_released=0;
                        }
                    }
                    else
                    {
                        key_to_react=0;
                        key_pressed=0;
                    }
                }
                break;
            case 3:
                /* 0-9 should input digit, 'C' should clear, 'D' should delete, */
                /* 'E' enter if valid input pass. If pass is correct, enter mode 2*/
                /* if pass incorrect, enter mode 4. If microphone frequency is equal */
                /* to pass frequency, enter mode 2, else to mode 4. */
                if (key_to_react) {
                //perform various tests and actions on key accordingly
                    if(key_released){
                        if ((key_detected >= 0 && key_detected <= 2) || (key_detected >= 4 && key_detected <= 6) || (key_detected >= 8 && key_detected <= 10) || key_detected == 12) {
                            if (guess == 0) {
                                guess = key_detected_toregint(key_detected);
                            } else if (guess > 0 && guess < 10) {
                                guess = (10 * guess) + key_detected_toregint(key_detected);
                            }
                            else if (guess >=10 && guess <100){
                                guess = (10 * guess) + key_detected_toregint(key_detected);
                            }
                    }
                        if (isalpha(keypad_number[key_detected])) {

                            if (keypad_number[key_detected] == 'D') {
                                guess = (guess - (guess % 10)) / 10;
                            }
                            if (keypad_number[key_detected] == 'C') {
                                guess = 0;
                            }
                            if (keypad_number[key_detected] == 'E') {
                                if (guess == passcode) {
                                    mode = 2;
                                    clearSSDS();
                                }
                                else{
                                    mode = 4;
                                    time_counter_ssd = 0;
                                    time_counter_seconds = 0;
                                }
                            }
                        }
                    key_to_react = 0;
                    key_released = 0;
                    }
                    else if(key_pressed){
                        key_pressed=0;
                        key_to_react=0;
                    }

            }
                break;
            case 4:
                /* If SSDs have been flashing for 5 seconds, then enter mode 3.*/
                if(time_counter_seconds==5){
                    mode = 3;
                    time_counter_ssd = 0;
                    time_counter_seconds = 0;
                }
                break;
        }
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