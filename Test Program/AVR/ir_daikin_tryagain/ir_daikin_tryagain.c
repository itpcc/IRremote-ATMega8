/*
 * Arduino IRremote Daikin 2015
 * Copyright 2015 danny
 *
 *
 * enableIROut declare base on  Ken Shirriff's IRremote library.
 * http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 *
 *
 */

#define F_CPU 16000000UL
#include <pololu/time.h>
#include <util/delay.h>
#include <avr/io.h>
#define delay(n) _delay_ms(n)

#ifdef F_CPU
#define SYSCLOCK F_CPU     // main Arduino clock
#else
#define SYSCLOCK 16000000  // main Arduino clock
#endif

// void mark(int time) {
// 	// Sends an IR mark for the specified number of microseconds.
// 	// The mark output is modulated at the PWM frequency.
// 	TCCR1A |= _BV(COM1A1); // Enable pin 3 PWM output
// 	delay_us(time);
// }

// //Leave pin off for time (given in microseconds)
// void space(int time) {
// 	// Sends an IR space for the specified number of microseconds.
// 	// A space is no output, so the PWM output is disabled.
// 	TCCR1A &= ~(_BV(COM1A1)); // Disable pin 3 PWM output
// 	delay_us(time);
// }

#define mark(t) TCCR1A |= _BV(COM1A1); _delay_us(t)
#define space(t) TCCR1A  &= ~(_BV(COM1A1)); _delay_us(t)

//~ #endif


void enableIROut(int khz) {
	static uint16_t pwmval;
	// Enables IR output.  The khz value controls the modulation frequency in kilohertz.
	// The IR output will be on pin 3 (OC2B).
	// This routine is designed for 36-40KHz; if you use it for other values, it's up to you
	// to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
	// TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
	// controlling the duty cycle.
	// There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
	// To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
	// A few hours staring at the ATmega documentation and this will all make sense.
	// See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.


	// Disable the Timer2 Interrupt (which is used for receiving IR)
	//TIMER_DISABLE_INTR; //Timer2 Overflow Interrupt

	/*pinMode(TIMER_PWM_PIN, OUTPUT);*/
	PORTB |= _BV(1);

	//digitalWrite(TIMER_PWM_PIN, HIGH); // When not sending PWM, we want it low
 //


	// COM2A = 00: disconnect OC2A
	// COM2B = 00: disconnect OC2B; to send signal set to 10: OC2B non-inverted
	// WGM2 = 101: phase-correct PWM with OCRA as top
	// CS2 = 000: no prescaling
	// The top value for the timer.  The modulation frequency will be SYSCLOCK / 2 / OCR2A.
	pwmval = 8000/khz;
	TCCR1A = _BV(WGM11);
	TCCR1B = _BV(WGM13)|_BV(CS10);
	ICR1 = pwmval;
	OCR1A = pwmval / 3;
	//TIMER_CONFIG_KHZ(khz);
}

#define NEC_BITS          32
	#define NEC_HDR_MARK    9000
	#define NEC_HDR_SPACE   4500
	#define NEC_BIT_MARK     560
	#define NEC_ONE_SPACE   1690
	#define NEC_ZERO_SPACE   560
	#define NEC_RPT_SPACE   2250

	//+=============================================================================
	void  sendNEC (unsigned long data,  int nbits){
		unsigned long  mask;
		// Set IR carrier frequency
		enableIROut(38);

		// Header
		mark(NEC_HDR_MARK);
		space(NEC_HDR_SPACE);

		// Data
		for (mask = 1UL << (nbits - 1);  mask;  mask >>= 1) {
			if (data & mask) {
				mark(NEC_BIT_MARK);
				space(NEC_ONE_SPACE);
			} else {
				mark(NEC_BIT_MARK);
				space(NEC_ZERO_SPACE);
			}
		}

		// Footer
		mark(NEC_BIT_MARK);
		space(0);  // Always end with the LED off
		PORTB &= ~_BV(1);
	}

int main(){
	DDRB |= _BV(1); //PB1
	delay(500);
	while(1){
		sendNEC(0xC1AA09F6, 32);
		delay(2000);
	}
}
