/*
 * File:   PWMGenerator.c
 * Author: andrewlipman
 *
 * Created on April 30, 2015, 3:21 PM
 */

#include <p32xxxx.h>
#include <plib.h>
#include <ctype.h>
#include <stdlib.h>
#include "dsplib_dsp.h"
#include "fftc.h"

/* SYSCLK = 8MHz Crystal/ FPLLIDIV * FPLLMUL/ FPLLODIV = 80MHz
PBCLK = SYSCLK /FPBDIV = 10MHz*/
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_8

// Define the Buttons
#define Btn1	PORTAbits.RA6
#define Btn2    PORTAbits.RA7

#define Servo1 LATEbits.LATE8
#define Servo2 LATDbits.LATD0
#define Servo3 LATDbits.LATD8
#define Servo4 LATEbits.LATE9

#define Led1    LATBbits.LATB10
#define Led2    LATBbits.LATB11
#define Led3    LATBbits.LATB12
#define Led4    LATBbits.LATB13



#define modeStraight    1
#define modeLeft        2
#define modeRight       3
// The counter speed
#define DEBOUNCE_TIME 200

 unsigned char SSD_number[] = {
    0b0111111, //0 -
    0b0000110, //1
    0b1011011, //2
    0b1001111, //3
    0b1100110, //4
    0b1101101, //5
    0b1111101, //6
    0b0000111, //7
    0b1111111, //8
    0b1101111, //9
    0b1110111, //A -10
    0b1111100, //B -11
    0b0111001, //C -12
    0b1011110, //D -13
    0b1111001, //E -14
    0b1110001, //F -15
    0b0000000 //clear -16
};

int mode = modeStraight;

unsigned short button1Lock = 0;
unsigned short button2Lock = 0;
int i;

void __ISR(_OUTPUT_COMPARE_1_VECTOR, ipl7) OC1_IntHandler(void) {
    // insert user code here
    IFS0CLR = 0x0040; // Clear the OC1 interrupt flag
}




main() {

    INTDisableInterrupts();

    //Set buttons to inputs
    TRISA = 0xC0; // set Btn1 and Btn2 as INPUT

    //Configure ports C~G to be output ports
    TRISB = 0;
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0;

    //Set ports to digital
    AD1PCFG = 0xFFFF;

    // initialize C~G to 0
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;
    PORTF = 0x00;
    PORTG = 0x00;

    //Code taken from pg 575 of the reference manual
    //Our peripheral bus clock is 10MHz, currently, so we want
    
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

    while (1) {

        // Current State Logic

        if (mode == modeStraight) {
            OC1RS = 0xEA5;
        } else if (mode == modeRight) {
            OC1RS = 0x9C3;
        } else if (mode == modeLeft) {
            OC1RS = 0x1387;
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
            mode = modeStraight;
        } else if (!(Btn2) && button2Lock) { // Check button status
            for (i = 0; i < DEBOUNCE_TIME; i++);
            button2Lock = 0; // stop multiple presses
            mode = modeStraight;
        }



    } // end of while loop
}

