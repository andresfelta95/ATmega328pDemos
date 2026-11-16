// A/D library, ATmega328P Version
// Simon Walker, NAIT

#include <avr/io.h>
#include "atd.h"

void AtoD_Init (AtoD_Channel chan)
{
  PRR &= ~(1 << PRADC); // turn on A/D module in power reduction register
  
  ADMUX = 0b00000000;   // right-aligned, ADC0 (28.9.1), reference set by AtoD_SetChannel
  ADCSRB = 0b00000000;  // default, no comparator, free running mode    
  
  // use support function to set channel (and voltage reference)
  AtoD_SetChannel(chan);
  
  ADCSRA = 0b11100111;  // ADEN, start conversion, ADATE (auto trigger = free running), prescale 128 (28.9.2)
}

void AtoD_SetChannel (AtoD_Channel chan)
{
  // kill the digital input buffer on the selected pin (saves power, 28.9.5)
  // only ADC0-ADC5 have digital inputs; DIDR0 bits 6 and 7 are reserved
  if (chan <= AtoD_Channel_5)
    DIDR0 |= (1 << chan);

  // voltage reference (28.9.1):
  //  - the internal temperature sensor must use the internal 1.1 V reference
  //  - everything else uses AVcc (REFS0), with a capacitor on the AREF pin.
  //    REFS = 00 (external AREF) only works if AREF is driven, which it
  //    isn't on an Arduino Uno/Nano.
  if (chan == AtoD_TempSensor)
    ADMUX = (ADMUX & 0b00111111) | (1 << REFS1) | (1 << REFS0);
  else
    ADMUX = (ADMUX & 0b00111111) | (1 << REFS0);

  // channel selection
  ADMUX &= 0b11110000;  // clear channel selection
  ADMUX |= chan;        // set back channel selection bits
}
