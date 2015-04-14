// CPEG222 - Project 2 Code
// Author: Byron Lambrou, Kathryn Black
// Input: Keypad PMod
// Output:  2xSSD pmods, and 4 onboard LEDs
// Comments:
#include<p32xxxx.h>
#include<plib.h>
#include<ctype.h>
#include<stdlib.h>
#include<time.h>

#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_2

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


int forLoop = 0;
int forLoop_blink = 0;
int trigger = 0;
int left_ssd_val = 1;

unsigned int dummy;
int key_to_react = 0;
int pressed_key;
int number_of_keys = 0;
int key_detected;

int secret_Number;
int lower_limit = 0;
int upper_limit = 99;
int mode = 1;
int turn = 0;
int guess;
int number_of_players = 0;
unsigned int timeseed=3400;



void setBoardLed(int value) {
    BLed1 = (value & 0b0001);
    BLed2 = (value & 0b0010) >> 1;
    BLed3 = (value & 0b0100) >> 2;
    BLed4 = (value & 0b1000) >> 3;

}
void setBoardLed_player(int value){
    if(value==1){
        setBoardLed(1);
    }
    if(value==2){
        setBoardLed(2);
    }
    if(value==3){
        setBoardLed(4);
    }
    if(value==4){
        setBoardLed(8);
    }
}

//Lookup table for SSD digits
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
    'D' //15
};

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

void clearSSDS() {
    displayDigit(0b0000000, 0, 0);
    displayDigit(0b0000000, 1, 0);
    displayDigit(0b0000000, 0, 1);
    displayDigit(0b0000000, 1, 1);
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

//Calls displayDigit using the SSD_number lookup table to convert the base 10 int input
//into binary input

void showNumber(int digit, unsigned int left_ssd_num, unsigned int leftdisp) {
    displayDigit(SSD_number[digit % 16], left_ssd_num, leftdisp);
}

void __ISR(_CHANGE_NOTICE_VECTOR, ipl5) ChangeNotice_Handler(void) {
    // 1. Disable interrupts
    INTDisableInterrupts();
    // 2. Debounce keys
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

main() {
	
    INTDisableInterrupts();
    srand(time(NULL));
    //Configure ports C~G to be output ports
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;
    TRISG = 0;
    // initialize C~G to 0
    PORTC = 0x00;
    PORTD = 0x00;
    PORTE = 0x00;
    PORTF = 0x00;
    PORTG = 0x00;
    //Set the pins that correspond to the Column pins to inputs
    TRISB = 0b0000000000001111;
    //Then clear everything but their pins
    PORTBCLR = 0xC;
    //Set port B to digital
    AD1PCFG = 0xFFFF;
	
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

    while (1) {
        //current state logic

        if (mode == 1) {
            //in Mode 1, pressing any key will enter Mode 2(config)
            //onboard LEDs should be at 0, and SSDs should show nothing
            setBoardLed(0);
        }
        if (mode == 2) {
            //in Mode 2, we are configuring the number of players.
            //SSD 1 should show 'CO'
            //SSD 2 should show the entered number of players
            //C should clear input, D should delete
            //Pressing F should set secret number to our decided number and enter mode 3(guess)
            //Pressing E should set secret number randomly and enter mode 3(Guess)
            //In both cases, the current input must be from 1-4 inclusive to enter mode 3
            //Set N_L to 0, N_H to 99
            if (left_ssd_val) {
                while (forLoop < 100) {
                    displayDigit(SSD_number[12], 1, 0);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (!left_ssd_val) {
                while (forLoop < 500) {
                    displayDigit(SSD_number[0], 1, 1);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (left_ssd_val) {
                while (forLoop < 400) {
                    displayDigit(SSD_number[number_of_players / 10], 0, 0);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (left_ssd_val) {
                while (forLoop < 100) {
                    displayDigit(SSD_number[number_of_players % 10], 0, 1);
                    forLoop++;
                }
                forLoop = 0;
            }

        }
        if (mode == 3) {
            //in Mode 3, the current player enters a guess
            //the current player's turn is decided by turn%4
            //onboard LEDs should show the current player's turn
            //the keypad numbers are used for input, and C should clear, D delete
            //E should enter guess if it is from N_L to N_H
            //F should enter mode 4(Range)
            setBoardLed_player((turn % number_of_players) + 1);
            if (left_ssd_val) {
                while (forLoop < 100) {
                    displayDigit(SSD_number[guess / 10], 0, 0);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (!left_ssd_val) {
                while (forLoop < 200) {
                    displayDigit(SSD_number[guess % 10], 0, 1);
                    forLoop++;
                }
                forLoop = 0;
            }


        }
        if (mode == 4) {
            //In Mode 4, pmod SSD 1 should show the current lowest,
            //and pmod SSD 2 should show the current highest
            //onboard LED should still show the current player
            //and any button pressed on the keypad should go back to mode 3
            if (left_ssd_val) {
                while (forLoop < 100) {
                    displayDigit(SSD_number[lower_limit / 10], 1, 0);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (!left_ssd_val) {
                while (forLoop < 400) {
                    displayDigit(SSD_number[lower_limit % 10], 1, 1);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (left_ssd_val) {
                while (forLoop < 400) {
                    displayDigit(SSD_number[upper_limit / 10], 0, 0);
                    forLoop++;
                }
                forLoop = 0;
            }
            if (left_ssd_val) {
                while (forLoop < 100) {
                    displayDigit(SSD_number[upper_limit % 10], 0, 1);
                    forLoop++;
                }
                forLoop = 0;
            }
        }
        if (mode == 5) {
            //in Mode 5, if guess is greater than the secret number, SSD1 display HH
            //If lower, display LL
            //If equal to secret number, display EE and flash at 2Hz.
            //SSD 2 display the guess, and if equal to secret number flash at 2Hz.
            //onboard LEDS should still show current player
            //If guess is correct, any key should enter Mode 1.
            //If guess is incorrect, any key should enter Mode 3.
            if (guess == secret_Number) {
                    if (left_ssd_val) {
                        while (forLoop < 100) {
                            displayDigit(SSD_number[14], 1, 0);
                            forLoop++;
                            forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (!left_ssd_val) {
                        while (forLoop < 500) {
                            displayDigit(SSD_number[14], 1, 1);
                            forLoop++;
                            forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (left_ssd_val) {
                        while (forLoop < 400) {
                            displayDigit(SSD_number[guess / 10], 0, 0);
                            forLoop++;
                            forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (left_ssd_val) {
                        while (forLoop < 100) {
                            displayDigit(SSD_number[guess % 10], 0, 1);
                            forLoop++;
                            forLoop_blink++;
                        }
                        forLoop = 0;
                    }

                    forLoop = 0;
                    while (forLoop_blink > 20000 && forLoop_blink < 35000) {
                        clearSSDS();
                        trigger = 1;
                        forLoop_blink++;
                    }
                    if (trigger) {
                        trigger = 0;
                        forLoop_blink = 0;
                    }
                    forLoop = 0;
            }
            if (guess != secret_Number) {
                if (guess > secret_Number) {
                    if (left_ssd_val) {
                        while (forLoop < 100) {
                            displayDigit(SSD_number[16], 1, 0);
                            forLoop++;
                            //forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (!left_ssd_val) {
                        while (forLoop < 500) {
                            displayDigit(SSD_number[16], 1, 1);
                            forLoop++;
                            //forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (left_ssd_val) {
                        while (forLoop < 400) {
                            displayDigit(SSD_number[guess / 10], 0, 0);
                            forLoop++;
                            //forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (left_ssd_val) {
                        while (forLoop < 100) {
                            displayDigit(SSD_number[guess % 10], 0, 1);
                            forLoop++;
                            //forLoop_blink++;
                        }
                        forLoop = 0;
                    }
					/*uncomment this and all the forLoop_blink increments for cool
					blinking action*/
                    //forLoop = 0;
                    //while (forLoop_blink > 20000 && forLoop_blink < 35000) {
                    //    clearSSDS();
                    //    trigger = 1;
                    //    forLoop_blink++;
                    //}
                    //if (trigger) {
                    //    trigger = 0;
                    //    forLoop_blink = 0;
                    //}
                    forLoop = 0;


                }
                if (guess < secret_Number) {
                    if (left_ssd_val) {
                        while (forLoop < 100) {
                            displayDigit(SSD_number[17], 1, 0);
                            forLoop++;
                        //    forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (!left_ssd_val) {
                        while (forLoop < 500) {
                            displayDigit(SSD_number[17], 1, 1);
                            forLoop++;
                        //    forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (left_ssd_val) {
                        while (forLoop < 400) {
                            displayDigit(SSD_number[guess / 10], 0, 0);
                            forLoop++;
                        //    forLoop_blink++;
                        }
                        forLoop = 0;
                    }
                    if (left_ssd_val) {
                        while (forLoop < 100) {
                            displayDigit(SSD_number[guess % 10], 0, 1);
                            forLoop++;
                        //    forLoop_blink++;
                        }
                        forLoop = 0;
                    }

                    forLoop = 0;
                    //while (forLoop_blink > 20000 && forLoop_blink < 35000) {
                    //    clearSSDS();
                    //    trigger = 1;
                    //    forLoop_blink++;
                    //}
                    //if (trigger) {
                    //    trigger = 0;
                    //    forLoop_blink = 0;
                    //}
                    forLoop = 0;
                }
            }
        }

        //next state logic
        if (mode == 1) {
            if (key_to_react) {
                mode = 2;
                key_to_react = 0;
            }
        }
        if (mode == 2) {
            if (key_to_react) {
                //perform various tests and actions on key accordingly
                if ((key_detected >= 0 && key_detected <= 2) || (key_detected >= 4 && key_detected <= 6) || (key_detected >= 8 && key_detected <= 10) || key_detected == 12) {
                    if (number_of_players == 0) {
                        number_of_players = key_detected_toregint(key_detected);
                    } else if (number_of_players > 0 && number_of_players < 10) {
                        number_of_players = (10 * number_of_players) + key_detected_toregint(key_detected);
                    }
                }
                if (isalpha(keypad_number[key_detected])) {

                    if (keypad_number[key_detected] == 'C') {
                        number_of_players = (number_of_players - (number_of_players % 10)) / 10;
                    }
                    if (keypad_number[key_detected] == 'D') {
                        number_of_players = 0;
                    }
                    if (keypad_number[key_detected] == 'E') {
                        if (number_of_players > 0 && number_of_players < 5) {
                            secret_Number = rand() % 100;
                            guess = 0;
                            mode = 3;
                            clearSSDS();
                        }
                    }
                    if (keypad_number[key_detected] == 'F') {
                        if (number_of_players > 0 && number_of_players < 5) {
                            secret_Number = 42;
                            guess = 0;
                            clearSSDS();
                            mode = 3;
                        }
                    }
                }
                key_to_react = 0;
            }

        }
        if (mode == 3) {
            if (key_to_react) {
                //perform various tests and actions on key accordingly
                if ((key_detected >= 0 && key_detected <= 2) || (key_detected >= 4 && key_detected <= 6) || (key_detected >= 8 && key_detected <= 10) || key_detected == 12) {
                    if (guess == 0) {
                        guess = key_detected_toregint(key_detected);
                    } else if (guess > 0 && guess < 10) {
                        guess = (10 * guess) + key_detected_toregint(key_detected);
                    }
                }
                if (isalpha(keypad_number[key_detected])) {

                    if (keypad_number[key_detected] == 'C') {
                        guess = (guess - (guess % 10)) / 10;
                    }
                    if (keypad_number[key_detected] == 'D') {
                        guess = 0;
                    }
                    if (keypad_number[key_detected] == 'E') {
                        if (guess >= lower_limit && guess <= upper_limit) {
                            //Go to mode 5
                            clearSSDS();
                            mode = 5;
                        }
                    }
                    if (keypad_number[key_detected] == 'F') {
                        //Go to mode 4
                        clearSSDS();
                        mode = 4;
                    }
                }
                key_to_react = 0;
            }

        }
        if (mode == 4) {
            if (key_to_react) {
                //perform various test and actions on key
                mode = 3;
                clearSSDS();
                key_to_react = 0;
            }

        }
        if (mode == 5) {
            if (key_to_react) {
                //perform various tests and actions on key
                if (guess == secret_Number) {
                    //go back to initial;
                    number_of_players = 0;
                    guess = 0;
                    turn = 0;
                    trigger=0;
                    lower_limit = 0;
                    upper_limit = 99;
                    mode=1;
                    clearSSDS();
                }
                if (guess != secret_Number) {
                    turn++;
                    if(guess>secret_Number){
                        upper_limit=guess;
                    }
                    if(guess<secret_Number){
                        lower_limit=guess;
                    }
                    guess=0;
                    //trigger=0;
                    mode=3;
                    clearSSDS();

                }
                key_to_react = 0;
            }

        }
        if (left_ssd_val == 1) {
            left_ssd_val = 0;
        } else if (left_ssd_val == 0) {
            left_ssd_val = 1;
        }
    }





}