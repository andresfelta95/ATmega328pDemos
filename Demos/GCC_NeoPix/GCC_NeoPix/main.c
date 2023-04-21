/*
 * GCC_NeoPix.c
 *
 * Created: 3/29/2023 1:13:57 PM
 * Author : atangarifecamac1
 * 
 * Description: This is a test program for the NeoPixel library.
 */ 
#define F_CPU 16E6

#include <avr/io.h>
#include <util/delay.h> // have to add, has delay implementation (requires F_CPU to be defined)
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "NeoPix.h"
#include "timer.h"

//  Global Variables
//  Constant for timer output compare offset
// const unsigned int _Timer_OC_Offset = 200; //    1/(16E6 / 1 / 20) = 1.25 us

//  global counter for timer
// volatile unsigned int _Timer_Counter = 0;



int main(void)
{
    //  Initialize the timer
    // Timer_Init(Timer_Prescale_1, _Timer_OC_Offset); //  1.25 us interval
    neopixel_init();
    // DDRD |= 1 << PORTD5;    //  make port D pin 5 an output (PD5)
    // enable sleep mode, for idle, sort of similar to WAI on 9S12X (13.2)
    //SMCR = 0b00000001;
    // sleep_enable();

    // set the global interrupt flag (enable interrupts)
    // this is backwards from the 9S12
    // sei();

    while (1) 
    {
        // go idle!
        // note: this implies that the main loop can execute in less
        // than 1ms, or, is tolerant of being interrupted if it can't
        // sleep_cpu(); 
        // Set red the fifth LED in the strip
        // neopixel_set_color(0, 255, 0, 0);
        // neopixel_set_color(1, 0, 255, 0);
        // neopixel_set_color(2, 0, 0, 255);
        // neopixel_set_color(3, 255, 255, 0);
        // neopixel_set_color(4, 255, 0, 255);
        // neopixel_set_color(5, 0, 255, 255);
        // neopixel_set_color(6, 255, 255, 255);
        // neopixel_set_color(7, 5, 100, 255);
        neopixel_set_color_all(0, 0, 255);
        // Send the color data to the LED strip
        neopixel_update();
        // Wait 1 second    
        // neopixel_update();
        _delay_ms(1000);
        // neopixel_turn_off_all();
        neopixel_set_color_all(0, 255, 0);
        neopixel_update();
        _delay_ms(1000);
        neopixel_set_color_all(100, 50, 255);
        neopixel_update();
        _delay_ms(1000);
        
    }
}

//  Timer Interrupt Service Routine
// output compare A interrupt
// ISR(TIMER1_COMPA_vect)
// {
//     // static unsigned char cnter = 0;
//     // rearm the output compare operation
//     OCR1A += _Timer_OC_Offset; // 

//     // Set the port D pin 5 high
//     PORTD |= 1 << PORTD5;
//     _delay_us(1);
//     // ++cnter;
//     // ++_Timer_Counter;
//     //  set the port D pin 5 low
//     PORTD &= ~(1 << PORTD5);
//     // //  set the port D pin 6 high
//     // PORTD |= 1 << PORTD6;
//     // //  set the port D pin 6 low
//     // PORTD &= ~(1 << PORTD6);
// }



