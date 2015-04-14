// CPEG222 - Lab0 template
// Author: Byron Lambrou, Kathryn Black
// Input: One on-board button: Btn2
// Output:  PMod8LD: PLed1~PLed8
// There are some questions left for you in the code


#include <p32xxxx.h>
#include <plib.h>

// Configuration Bit settings (Don't touch them if not necessary)
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF
// Other options are don't care
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_2

// Q: What does this mean?
// A: This is the time defined to be the debounce time-
// When a button is pressed, a null for loop is run for this many
// cycles.
#define DEBOUNCE_TIME 200000

// Button
// This is an easy way to assign names to the pins, so that writing
// or reading is easier

#define Btn1	PORTAbits.RA6

// Define Pmod LEDs as PLed1~PLed8
// Q: Where to plug the pmod on the board and why?
// A: The pmod must be plugged into port B on the board.
// Port B can be used as both an analog and digital output port.
// Q: Why use "LAT" instead of "PORT" here? Hint: difference between Input and Output
// A: A LATch register holds data written to an IO port, whereas a PORT register
// just allows an IO pin status to be read.
// Notice: Ports of onboard LEDs are preassigned.You can find them on reference manual
#define PLed1   LATGbits.LATG9
#define PLed2   LATGbits.LATG8
#define PLed3   LATGbits.LATG7
#define PLed4   LATGbits.LATG6
#define PLed5   LATBbits.LATB15
#define PLed6   LATDbits.LATD5
#define PLed7   LATDbits.LATD4
#define PLed8   LATBbits.LATB14

#define BLed1   LATBbits.LATB10
#define BLed2   LATBbits.LATB11
#define BLed3   LATBbits.LATB12
#define BLed4   LATBbits.LATB13

void setPmodLed(int value) { // This function will be refered later
    PLed1 = (value & 0b10000000) >> 7;
    PLed2 = (value & 0b01000000) >> 6;
    PLed3 = (value & 0b00100000) >> 5;
    PLed4 = (value & 0b00010000) >> 4;
    PLed5 = (value & 0b00001000) >> 3;
    PLed6 = (value & 0b00000100) >> 2;
    PLed7 = (value & 0b00000010) >> 1;
    PLed8 = (value & 0b00000001);
}

void setBoardLed(int value) {
    BLed1 = (value & 0b0001);
    BLed2 = (value & 0b0010) >> 1;
    BLed3 = (value & 0b0100) >> 2;
    BLed4 = (value & 0b1000) >> 3;

}

int main(void) {

    // You can check the schematic/reference manual (all on Sakai) to figure out
    // which pin of the microcontroller is connected to which pin of board
    // The x's are do not matter, since we are not using those pins.

    /**************************************/
    /****Set PORTB to be digital/analog****/
    /**************************************/

    // This is only required for PORTB since it can also be used
    // This step is necessary!!!
    // as analog I/O. Setting the bits to 1 here will make the
    // corresponding pin to be defined as "digital", and 0 as "analog"
    // "0b1111 1111 1111 1111"
    AD1PCFG = 0xFFFF; // all PORTB as digital

    /*************************************/
    /****Set the direction of the pins****/
    /*************************************/

    // IMPORTANT: each pin you gonna use should be set to either INPUT (the pin you read value from) or OUTPUT (the pin you set value to)
    // It is not suggested to read an OUTPUT pin or write to an INPUT pin!

    // The direction is controlled by TRISX register
    // For example, "TRISB= 0x0000" sets all PORTB pins to OUTPUT. Actually since only B15 and B14 are used (PLed5 and PLed8),
    // more precisely we should use TRISB &=0x3FFF, if you don't want to change the other bits
    // (we don't do that since the other PORTB pins are "don't care cases" here).

    // Rules of bit manipulation:
    // OR'ing with 1 means that it is going to be 1: (0 | 1 = 1, 1 | 1 = 1)
    // AND'ing with 0 means that it is going to be 0 (0 & 1 =0, 1 & 0 = 0)
    // OR'ing with 0 means that it is going to preserve the previous value: (0 | 0 = 0, 1 | 0 = 1)
    // AND'ing with 1 means that it is going to preserve the previous value: (0 & 1 = 0, 1 & 1 = 1)

    // You can also set B15 and B14 to 0 by using TRISBCLR = 0xC000;
    // An opposite fuction is TRISBSET = 0xC000, which sets B15 and B14 as INPUT.
    TRISA = 0x70; // set Btn1 as INPUT
    TRISB = 0x0000;
    TRISD = 0x0000;
    TRISG = 0x0000;

    /*************************************************/
    /****Set the initial values of the OUTPUT pins****/
    /*************************************************/

    // Writing to a port is done by PORTX register
    // Make sure to understand how to set this in alternative way
    // (either OR'ing or AND'ing)
    // something like PORTB &= xxxx; or PORTB |= xxxx;
    // Also you can write to these LEDs with the predefined values:
    // LATBbits.LATB10=0;  or:
    // PLed1 = 0;
    PORTB = 0x0000;
    PORTD = 0x0000;
    TRISG = 0x0000;

    /******************************************/
    /****Define the variables you gonna use****/
    /******************************************/

    unsigned int i = 0;
    unsigned int no_hit = 0;
    unsigned int score = 0;
    unsigned int right = 1;
    unsigned int time_counter = 0; // Q: Why we need this counter?
    unsigned short mode = 1; // determine the mode
    unsigned short buttonLock = 0; // Q: Why do we need this variable?
    unsigned int threshold = 50000; // Q: What does this do?
    unsigned int display_number = 0; //Used as position variable


    // Main Loop
    // For all the projects you need an infinite loop to keep the program running
    while (1) {

        /* Current State Logic */
        if (mode == 1) {
            ; //do nothing
        } else if (mode == 2) {
            time_counter++;
            if (time_counter > threshold) { // What will happen if we change the value of threshold?
                //Threshold determines the speed at which the ball will move.
                time_counter = 0;
                if (right == 1) {
                    display_number = (display_number == 0x00) ? display_number : ((display_number >> 1)); // Try to understand what is it doing. Hint:Ternary operator
                }
                if (right == 0) {
                    display_number = (display_number == 0x80) ? display_number : ((display_number << 1)); // Try to understand what is it doing.  Hint:Ternary operator
                }
            }
        } else { //Q: what is the mode? A: Mode 3, hit or lose
            time_counter++;
            if (time_counter > threshold) {
                time_counter = 0;
                no_hit = 1;

            }
        }
        setPmodLed(display_number);

        /* Next State Logic */
        if (Btn1 && !buttonLock && mode!=2) { // Check button status
            buttonLock = 1;
            for (i = 0; i < DEBOUNCE_TIME; i++); // Why need this code? This is used for debouncing the analog input bounces.
            if (mode == 1) {
                time_counter = 0;
                mode = 2;
                display_number = 0x80;
            } else if (mode == 2) {
                ;
            } else if (mode == 3 && no_hit == 0) {

                mode = 2;
                right = 1;
                score = (score == 15) ? score : ((score = score + 1));
                setBoardLed(score);
                display_number = 0x80;
            }
        } else if (!Btn1 && buttonLock && mode!=2) { // Check button status
            for (i = 0; i < DEBOUNCE_TIME; i++);
            buttonLock = 0; // Why need this code?
            //The use of a button lock allows us to ensure that a state change
            //only occurs once per button press, rather than multiple times if held
            //down.
        }
        if (display_number == 0x01) {
            right = 0;
        }
        if (display_number == 0x80 && right == 0) {
            mode = 3; //Transition to hit or die mode
        }
        if (no_hit == 1) {//if player doesn't hit in time, reset
            no_hit = 0;
            score = 0;
            mode = 1;
            display_number = 0;
            right = 1;
        }
    } // end of while loop
} // end of the main function