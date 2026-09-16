/*
 * File:   DigClock_main_Eg.c
 * Author: Egeree Gemta
 *
 * Created on January 3, 2026, 10:31 PM
 */


#include <p24FJ64GA002.h>

#include "xc.h"
#include "gemta_lab3_ctoasm_v001.h"

// CW1: FLASH CONFIGURATION WORD 1 (see PIC24 Family Reference Manual 24.1)
#pragma config ICS = PGx1          // Comm Channel Select (Emulator EMUC1/EMUD1 pins are shared with PGC1/PGD1)
#pragma config FWDTEN = OFF        // Watchdog Timer Enable (Watchdog Timer is disabled)
#pragma config GWRP = OFF          // General Code Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF           // General Code Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = OFF        // JTAG Port Enable (JTAG port is disabled)


// CW2: FLASH CONFIGURATION WORD 2 (see PIC24 Family Reference Manual 24.1)
#pragma config I2C1SEL = PRI       // I2C1 Pin Location Select (Use default SCL1/SDA1 pins)
#pragma config IOL1WAY = OFF       // IOLOCK Protection (IOLOCK may be changed via unlocking seq)
#pragma config OSCIOFNC = ON       // Primary Oscillator I/O Function (CLKO/RC15 functions as I/O pin)
#pragma config FCKSM = CSECME      // Clock Switching and Monitor (Clock switching is enabled, 
                                       // Fail-Safe Clock Monitor is enabled)
#pragma config FNOSC = FRCPLL      // Oscillator Select (Fast RC Oscillator with PLL module (FRCPLL))

/*This function sets up the PIC24 pins and its peripherals*/
void setup(void) {
    
    CLKDIVbits.RCDIV = 0; // set pic24 frequency to 16MHz
    AD1PCFG = 0xffff; // set all the pins to digital mode
    
    LATB = 0x0000;
    TRISB = 0x0000; // initially set all 
    
    TRISBbits.TRISB8 = 1; // set RB8 to input
    TRISBbits.TRISB9 = 1; // set RB9 to input
    TRISBbits.TRISB15 = 1; // set RB15 to input
    TRISBbits.TRISB14 = 1; // set RB14 to input
    
    TRISA &= 0x0000; // set all RA pins to output, since they're not used
    
    /*Set up I2C module*/
    I2C1CONbits.I2CEN = 0; // Disable I2C module
    I2C1BRG = 0x9D; // According to the table 16-1, since Fcy = 16 MHz
    _MI2C1IF = 0; // clear the I2C interrupt flag
    I2C1CONbits.I2CEN = 1; // Enable I2C module
     
}

void wait_1ms(void);

void delay(unsigned long int number) {
    unsigned long int i = 0;
    
    while (i < number) {
        wait_1ms();
        i++;
    }
}

/*This function uses timers' 4 and 5 to form a 
 * 32-bit timer that resets every minute.It also 
 * enables up the interrupt for the combined 32-bit timer. 
 */
void minuteSetup(void) {
    /*Set up 60 second timer*/
    T4CON = 0x0000; // set the timer 4/5 control register to 0
    T4CONbits.T32 = 1; // Use Timer 4/5 as a 32-bit timer
    T4CONbits.TCS = 0;// use internal clock for the timer
    T4CONbits.TCKPS = 0x00; // set pre-scaler to 1
    
    /*Together these numbers form 959,999,999 cycles = 60 seconds*/
    PR5 = 0x3938; // most significant half of 32 bit timer
    PR4 = 0x6FFF; // least significant half of 32 bit timer
    
    IFS1bits.T5IF = 0; // reset timer 4/5 interrupt flag
    IEC1bits.T5IE = 1; // enable timer 4/5 interrupt
    IPC7bits.T5IP = 7; // set interrupt to  highest priority
    
    TMR4 = 0; // reset first half of timer 4/5 value
    TMR5 = 0; // reset second half of Timer 4/5 value
    
    T4CONbits.TON = 1; // turn on timer 4/5      
}

/*This function sets up pins RB14 and RB15 on the microcontroller
   connecting it as inputs to input capture and sets up the timer 
    that the IC's use to measure the time of each edge during a 
 button press.*/
void buttonSetup() {
    
    T2CON = 0x0; // reset timer 1
    T2CONbits.TCS = 0; // user internal clk
    T2CONbits.TCKPS = 0x3; // prescaler set to 256:1
    PR2 = 65535; // Timer counts for as long as possible
    TMR2 = 0; // reset timer value
    
    IFS0bits.T2IF = 0; // reset timer 2 interrupt flag
    IEC0bits.T2IE = 1; // enable timer 2 interrupt flag
    IPC1bits.T2IP = 6; // set interrupt to second highest priority
    
    T2CONbits.TON = 1; // turn on timer 2
    
     
    __builtin_write_OSCCONL(OSCCON & 0xbf); // unlock PPS
    RPINR7bits.IC1R = 0xF; // set Input Capture 1 input to RB 15
    RPINR7bits.IC2R = 0xE; // set input capture 2 input to RB 14
    __builtin_write_OSCCONL(OSCCON | 0x40); // lock   PPS
    
    CNPU1bits.CN11PUE = 1; // activate pull up resistor for RB15 (button)
    CNPU1bits.CN12PUE = 1; // activate pull up resistor for RB14 (button)
    
    IC1CONbits.ICM = 0x1; // Input capture 1 catches every edge
    IC1CONbits.ICI = 0x0; // Input Capture 1 catches every capture event
    IC1CONbits.ICTMR = 0x1; // Input Capture 1 source is Timer 2
    
    _IC1IF = 0; // Clear Input Capture 1 interrupt flag
    _IC1IE = 1; // Enable IC1 Interrupt
    _IC1IP = 1; // 
    
    IC2CONbits.ICM = 0x1; // Input Capture 2 catches every edge
    IC2CONbits.ICI = 0x0; // Input Capture 2 catches every event
    IC2CONbits.ICTMR = 0x1; // Input Capture 2 source is Timer 2
    
    _IC2IF = 0; // Clear IC2 interrupt flag
    _IC2IE = 1; // Enable IC2 Interrupt
    _IC2IP = 2; // 
            
}

/*This function takes in a command as a parameter and 
 sends that command to the LCD using I2C protocol.*/
void lcdCommand(char command){
    delay(2);
    
    I2C1CONbits.SEN = 1; // Start conditon of sequence
    while (I2C1CONbits.SEN); // When the start condition is over move on and send frames
    _MI2C1IF = 0; // clear interrupt flag
    
    //LATBbits.LATB5 = 1; // turn on heartbeat LED
    
    /*Send Address Frame*/
    I2C1TRN = 0x78; // address of LCD when SA0 pin is grounded
    while (!_MI2C1IF || I2C1STATbits.TRSTAT); // wait for acknowledge bit, bus to be empty, and interrupt flag
    delay(5); // delay program by 5 milliseconds
    if (I2C1STATbits.ACKSTAT) { // NACK sent
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        _MI2C1IF = 0;
        return;
    }
    _MI2C1IF = 0; // clear interrupt flag
    
    /*Send Control Frame*/
    I2C1TRN = 0x00; // send control frame
    while (!_MI2C1IF || I2C1STATbits.TRSTAT); // wait for acknowledge bit, bus to be empty, and interrupt flag
    delay(5);
    if (I2C1STATbits.ACKSTAT) { // NACK sent
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        _MI2C1IF = 0;
        return;
    }
    _MI2C1IF = 0; // clear interrupt flag
    
    /*Send actual command*/
    I2C1TRN = command; // transmit command
    while (!_MI2C1IF || I2C1STATbits.TRSTAT); // wait for acknowledge bit, bus to be empty, and interrupt flag
    if (I2C1STATbits.ACKSTAT) { // NACK sent
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        _MI2C1IF = 0;
        return;
    }
    _MI2C1IF = 0; // clear interrupt flag
    
    I2C1CONbits.PEN = 1; // start stop condition
    while (I2C1CONbits.PEN); // wait until stop condition is over
    
    //LATBbits.LATB5 = 0; // turn off heartbeat LED
}

/*This function resets the LCD using the RB6 pin on the PIC24
 * and sends a series of commands to the LCD using the 
 lcdCommand function that turns on the LCD and handles the
 abstractions.*/
void lcdInitialize() {
    delay(5);
    LATBbits.LATB6 = 0;
    delay(5);
    LATBbits.LATB6 = 1; // reset the LCD using Reset pin
    
    lcdCommand(0x3A); // function set
    lcdCommand(0x09); // extended function set
    lcdCommand(0x06); // entry mode set
    lcdCommand(0x1E); // bias setting
    lcdCommand(0x39); // function set
    lcdCommand(0x1B); // internal osc
    lcdCommand(0x6E); // follower control
    lcdCommand(0x56); // power control
    lcdCommand(0x7A); // contrast set
    lcdCommand(0x38); // function set
   
    
    
    lcdCommand(0x0F); // display on
    
    lcdCommand(0x3A); // function set, N = 1
   lcdCommand(0x17); // 3 lines and the middle has double the height
    lcdCommand(0x3c); // RE = 0 and DH = 1
   
    //lcdCommand(0x1a); //    
}

/* This function writes data to the LCD by generating the start condition of the bit transfer, then
 * sending the address of LCD, a control frame indicating a data write, and the a frame that holds the 
 * actual data.
 *  */
void writeLCD(char data) {
    
    I2C1CONbits.SEN = 1; // Start conditon of sequence
    delay(5);
    while (I2C1CONbits.SEN); // When the start condition is over move on and send frames
    _MI2C1IF = 0; // clear interrupt flag
    
    LATBbits.LATB5 = 1; // turn on heartbeat LED
    
    /*Send Address Frame*/
    I2C1TRN = 0x78; // address of LCD when SA0 pin is grounded
    while (!_MI2C1IF  || I2C1STATbits.TRSTAT); // wait for acknowledge bit, bus to be empty, and interrupt flag
    delay(5); // delay program by 500 milliseconds
    if (I2C1STATbits.ACKSTAT) { // NACK sent
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        _MI2C1IF = 0;
        return;
    }
    _MI2C1IF = 0; // clear interrupt flag
    
    /*Send Control Frame*/
    I2C1TRN = 0x40; // send control frame that indicates a read
    while (!_MI2C1IF || I2C1STATbits.TRSTAT); // wait for acknowledge bit, bus to be empty, and interrupt flag
    delay(5);
    if (I2C1STATbits.ACKSTAT) { // NACK sent
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        _MI2C1IF = 0;
        return;
    }
    _MI2C1IF = 0; // clear interrupt flag
    
    /*Send actual data*/
    I2C1TRN = data; // transmit data
    while (!_MI2C1IF || I2C1STATbits.TRSTAT); // wait for acknowledge bit, bus to be empty, and interrupt flag
    delay(5);
    if (I2C1STATbits.ACKSTAT) { // NACK sent
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        _MI2C1IF = 0;
        return;
    }
    _MI2C1IF = 0; // clear interrupt flag
    
    I2C1CONbits.PEN = 1; // start stop condition
    while (I2C1CONbits.PEN); // wait until stop condition is over
    
    LATBbits.LATB5 = 0; // turn off heartbeat LED
    delay(5);
}

/*Clear the display and the contents in the DRAM*/
void clearDisplay(){
    lcdCommand(0x01); // command that clears display
}

volatile unsigned long int minuteCount = 0; // integer holds current time as an integer


/* These global integers each hold a digit of the clock. */
unsigned int hourLSB = 0;
unsigned int hourMSB = 0;
unsigned int minuteLSB = 0;
unsigned int minuteMSB = 0; 
/*This character array holds the AM or PM indicating 
 if the time is before or after noon.*/
char midday[5] = {' ', 'A', '.', 'M', '.'};

/* Calculate the current time and send consecutive data writes
 to write that time to the LCD.*/
void calculateTime(){
    /* Initialize variables used to measure each digit of time*/
    unsigned int hour = 0;
    unsigned int minute = 0;
    
    if (minuteCount >= 1440) { // check if clock has gone past 24 hours
        minuteCount %= 60; // minute count holds the current minute value of the time
        hour = 0; // resets hour to 12:00 AM
    }
    else if (minuteCount < 720) { // time is between 12:00 AM and 11:59 AM
    hour = minuteCount / 60; // calculate the number of hours 
    }
    else { // time is between 12:00 PM and 11:59 PM
        hour = (minuteCount - 720) / 60;
    }
    minute = minuteCount % 60; // calculate what minute of the day it currently is
    
    hourLSB = hour % 10; // calculate the value of the hour for first digit
    hourMSB = hour / 10; // calculate the value of the hour for second digit
    
    /*Check if the clock is in the first hour of the day (12:00 - 12:59 AM)
     since 00:12 is not */
    if (hourMSB == 0 && hourLSB == 0) {
        hourMSB = 1;
        hourLSB = 2;
    }
        
    minuteLSB = minute % 10;
    minuteMSB = minute / 10;
    
    if (minuteCount < 720){ // If we are less than halfway through the day
        midday[1] = 'A';       
    }
    else {
        midday[1] = 'P';       
    }
} 

/*This function sends a data write to the LCD
 and writes the time of the */
void setTime() {
    char digitOne = '0' + hourMSB;
    char digitTwo = '0' + hourLSB;
    
    char digitThree = '0' + minuteMSB;
    char digitFour = '0' + minuteLSB;
    
    lcdCommand(0xa0); //set cursor to the middle
    delay(1);
    
    writeLCD(digitOne);
    delay(1);
    writeLCD(digitTwo);
    delay(1);
    writeLCD(':');
    delay(1);
    writeLCD(digitThree);
    delay(1);
    writeLCD(digitFour);
    delay(1);
    
    for (int i = 0; i < 5; i++) {
        writeLCD(midday[i]);
        delay(1);
    }       
}

/*This interrupt is fired every 60 seconds and updates the time on the LCD with 
    the integer minuteCount. */
void __attribute__((__interrupt__,__auto_psv__)) _T5Interrupt(void) {
    //LATBbits.LATB5 ^= 1;
    _T5IF = 0; // clear timer 4/5 interrupt flag
    
    minuteCount++ ;
    if (minuteCount >= 1440) { // if 24 hours has passed reset clock
        minuteCount = 0; // restarts counter
    }
    
    calculateTime(); // calculate current time
    setTime(); // update current time        
}

volatile unsigned int timer2Overflow = 0; // keeps track of how many times timer 2 resets

/*Interrupt is generated when timer 2 resets.*/
void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(void) {
    timer2Overflow += 1; // increment by 1
    IFS0bits.T2IF = 0;
}

int prevStateIC1 = 0; // integer holds previous state of button (press or release)
unsigned long int prevIC1Time = 0; // holds previous IC1 edge time

/*This interrupt fires when an edge is captured by IC and checks whether the button was pressed and updates the
 time by a minute if so.*/
void __attribute__((__interrupt__,__auto_psv__)) _IC1Interrupt(void) {
    
    unsigned long int currIC1Time = TMR2 + timer2Overflow*65535 ; // hold current time of edge
    
    if ((currIC1Time - prevIC1Time) > 1000) { // check if time between the two edges is > 2ms
        prevIC1Time = currIC1Time; // store the current time of the edge
        
        if (!prevStateIC1) { // if the previous state of the button was released, then this is a button press
            prevStateIC1 ^= 1; // toggle the state of the button
            minuteCount++; // update time
            calculateTime(); // 
            setTime();
        }
        else { // previous state of the button was press
            prevStateIC1 ^= 1; // toggle the state of the button
        }
    }
    
    IFS0bits.IC1IF = 0;
}

int prevStateIC2 = 0;
unsigned long int prevIC2Time;
void __attribute__((__interrupt__,__auto_psv__)) _IC2Interrupt(void) {
    unsigned long int currIC2Time = TMR2 + timer2Overflow*65535 ; // hold current time of edge
    
    if ((currIC2Time - prevIC2Time) > 1000) { // check if time between the two edges is > 2ms
        prevIC2Time = currIC2Time; // store the current time of the edge
        
        if (!prevStateIC2) { // if the previous state of the button was released, then this is a button press
            prevStateIC2 ^= 1; // toggle the state of the button
            minuteCount += 60 ; // update time by 1 hour
            calculateTime(); // 
            setTime();
        }
        else { // previous state of the button was press
            prevStateIC2 ^= 1; // toggle the state of the button
        }
    }
    
    IFS0bits.IC2IF = 0;
}

int main(void) {
    setup();
    minuteSetup();
    delay(5);
    buttonSetup();
    
    lcdInitialize();
    clearDisplay();
    
    calculateTime();
    setTime();
    
    while (1){
    }
     
    return 0;
}
