//  NeoPixel library, ATmega328P Version
//  Andres Tangarife, NAIT
//  Revision History:
//  March 29 2023 - Initial Build
//  Description:    This is the header for the library
//                  used to control a NeoPixel (WS2812B) LED strip
//                  with an ATmega328P using C.
//  Notes:          The DIN pin is connected to pin 5 of port D (PD5).
//                  The strip has NEOPIXEL_NUM_LEDS LEDs (12 by default).
//                  The bit timing is cycle-counted for F_CPU = 16 MHz.

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

#define NEOPIXEL_PORT PORTD
#define NEOPIXEL_DDR DDRD
#define NEOPIXEL_PIN PD5

#define NEOPIXEL_NUM_LEDS 12

// make the data pin an output (low)
void neopixel_init();

// set the colour of one LED / all LEDs in the frame buffer (no output yet)
void neopixel_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void neopixel_set_color_all(uint8_t r, uint8_t g, uint8_t b);

// set one LED / all LEDs to black in the frame buffer (no output yet)
void neopixel_turn_off(uint8_t led);
void neopixel_turn_off_all();

// hold the data line low long enough (> 50 us) for the LEDs to latch the frame
void neopixel_send_pulse();

// send the frame buffer to the strip (interrupts are blocked while sending)
void neopixel_update();

#endif /* NEOPIXEL_H */
