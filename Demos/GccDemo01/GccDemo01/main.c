/*
 * GccDemo01.c
 *
 * Created: 3/10/2023 2:32:51 PM
 * Author : atangarifecamac1
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>


int main(void)
{
    // one-time initialization section

    // sample schematic shows a LED on pin PB0
    // set pin 0 of port B to be an output
    // note: this is just like with our micro!
    // but the derivative file is a little different...
    DDRB |= 0b00000001; // same as DDRB |= 0b00000001;
    // same as DDRD |= 0b10000000;

    // main program loop - don't exit
    while(1)
    {
        //_delay_ms(1);
        //PORTD |= 0b10000000; // PD7 on
        //_delay_ms(1);
        //PORTD &= ~(0b10000000); // PD7 off

        // same as
        _delay_ms(500);
        //  Toggle the LED
        PORTB ^= 0b00000001; // PB0 toggle
    }
}

