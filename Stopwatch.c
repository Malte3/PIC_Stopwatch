/*
 * File:   main.c
 * Author: Malte Rethwisch
 *
 * Created on 10. June 2018, 18:24
 */

// PIC18F1320 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1H
#pragma config OSC = INTIO2     // Oscillator Selection bits (Internal RC oscillator, port function on RA6 and port function on RA7)
#pragma config FSCM = OFF       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOR = OFF         // Brown-out Reset Enable bit (Brown-out Reset enabled)
// BORV = No Setting

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled, RA5 input pin disabled)

// CONFIG4L
#pragma config STVR = ON        // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = ON         // Low-Voltage ICSP Enable bit (Low-Voltage ICSP enabled)

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (00200-000FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (001000-001FFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot Block (000000-0001FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (00200-000FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (001000-001FFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0001FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (00200-000FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (001000-001FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0001FFh) not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define _XTAL_FREQ 8000000

void update_miliseconds(unsigned char vector[]);
void update_seconds(unsigned char vector[]);
void switch_display(unsigned char nr);
void switch_numbers(unsigned char nr, unsigned char dp);

// initialize state and display vector and states
typedef enum { zero, started, stopped,} States;
States state = zero;
typedef enum { off, on, debounce} Button_States;
Button_States button_s = off;
int block = 0;
unsigned char dislay_vector[6] = { 0, 0, 0, 0, 0, 0};
//Main Interrupt Service Routine (ISR)
void interrupt ISR(void)
{
    // Button press interrupt
    if(INTCON3bits.INT2IF)
    {
        // switch stopwatch state after button press
        if(state == zero)
        {
            // watch shows zero
            block = 1; //block button press
            button_s = debounce;
            state = started;
            // reset timer and set interrupt counter
            T0CON = 0xF6;
            T1CON = 0x85;
            TMR1H = 248;
            TMR1L = 48;
            TMR0H = 0x00;
            TMR0L = 0x00;
            PIE1 = 0x01;
            INTCON3 = 0x00; //block button press
            INTCON = 0xE0;
        }
        else if(state == started)
        {
            // stopwatch started and shows time
            block = 1; //block button press
            button_s = debounce;
            state = stopped;
            // reset timer and interrupt
            T0CON = 0x00;
            T1CON = 0x00;
            PIE1 = 0x00;
            INTCON3 = 0x00; //block button press
            INTCON = 0xC0;
        }
        else if(state == stopped)
        {
            // set output to all zeros
            for(unsigned int j = 0; j < 6; j++)
            {
                dislay_vector[j] = 0;
            }
            block = 1; //block button press
            button_s = debounce;
            state = zero;
            // reset timer and interrupt
            T0CON = 0x00;
            T1CON = 0x00;
            PIE1 = 0x00;
            INTCON3 = 0x00; //block button press
            INTCON = 0xC0;
        }
        // reset interrupt flag
        INTCON3bits.INT2IF = 0;        
    }
    // Counter Interrupt
    if(PIR1bits.TMR1IF)
    {
        // updates miliseconds
        // counter preset
        TMR1H = 248;
        TMR1L = 48;
        // reset interrupt flag
        PIR1bits.TMR1IF = 0;
        // synchronize miliseconds and seconds
        if(dislay_vector[0] != 9 | dislay_vector[1] != 9 | dislay_vector[2] != 9)
        {
            update_miliseconds(dislay_vector);
        }
    }
    if(INTCONbits.TMR0IF)
    {
        // updates senconds and minutes
        INTCONbits.TMR0IF = 0;
        update_seconds(dislay_vector);
    }
    return;
}


void main(void) {
    // initialize all ports and interrupt registers
    INTCON = 0xC0; // global interrupt enable
    INTCON2 = 0x04; // falling edge interrupt enable
    INTCON3 = 0x90; // INT2 on RB2 enable
    TRISA = 0x10; //RB0 as Output PIN
    TRISB = 0x04; //RB0 as Output PIN
    LATB = 0x04;
    LATA = 0x00;
    OSCCON = 0x70; // inernal osc at 8 mhz
    RCON = 0x00; // interrupt priority disabledabled
    ADCON1 = 0x7F;
    T1CON = 0x00;
    T0CON = 0x00;
    TMR1H = 248;
    TMR1L = 48;
    TMR0H = 0x00;
    TMR0L = 0x00;
    PIE1 = 0x00;
    // reset interrupt flag
    PIR1bits.TMR1IF = 0;
    
    // set the senconds and mili seconds seperators
    unsigned char DP[6] = { 0, 0, 0, 1, 0, 1};
    // numbers from 0 to 9
    unsigned char numbers[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    
    // initilaize counter for button debouncing
    unsigned int count_total = 0;
    unsigned int count_low = 0;
    
    // main loop
    while(1)
    {   
        // loop through seven segment displays
        for(unsigned int j = 0; j < 6; j++)
        {
            // switch to the current display
            switch_display(j);
            // put out the current number
            switch_numbers(numbers[dislay_vector[j]], DP[j]);
            // debounce button by not allowing more button presses for 500 iterations
            if(button_s == debounce)
            {
                if (PORTBbits.RB2 != 1)
                {
                    count_low++;
                }	
                count_total++;
                // just release after 500
                if(count_total == 500)
                {
                    block = 0;
                    INTCON3 = 0x90; // INT2 on RB2 enable
                }
                // if button released
                if( count_total - count_low > 500)
                {
                    button_s = off;
                    count_low = 0;
                    count_total = 0;
                    block = 0;
                    INTCON3 = 0x90; // INT2 on RB2 enable
                }
            }
            //clear the leds for a short time to avoid flickering and ghosting
            //show number for:
            __delay_us(350);
            switch_display(j);
            switch_numbers(10, 0);
            //clear number for:
            __delay_us(350);
        }      
    }
  return;
}

// rutine to update milliseconds by updating the display vector of the last three digits
void update_miliseconds(unsigned char vector[])
{
    // add 1 to the milliseconds place
    vector[0]++;
    
    // check for overflow to update the following digits
    for(int i = 0; i < 3; i++)
    {
        if( i < 2  )
        {
            // when one of the milliseconds places reches 10 set them to 0 and update the following
            if( vector[i] == 10)
            {
                vector[i+1]++;
                vector[i] = 0;
            }
        }
        else
        {
            // when 1000 milliseconds are reached
            if( vector[i] == 10)
            {
                vector[0] = 0;
                vector[1] = 0;
                vector[2] = 0;
            }
        }
    }
    return;
}

// update second and minute
void update_seconds(unsigned char vector[])
{
    // add 1 to the seconds place
    vector[3]++;
    // set the milliseconds to zero so they are synchronized with the seconds counter
    vector[0] = 0;
    vector[1] = 0;
    vector[2] = 0;
    // check for overflows
    for(int i = 3; i < 5; i++)
    {
        if( i < 4  )
        {
            // seconds
            if( vector[i] == 10)
            {
                vector[i+1]++;
                vector[i] = 0;
            }
        }
        else
        {
            // tens of seconds
            if( vector[i] == 6)
            {
                vector[i+1]++;
                vector[i] = 0;
            }
        }
    }
    return;
}

// switch display numbers to the correct ports
void switch_display(unsigned char nr)
{
    switch(nr)
    {
        case 0:
            LATBbits.LATB0 = 1;
            LATBbits.LATB1 = 0;
            LATAbits.LATA6 = 0; 
            LATBbits.LATB3 = 0; 
            LATBbits.LATB4 = 0; 
            LATBbits.LATB6 = 0;
            break;
        case 1:
            LATBbits.LATB0 = 0;
            LATBbits.LATB1 = 1;
            LATAbits.LATA6 = 0; 
            LATBbits.LATB3 = 0; 
            LATBbits.LATB4 = 0; 
            LATBbits.LATB6 = 0;
            break;
        case 2:
            LATBbits.LATB0 = 0;
            LATBbits.LATB1 = 0;
            LATAbits.LATA6 = 1; 
            LATBbits.LATB3 = 0; 
            LATBbits.LATB4 = 0; 
            LATBbits.LATB6 = 0;
            break;
        case 3:
            LATBbits.LATB0 = 0;
            LATBbits.LATB1 = 0;
            LATAbits.LATA6 = 0; 
            LATBbits.LATB3 = 1; 
            LATBbits.LATB4 = 0; 
            LATBbits.LATB6 = 0;
            break;
        case 4:
            LATBbits.LATB0 = 0;
            LATBbits.LATB1 = 0;
            LATAbits.LATA6 = 0; 
            LATBbits.LATB3 = 0; 
            LATBbits.LATB4 = 1; 
            LATBbits.LATB6 = 0;
            break;
        case 5:
            LATBbits.LATB0 = 0;
            LATBbits.LATB1 = 0;
            LATAbits.LATA6 = 0; 
            LATBbits.LATB3 = 0; 
            LATBbits.LATB4 = 0; 
            LATBbits.LATB6 = 1;
            break;
        default:
            LATBbits.LATB0 = 0;
            LATBbits.LATB1 = 0;
            LATAbits.LATA6 = 0; 
            LATBbits.LATB3 = 0; 
            LATBbits.LATB4 = 0; 
            LATBbits.LATB6 = 0;
            break;
                  
    }
    return;
}

// switch digits to the correct ports
void switch_numbers(unsigned char nr, unsigned char dp)
{
    LATAbits.LATA0 = 0x01 & nr; 
    LATAbits.LATA1 = (0x02 & nr) >> 1; 
    LATAbits.LATA2 = (0x04 & nr) >> 2; 
    LATAbits.LATA3 = (0x08 & nr) >> 3;
    LATBbits.LATB7 = dp;
    return;
}