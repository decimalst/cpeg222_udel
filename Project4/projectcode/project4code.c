// CPEG222 - Project 2 Code
// Author: Byron Lambrou, Andrew Lipman
// Input: Onboard buttons, Microphone PMOD, IR LED sensors
// Output:  1xSSD Pmod, onboard LEDs, Servo motors
// Comments:

#include <p32xxxx.h>
#include <plib.h>
#include <stdlib.h>
#include "dsplib_dsp.h"
#include "fftc.h"

/* SYSCLK = 8MHz Crystal/ FPLLIDIV * FPLLMUL/ FPLLODIV = 80MHz
PBCLK = SYSCLK /FPBDIV = =10MHz*/
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_8

#define idleDutyCycleValue	0xEA5
#define rightDutyCycleValue	0x9C3
#define leftDutyCycleValue	0x1387

// Define ports used
//Onboard buttons
#define Btn1	PORTAbits.RA6
#define Btn2    PORTAbits.RA7

//Onboard LEDs
#define Led1    LATBbits.LATB10
#define Led2    LATBbits.LATB11
#define Led3    LATBbits.LATB12
#define Led4    LATBbits.LATB13

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

//Servos
#define Servo1 LATEbits.LATE8
#define Servo2 LATDbits.LATD0
#define Servo3 LATDbits.LATD8
#define Servo4 LATEbits.LATE9

//Variable declarations

//Functions definitions

//ISR definitions

void __ISR(_TIMER_5_VECTOR, ipl5) _T5Interrupt(void) {
	//This timer occurs at a 80Hz frequency, and should be used to flip the SSD
	//side display.
    IFS0CLR = 0x100000; // Clear Timer5 interrupt status flag (bit 20)
}

//main function call

main() 
{
	INTDisableInterrupts();

	//Set onboard buttons to inputs
	TRISA = 0xC0; //Btn1, Btn2 are input.

	//Set microphone input pin to analog.
	TRISB = 0x80F;

    //ADC manual config
    AD1PCFG = 0xF7FF; // all PORTB = digital but RB11 = analog
    AD1CON1 = 0; // manual conversion sequence control
    AD1CHS = 0x000B0000; // Connect RB7/AN7 as CH0 input
    AD1CSSL = 0; // no scanning required
    AD1CON2 = 0; // use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F02; // Tad = 128 x Tpb, Sample time = 31 Tad
    AD1CON1bits.ADON = 1; // turn on the ADC

    //Port C-G are output ports
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

    //Timer configured for SSD display at 80Hz speed.
    T5CONbits.ON = 0; // Stop timer, clear control registers
    TMR5 = 0; // Timer counter
    PR5 = 15624; //Timer count amount for interupt to occur - 2048Hz frequency
    IPC5bits.T5IP = 5; //prioirty 5
    IFS0bits.T5IF = 0; // clear interrupt flag
    T5CONbits.TCKPS = 2; // prescaler of 64, internal clock sourc
    T5CONbits.ON = 1; // Timer 5 module is enabled
    IEC0bits.T5IE = 1; //enable Timer 5

    
    //Code taken from pg 575 of reference manual for output compare
    OC1CON = 0x0000; //Turn off OC1 while doing setup
    OC1R = 0xEA5; //Initialize primary compare register
    OC1RS = 0xEA5; //Initialize secondary compare register
    OC1CON = 0x0006; //Configure for PWM mode
    PR2 = 0xC34F; //Set period

    IFS0CLR = 0x00000040; //clear the OC1 interrupt flag
    IEC0SET = 0x00000040; //Enable the OC1 interrupt
    IPC1SET = 0x001C0000; //Set priority to 7,
    IPC1SET = 0x00030000; //Set subpriority to 3

    T2CONSET = 0x8020; //Enable timer 2
    OC1CONSET = 0x8000; //Enable OC1

    INTEnableSystemMultiVectoredInt();

    while (1) 
    {





    }



}