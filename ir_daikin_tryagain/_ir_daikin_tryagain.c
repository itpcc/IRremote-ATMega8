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

#ifndef IRremoteDaikinSend_h
#define IRremoteDaikinSend_h
	void sendRaw(unsigned int buf[], int len, int hz);
	void sendDaikin(unsigned char buf[], int len,int start);
	void sendDaikinWake();
	// private:
	void enableIROut(int khz);
	void setPin();
	void mark(int usec);
	void space(int usec);
#endif
#ifndef IRremoteintDaikin_h
#define IRremoteintDaikin_h
#ifdef F_CPU
#define SYSCLOCK F_CPU     // main Arduino clock
#else
#define SYSCLOCK 16000000  // main Arduino clock
#endif

//DAIKIN
#define DAIKIN_HDR_MARK	    3600 //DAIKIN_ZERO_MARK*8
#define DAIKIN_HDR_SPACE	1600 //DAIKIN_ZERO_MARK*4
#define DAIKIN_ONE_SPACE	1300
#define DAIKIN_ONE_MARK	    380
#define DAIKIN_ZERO_MARK	380
#define DAIKIN_ZERO_SPACE   380
#endif

void sendDaikin(unsigned char buf[], int len, int start) {
	int data2, i, j;
	enableIROut(38);
	mark(DAIKIN_HDR_MARK);
	space(DAIKIN_HDR_SPACE);

	for (i = start; i < start+len; i++) {
		data2=buf[i];

		for (j = 0; j < 8; j++) {
			if ((1 << j & data2)) {
				mark(DAIKIN_ONE_MARK);
				space(DAIKIN_ONE_SPACE);
			}else {
				mark(DAIKIN_ZERO_MARK);
				space(DAIKIN_ZERO_SPACE);
			}
		}
	}
	mark(DAIKIN_ONE_MARK);
	space(DAIKIN_ZERO_SPACE);
}

void sendDaikinWake() {
	enableIROut(38);
	space(DAIKIN_ZERO_MARK);
	//
	mark(DAIKIN_ZERO_MARK);
	space(DAIKIN_ZERO_MARK);
	//
	mark(DAIKIN_ZERO_MARK);
	space(DAIKIN_ZERO_MARK);
	//
	mark(DAIKIN_ZERO_MARK);
	space(DAIKIN_ZERO_MARK);
	//
	mark(DAIKIN_ZERO_MARK);
	space(DAIKIN_ZERO_MARK);
	//
	mark(DAIKIN_ZERO_MARK);
	space(DAIKIN_ZERO_MARK);
}



void sendRaw(unsigned int buf[], int len, int hz){
	int i;
	enableIROut(hz);

	for (i = 0; i < len; i++) {
		if (i & 1) {
			space(buf[i]);
		}
		else {
			mark(buf[i]);
		}
	}
	space(0); // Just to be sure
}

void mark(int time) {
	// Sends an IR mark for the specified number of microseconds.
	// The mark output is modulated at the PWM frequency.
	TCCR1A |= _BV(COM1A1); // Enable pin 3 PWM output
	delayMicroseconds(time);
}

/* Leave pin off for time (given in microseconds) */
void space(int time) {
	// Sends an IR space for the specified number of microseconds.
	// A space is no output, so the PWM output is disabled.
	TCCR1A &= ~(_BV(COM1A1)); // Disable pin 3 PWM output
	delayMicroseconds(time);
}

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

static unsigned char daikin2[27] = {
  0x11, 0xDA, 0x27, 0xF0, 0x00, 0x00, 0x00, 0x2, 
  0x11, 0xDA, 0x27, 0x00, 0x00, 0x31, 0x32, 0x00, 
  0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC1, 0x80, 0x00, 0x26}; 

int main(){
	DDRB |= _BV(1); //PB1
	PORTB &= ~_BV(1);
	while(1){
		sendDaikin(daikin2, 8,0);
		delay(29);
		sendDaikin(daikin2, 19,8);
		PORTB &= ~_BV(1);
		delay(2000);
	}
}
