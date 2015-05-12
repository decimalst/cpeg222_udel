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

#define idleDutyCycleValueRight	0xE9C
#define hardrightDutyCycleValueRight	0x9C3
#define hardleftDutyCycleValueRight	0x1387
#define softrightDutyCycleValueRight    0x9C3
#define softleftDutyCycleValueRight 0x1387

#define idleDutyCycleValueLeft 0xEA0
#define hardrightDutyCycleValueLeft    0x9C3
#define hardleftDutyCycleValueLeft	0x1387
#define softrightDutyCycleValueLeft    0x9C3
#define softleftDutyCycleValueLeft  0x1387

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
#define LS1Left PORTGbits.RG12
#define LS2CenterLeft PORTGbits.RG13
#define LS3CenterRight PORTGbits.RG14
#define LS4Right PORTGbits.RG15

#define leftServoPWM OC3RS
#define rightServoPWM OC2RS

#define modeStraight    1
#define modeLeft        2
#define modeRight       3
// The counter speed
#define DEBOUNCE_TIME 200

//Variable declarations

//State variables
int mode = 1; //Mode 1=idle, mode 2=movement, mode3=victory/reset

int modeMoving = modeStraight;

unsigned short button1Lock = 0;
unsigned short button2Lock = 0;
int i;

//Used for SSD display
int SSDleft = 1;
int counter = 0;
int distanceTravelled = 0;
int timeMoved = 0;

//Used for average noise level
int wtf = 0;
int temp = 0;
int noiseAverage = 700;
int noiseAverageTemp = 0;
int noiseIndex = 0;

//Used for SSD display
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


//Functions definitions

int readADC() {
    AD1CON1bits.SAMP = 1; // 1. start sampling
    for (TMR1 = 0; TMR1 < 100; TMR1++); //2. wait for sampling time
    AD1CON1bits.SAMP = 0; // 3. start the conversion
    while (!AD1CON1bits.DONE); // 4. wait conversion complete
    return ADC1BUF0; // 5. read result

}

void displayDigit(unsigned char value, unsigned int leftdisp) {
    //For left=1, display on left SSD, else, on right SSD
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

void showNumber(int digit) {
    if (SSDleft) {
        displayDigit(SSD_number[digit % 10], 1);
    } else {
        displayDigit(SSD_number[digit / 10], SSDleft);
    }


}

void clearSSDS() {
    displayDigit(0b0000000, 0);
    displayDigit(0b0000000, 1);
}

//ISR definitions

void __ISR(_TIMER_5_VECTOR, ipl4) _T5Interrupt(void) {
    //This timer occurs at a 80Hz frequency, and should be used to flip the SSD
    //side display.
    SSDleft = !(SSDleft);
    IFS0CLR = 0x100000; // Clear Timer5 interrupt status flag (bit 20)
}

void __ISR(_TIMER_4_VECTOR, ipl5) _T4Interrupt(void) {
    //This timer occurs at a 30Hz frequency, and should be used to sample ADC
    //and add to average.
    temp = readADC();
    noiseAverageTemp += temp;
    noiseIndex++;
    if (noiseIndex % 30 == 0) {
        counter++;
    }
    if (noiseIndex == 150) {
        noiseAverage = 40 + (noiseAverageTemp / 150);
        noiseIndex = 0;
        noiseAverageTemp = 0;
    }

    IFS0bits.T4IF = 0; // Clear Timer5 interrupt status flag (bit 20)
}

void __ISR(_OUTPUT_COMPARE_2_VECTOR, ipl7) OC2_IntHandler(void) {
    // insert user code here
    IFS0CLR = 0x00000800; // Clear the OC2 interrupt flag

}

void __ISR(_OUTPUT_COMPARE_3_VECTOR, ipl7) OC3_IntHandler(void) {
    // insert user code here
    IFS0CLR = 0x00004000; // Clear the OC3 interrupt flag
}

//main function call

main() {
    INTDisableInterrupts();

    //Set onboard buttons to inputs
    TRISA = 0xC0; //Btn1, Btn2 are input.

    //Set microphone pin to input.
    TRISB = 0x000F;

    //ADC manual config
    AD1PCFG = 0xFFFD; // all PORTB = digital but RB11 = analog
    AD1CON1 = 0; // manual conversion sequence control
    AD1CHS = 0x00010000; // Connect RB7/AN7 as CH0 input
    AD1CSSL = 0; // no scanning required
    AD1CON2 = 0; // use MUXA, AVss/AVdd used as Vref+/-
    AD1CON3 = 0x1F02; // Tad = 128 x Tpb, Sample time = 31 Tad
    AD1CON1bits.ADON = 1; // turn on the ADC

    //Port C-G are output ports
    //Light sensors are inputs 12-15 on G
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0xF000;
    // initialize C~G to 0
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;
    PORTF = 0x00;
    PORTG = 0x00;

    //Timer configured for SSD display at 80Hz speed.
    T5CONbits.ON = 0; // Stop timer, clear control registers
    TMR5 = 0; // Timer counter
    PR5 = 15624; //Timer count amount for interupt to occur - 80Hz frequency
    IPC5bits.T5IP = 4; //prioirty 5
    IPC5bits.T5IS = 1;
    IFS0bits.T5IF = 0; // clear interrupt flag
    T5CONbits.TCKPS = 2; // prescaler of 64, internal clock sourc
    T5CONbits.ON = 1; // Timer 5 module is enabled
    IEC0bits.T5IE = 1; //enable Timer 5
    //Timer configured for microphone sampling in Mode1. 30Hz frequency
    //30Hz = 80MHz/(prescale*(PR + 1))
    T4CONbits.ON = 0; //Stoptimer,
    TMR4 = 0;
    PR4 = 41665; // Period 4, for 30Hz event frequency
    IPC4bits.T4IP = 5;
    IPC4bits.T4IS = 2;
    IFS0bits.T4IF = 0;
    T4CONbits.TCKPS = 2;
    T4CONbits.ON = 1;
    IEC0bits.T4IE = 1;



    //Code taken from pg 575 of reference manual for output compare
    //Use for right servo
    OC2CON = 0x0000; //Turn off OC while doing setup
    OC2R = 0xE9C; //Initialize primary compare register
    OC2RS = 0xE9C; //Initialize secondary compare register
    OC2CON = 0x0006; //Configure for PWM mode
    PR2 = 0xC34F; //Set period

    IFS0CLR = 0x00000800; //OC2 interrupt flag is bit 10
    IEC0SET = 0x00000800; //Enable the OC1 interrupt on bit 10
    IPC2SET = 0x001C0000; //OC2 priority is bits18-20
    IPC2SET = 0x00030000; //OC2 priority is bits16-17
    //Left out the timer 2 con part here, since they both can
    //use it, no need to configure twice.


    //Code taken from pg 575 of reference manual for output compare
    //Use for left servo
    //For OC3, the servo turns slightly backwards when idle set to 0xEA5.
    OC3CON = 0x0000; //Turn off OC3 while doing setup
    OC3R = 0xEA0; //Initialize primary compare register
    OC3RS = 0xEA0; //Initialize secondary compare register
    OC3CON = 0x0006; //Configure for PWM mode
    PR2 = 0xC34F; //Set period

    IFS0CLR = 0x00004000; //clear the OC3 interrupt flag
    IEC0SET = 0x00004000; //Enable the OC3 interrupt
    IPC3SET = 0x001C0000; //Set priority to 7,
    IPC3SET = 0x00030000; //Set subpriority to 3

    T2CONSET = 0x8020; //Enable timer 2

    OC2CONSET = 0x8000; //Enable OC1
    OC3CONSET = 0x8000; //Enable OC3
    wtf = readADC();


    INTEnableSystemMultiVectoredInt();

    while (1) {
        /* Current state logic */
        switch (mode) {
            case 2:
                //Check Light Sensors
                Led1 = LS1Left;
                Led2 = LS2CenterLeft;
                Led3 = LS3CenterRight;
                Led4 = LS4Right;
                showNumber(distanceTravelled);

                //In mode 2, we move the robot and light LEDS according to sensors
                #define idleDutyCycleValueRight	0xE9C
#define hardrightDutyCycleValueRight	0x9C3
#define hardleftDutyCycleValueRight	0x1387

#define idleDutyCycleValueLeft 0xEA0
#define hardrightDutyCycleValueLeft    0x9C3
#define hardleftDutyCycleValueLeft	0x1387
                if(!(LS1Left) && !(LS2CenterLeft) && !(LS3CenterRight) && !(LS4Right)){
                    leftServoPWM = idleDutyCycleValueLeft;
                    rightServoPWM = idleDutyCycleValueRight;
                }
                else if (!LS4Right && (LS1Left)) {
                    rightServoPWM = idleDutyCycleValueRight;
                    leftServoPWM = hardleftDutyCycleValueLeft;
                }
                else if (!LS1Left && (LS4Right)) {
                    rightServoPWM = hardrightDutyCycleValueRight;
                    leftServoPWM = idleDutyCycleValueLeft;
                }
                else if (!LS2CenterLeft && !LS3CenterRight) {
                    leftServoPWM = hardleftDutyCycleValueRight;
                    rightServoPWM = hardrightDutyCycleValueLeft;
                }
                break;
            case 1:
                //In mode 1, we do nothing
                showNumber(noiseAverage /10);
                break;
            case 3:
                //In mode 3, we do nothing
                break;
        }

        /* Next state logic */
        switch (mode) {
            case 2:
                //In mode 2, we change the servo duty cycles depending on sensors
                //ignore press if not at place where it matters

                if ((Btn1) && !button1Lock) { // Check button status
                    button1Lock = 1;
                    modeMoving = modeRight;
                }

                if ((Btn2) && !button2Lock) { // Check button status
                    button2Lock = 1;
                    modeMoving = modeLeft;


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
                //In mode 1, we check for noise, or button press
                if (Btn1) {
                    mode = 2;
                }
                if (counter > 2) {
                    if (temp > noiseAverage) {
                        mode = 2;
                        T4CONbits.ON = 1;
                        IEC0bits.T4IE = 1;
                    }
                }
                break;
            case 3:
                //In mode 3, check for a button to reset the robot.
                break;
        }



    }



}
