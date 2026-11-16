// Timer Library
// Simon Walker, NAIT

#include <avr/io.h>
#include "timer.h"

void Timer_Init (Timer_Prescale pre, unsigned int uiInitialOffset)
{
	// start code will power off all modules...
	// ensure power is on : Timer 1
	// PRR on 328P, PRR0 on 328PB
	PRR &= ~(1 << PRTIM1);

	// set prescale to requested rate
	TCCR1B = 0;		// noise canceler disabled, waveform generator normal
	TCCR1B |= pre;	// put back requested prescale bits
	
	// setup initial event for output compare 1 A
	OCR1A = TCNT1 + uiInitialOffset;

	// setup interrupt for output compare
	// timer/counter 1, output compare A match interrupt enable
	TIMSK1 = 0b00000010;
}

void Timer_F_PWM0 (Timer_PWM_Channel chan, Timer_PWM_ClockSel clksel, Timer_PWM_Pol pol)
{
  // setup fast PWM mode (mode 3, TOP = 0xFF) (14.7.3)
  // non-inverting is like +'ve polarity on 9S12 : COMx = 0b10
  // inverting is like -'ve polarity on 9S12     : COMx = 0b11

  // start code will power off all modules...
  // ensure power is on : Timer 0
  // PRR on 328P, PRR0 on 328PB
  PRR &= ~(1 << PRTIM0);

  unsigned char com = (pol == Timer_PWM_Pol_NonInverting) ? 0b10 : 0b11;

  if (chan == Timer_PWM_Channel_OC0A)
  {
    // keep channel B's settings, set channel A's compare output mode + fast PWM
    TCCR0A = (TCCR0A & 0b00110000) | (com << COM0A0) | (1 << WGM01) | (1 << WGM00);

    // start with 50% duty
    OCR0A = 0x7F;

    // pin must be marked as output (14.9.1) : OC0A is PD6
    DDRD |= (1 << PD6);
  }
  else
  {
    // keep channel A's settings, set channel B's compare output mode + fast PWM
    TCCR0A = (TCCR0A & 0b11000000) | (com << COM0B0) | (1 << WGM01) | (1 << WGM00);

    // start with 50% duty
    OCR0B = 0x7F;

    // pin must be marked as output (14.9.1) : OC0B is PD5
    DDRD |= (1 << PD5);
  }

  TCCR0B = clksel;      // set the desired clock (WGM02 = 0)

  // no interrupts are needed for PWM: enabling TOIE0 without an
  // ISR(TIMER0_OVF_vect) in the program would reset the MCU on sei()
}
