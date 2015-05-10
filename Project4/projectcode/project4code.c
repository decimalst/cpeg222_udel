// CPEG222 - Project 2 Code
// Author: Byron Lambrou, Andrew Lipman
// Input: Onboard buttons, Microphone PMOD, IR LED sensors
// Output:  1xSSD Pmod, onboard LEDs, Servo motors
// Comments:

#include <p32xxxx.h>
#include <plib.h>
#include <stdlib.h>


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


//Light Sensors Top of JC
#define LS1 PORTGbits.RG12
#define LS2 PORTGbits.RG13
#define LS3 PORTGbits.RG14
#define LS4 PORTGbits.RG15

#define modeStraight    1
#define modeLeft        2
#define modeRight       3
// The counter speed
#define DEBOUNCE_TIME 200

//Variable declarations

//State variables
int mode = 2; //Mode 1=idle, mode 2=movement, mode3=victory/reset

int modeMoving = modeStraight;

unsigned short button1Lock = 0;
unsigned short button2Lock = 0;
int i;


//Used for average noise level
int noiseAverage = 400;
int noiseAverageArray[50];
int noiseArrayIndex = 0;


//Functions definitions

void noiseAverager() {
    /* Calculates the new average noise level when the noiseAverageArray */
    /* is full of new samples. */
    /* Should be called every 20th interrupt. */
    int newAvg = 0;
    int i;
    for (i = 0; i < 50; i++) {
        newAvg += noiseAverageArray[i];
    }
    noiseAverage = (newAvg / 50);
}

//ISR definitions

void __ISR(_TIMER_5_VECTOR, ipl5) _T5Interrupt(void) {
    //This timer occurs at a 80Hz frequency, and should be used to flip the SSD
    //side display.
    IFS0CLR = 0x100000; // Clear Timer5 interrupt status flag (bit 20)
}

void __ISR(_OUTPUT_COMPARE_1_VECTOR, ipl7) OC1_IntHandler(void) {
    // insert user code here
    IFS0CLR = 0x0040; // Clear the OC1 interrupt flag
}

void __ISR(_OUTPUT_COMPARE_2_VECTOR, ipl7) OC2_IntHandler(void) {
    // insert user code here
    IFS0CLR = 0x0040; // Clear the OC1 interrupt flag
}

//main function call

main() {
    INTDisableInterrupts();

    //Set onboard buttons to inputs
    TRISA = 0xC0; //Btn1, Btn2 are input.

    //Set microphone input pin to analog.
    TRISB = 0x08F0;

    //ADC manual config
    AD1PCFG = 0xF7FF; // all PORTB = digital but RB11 = analog
    AD1CON1 = 0; // manual conversion sequence control
    AD1CHS = 0x000B0000; // Connect RB7/AN7 as CH0 input
    AD1CSSL = 0; // no scanning required
    AD1CON2 = 0; // use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F02; // Tad = 128 x Tpb, Sample time = 31 Tad
    AD1CON1bits.ADON = 1; // turn on the ADC

    //Port C-G are output ports
    //Light sensors are inputs
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0xF000;
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
    //Use for left servo
    OC3CON = 0x0000; //Turn off OC1 while doing setup
    OC3R = 0xEA5; //Initialize primary compare register
    OC3RS = 0xEA5; //Initialize secondary compare register
    OC3CON = 0x0006; //Configure for PWM mode
    PR2 = 0xC34F; //Set period

    IFS0CLR = 0x00004000; //clear the OC1 interrupt flag
    IEC0SET = 0x00004000; //Enable the OC1 interrupt
    IPC3SET = 0x001C0000; //Set priority to 7,
    IPC3SET = 0x00030000; //Set subpriority to 3

    T2CONSET = 0x8020; //Enable timer 2
    OC3CONSET = 0x8000; //Enable OC1

    //Code taken from pg 575 of reference manual for output compare
    //Use for right servo
    OC2CON = 0x0000; //Turn off OC1 while doing setup
    OC2R = 0xEA5; //Initialize primary compare register
    OC2RS = 0xEA5; //Initialize secondary compare register
    OC2CON = 0x0006; //Configure for PWM mode
    PR2 = 0xC34F; //Set period

    IFS0CLR = 0x00000800; //OC2 interrupt flag is bit 10
    IEC0SET = 0x00000800; //Enable the OC1 interrupt on bit 10
    IPC2SET = 0x001C0000; //OC2 priority is bits18-20
    IPC2SET = 0x00030000; //OC2 priority is bits16-17
    //Left out the timer 2 con part here, since they both can
    //use it, no need to configure twice.
    OC2CONSET = 0x8000; //Enable OC1

    INTEnableSystemMultiVectoredInt();

    while (1) {
        /* Current state logic */
        switch (mode) {
            case 2:
                //Check Light Sensors
                Led1 = LS1;
                Led2 = LS2;
                Led3 = LS3;
                Led4 = LS4;

                //In mode 2, we move the robot and light LEDS according to sensors
                if (modeMoving == modeStraight) {
                    OC2RS = 0x9C3;
                    OC3RS = 0x1387;
                } else if (modeMoving == modeRight) {
                    OC2RS = 0xEA5;
                    OC3RS = 0x1387;
                } else if (modeMoving == modeLeft) {
                    OC2RS = 0x9C3;
                    OC3RS = 0xEA5;
                }
                /* Next State Logic */
                //ignore press if not at place where it matters

                if ((Btn1) && !button1Lock) { // Check button status
                    button1Lock = 1;
                    mode = modeRight;
                }

                if ((Btn2) && !button2Lock) { // Check button status
                    button2Lock = 1;
                    mode = modeLeft;


                } else if (!(Btn1) && button1Lock) { // Check button status
                    for (i = 0; i < DEBOUNCE_TIME; i++);
                    button1Lock = 0; // stop multiple presses
                    modeMoving = modeStraight;
                } else if (!(Btn2) && button2Lock) { // Check button status
                    for (i = 0; i < DEBOUNCE_TIME; i++);
                    button2Lock = 0; // stop multiple presses
                    modeMoving = modeStraight;
                }






                break;
            case 1:
                //In mode 1, we do nothing
                break;
            case 3:
                //In mode 3, we do nothing
                break;
        }

        /* Next state logic */
        switch (mode) {
            case 2:
                //In mode 2, we change the servo duty cycles depending on sensors

                break;
            case 1:
                //In mode 1, we check for noise, or button press
                break;
            case 3:
                //In mode 3, check for a button to reset the robot.
                break;
        }



    }



}
