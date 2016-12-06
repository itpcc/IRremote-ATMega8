#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define SEND_NEC 1
#define SEND_PANASONIC 0
#define SEND_RC5 0
#define SEND_RC6 0
#define SEND_SAMSUNG 0

/*void mark(int time) {
	// Sends an IR mark for the specified number of microseconds.
	// The mark output is modulated at the PWM frequency.
	TCCR1A |= _BV(COM1A1); // Enable pin 3 PWM output
	delayMicroseconds(time);
}

 Leave pin off for time (given in microseconds) 
void space(int time) {
	// Sends an IR space for the specified number of microseconds.
	// A space is no output, so the PWM output is disabled.
	TCCR1A &= ~(_BV(COM1A1)); // Disable pin 3 PWM output
	delayMicroseconds(time);
}

//~ #endif*/
#define mark(t) TCCR1A |= _BV(COM1A1); _delay_us(t)
#define space(t) TCCR1A  &= ~(_BV(COM1A1)); _delay_us(t)


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

#if SEND_NEC == 1
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
	}
#endif

//==============================================================================
//       PPPP    AAA   N   N   AAA    SSSS   OOO   N   N  IIIII   CCCC
//       P   P  A   A  NN  N  A   A  S      O   O  NN  N    I    C
//       PPPP   AAAAA  N N N  AAAAA   SSS   O   O  N N N    I    C
//       P      A   A  N  NN  A   A      S  O   O  N  NN    I    C
//       P      A   A  N   N  A   A  SSSS    OOO   N   N  IIIII   CCCC
//==============================================================================

//+=============================================================================
#if SEND_PANASONIC == 1
	#define PANASONIC_BITS          48
	#define PANASONIC_HDR_MARK    3502
	#define PANASONIC_HDR_SPACE   1750
	#define PANASONIC_BIT_MARK     502
	#define PANASONIC_ONE_SPACE   1244
	#define PANASONIC_ZERO_SPACE   400
	void  sendPanasonic (unsigned int address,  unsigned long data){
		unsigned long  mask;
		// Set IR carrier frequency
		enableIROut(35);

		// Header
		mark(PANASONIC_HDR_MARK);
		space(PANASONIC_HDR_SPACE);

		// Address
		for (mask = 1UL << (16 - 1);  mask;  mask >>= 1) {
			mark(PANASONIC_BIT_MARK);
			if (address & mask)  { space(PANASONIC_ONE_SPACE) ; }
			else                 { space(PANASONIC_ZERO_SPACE) ; }
		}

		// Data
		for (mask = 1UL << (32 - 1);  mask;  mask >>= 1) {
			mark(PANASONIC_BIT_MARK);
			if (data & mask)  { space(PANASONIC_ONE_SPACE) ; }
			else              { space(PANASONIC_ZERO_SPACE) ; }
		}

		// Footer
		mark(PANASONIC_BIT_MARK);
		space(0);  // Always end with the LED off
	}
#endif
//==============================================================================
//                          RRRR    CCCC  55555
//                          R   R  C      5
//                          RRRR   C      5555
//                          R  R   C          5
//                          R   R   CCCC  5555
//
// NB: First bit must be a one (start bit)

//+=============================================================================

#if SEND_RC5 == 1
	//
	#define MIN_RC5_SAMPLES     11
	#define RC5_T1             889
	#define RC5_RPT_LENGTH   46000
	void  sendRC5(unsigned long data,  int nbits){
		unsigned long  mask;
		// Set IR carrier frequency
		enableIROut(36);

		// Start
		mark(RC5_T1);
		space(RC5_T1);
		mark(RC5_T1);

		// Data
		for (mask = 1UL << (nbits - 1);  mask;  mask >>= 1) {
			if (data & mask) {
				space(RC5_T1); // 1 is space, then mark
				mark(RC5_T1);
			} else {
				mark(RC5_T1);
				space(RC5_T1);
			}
		}
		space(0);  // Always end with the LED off
	}
#endif

//+=============================================================================
//                           RRRR    CCCC   6666
//                           R   R  C      6
//                           RRRR   C      6666
//                           R  R   C      6   6
//                           R   R   CCCC   666
//
// NB : Caller needs to take care of flipping the toggle bit
//

#if SEND_RC6 == 1
	#define MIN_RC6_SAMPLES      1
	#define RC6_HDR_MARK      2666
	#define RC6_HDR_SPACE      889
	#define RC6_T1             444
	#define RC6_RPT_LENGTH   46000
	void  sendRC6 (unsigned long data,  int nbits){
		unsigned long  i, mask;
		int t;
		// Set IR carrier frequency
		enableIROut(36);

		// Header
		mark(RC6_HDR_MARK);
		space(RC6_HDR_SPACE);

		// Start bit
		mark(RC6_T1);
		space(RC6_T1);

		// Data
		for (i = 1, mask = 1UL << (nbits - 1);  mask;  i++, mask >>= 1) {
			// The fourth bit we send is a "double width trailer bit"
			t = (i == 4) ? (RC6_T1 * 2) : (RC6_T1) ;
			if (data & mask) {
				mark(t);
				space(t);
			}else{
				space(t);
				mark(t);
			}
		}
		space(0);  // Always end with the LED off
	}
#endif

/*void sendRaw(unsigned int buf[], int len, int hz){
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
}*/
