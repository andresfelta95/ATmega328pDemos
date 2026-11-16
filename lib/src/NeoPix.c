//  NeoPixel library, ATmega328P Version
//  Andres Tangarife, NAIT
//  Revision History:
//  March 29 2023 - Initial Build
//  Description:    This library is used to control a NeoPixel (WS2812B) LED strip
//                  with an ATmega328P using C.
//  Notes:          The DIN pin is connected to pin 5 of port D (PD5).
//                  The strip has NEOPIXEL_NUM_LEDS LEDs (12 by default).

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "NeoPix.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

// The bit loop in neopixel_update() is counted in CPU cycles
#if F_CPU != 16000000UL
#error "NeoPix.c: the WS2812 bit timing is written for F_CPU = 16 MHz"
#endif

// Data buffer for transmitting color data to the LED strip
// 3 bytes per LED, in the G, R, B order the WS2812 expects
static uint8_t neopixel_buffer[NEOPIXEL_NUM_LEDS * 3];

// Hold the line low so the LED strip latches the current color data
// (WS2812B needs > 50 us low, newer parts up to 280 us; 300 us covers both)
void neopixel_send_pulse()
{
  NEOPIXEL_PORT &= ~(1 << NEOPIXEL_PIN);
  _delay_us(300);
}

// Initialize the neopixel strip
void neopixel_init() {
  // Set the neopixel pin as an output, starting low
  NEOPIXEL_PORT &= ~(1 << NEOPIXEL_PIN);
  NEOPIXEL_DDR |= (1 << NEOPIXEL_PIN);
}

// Set the color of a specific LED in the strip
void neopixel_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
  // ignore LEDs that aren't on the strip
  if (led >= NEOPIXEL_NUM_LEDS)
    return;

  // Calculate the start index of the color data for this LED
  uint16_t index = led * 3;

  // Store the color data for this LED in the data buffer
  neopixel_buffer[index + 0] = g;
  neopixel_buffer[index + 1] = r;
  neopixel_buffer[index + 2] = b;
}

// Set the color of all LEDs in the strip
void neopixel_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
  // Store the color data for all LEDs in the data buffer
  for (uint8_t i = 0; i < NEOPIXEL_NUM_LEDS; i++) {
    neopixel_set_color(i, r, g, b);
  }
}

// Turn off a specific LED in the strip
void neopixel_turn_off(uint8_t led) {
  // Set the color data for this LED to 0
  neopixel_set_color(led, 0, 0, 0);
}

// Turn off all LEDs in the strip
void neopixel_turn_off_all() {
  // Set the color data for all LEDs to 0
  neopixel_set_color_all(0, 0, 0);
}

// Send the color data in the buffer to the LED strip
//
// WS2812B timing at 16 MHz (1 cycle = 62.5 ns), MSB first:
//   '0' bit : high  6 cycles (375 ns), low 14 cycles
//   '1' bit : high 13 cycles (812 ns), low  7 cycles
//   every bit is 20 cycles (1.25 us = 800 kHz); the last bit of each
//   byte has 6 extra low cycles while the next byte is loaded.
// The loop is written in assembly so the compiler can't change the timing,
// and interrupts are blocked so an ISR can't stretch a pulse.
void neopixel_update() {
  const uint8_t *ptr = neopixel_buffer;
  uint16_t count = sizeof(neopixel_buffer);
  uint8_t byte, bits;

  uint8_t sreg = SREG;
  cli();

  // port values with the data pin high / low (port read with interrupts off)
  uint8_t hi = NEOPIXEL_PORT | (1 << NEOPIXEL_PIN);
  uint8_t lo = NEOPIXEL_PORT & ~(1 << NEOPIXEL_PIN);

  __asm__ volatile (
    "1:                       \n\t"
    "ld   %[byte], %a[ptr]+   \n\t" // 2  next byte
    "ldi  %[bits], 8          \n\t" // 1
    "2:                       \n\t"
    "out  %[port], %[hi]      \n\t" // 1  t=0  line high
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1  t=4
    "sbrs %[byte], 7          \n\t" // 1/2 skip next if bit is 1
    "out  %[port], %[lo]      \n\t" // 1  t=6  '0' bit: line low
    "lsl  %[byte]             \n\t" // 1  t=7  next bit into bit 7
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1  t=12
    "out  %[port], %[lo]      \n\t" // 1  t=13 '1' bit: line low
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1
    "nop                      \n\t" // 1  t=16
    "dec  %[bits]             \n\t" // 1  t=17
    "brne 2b                  \n\t" // 2  t=19 -> next 'out' ends at t=20
    "sbiw %[count], 1         \n\t" // 2
    "brne 1b                  \n\t" // 2
    : [byte] "=&r" (byte),
      [bits] "=&d" (bits),
      [ptr] "+e" (ptr),
      [count] "+w" (count)
    : [port] "I" (_SFR_IO_ADDR(NEOPIXEL_PORT)),
      [hi] "r" (hi),
      [lo] "r" (lo)
    : "memory"
  );

  SREG = sreg;   // restore the interrupt flag

  // latch the frame
  neopixel_send_pulse();
}
