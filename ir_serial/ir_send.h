// Value for baudrate.
// Using this calculation:
//    UBRR = ( system_clock / (16 * baudrate)) - 1
// Google can be used for the calculation, like this: http://www.google.com/search?q=%2816000000%2F%2816+*+9600%29%29+-1
//
// Use the BAUDRATE and F_CPU (or include a file with F_CPU in uart.c)
// Or set the UBBR_VALUE directly to a value.
#define F_CPU 16000000UL

#define SEND_NEC 1
#define SEND_PANASONIC 1
#define SEND_RC5 1
#define SEND_RC6 1
#define SEND_SAMSUNG 1

#if SEND_NEC == 1
	void  sendNEC (unsigned long data,  int nbits);
#endif
#if SEND_PANASONIC == 1
	void  sendPanasonic (unsigned int address,  unsigned long data);
#endif
#if SEND_RC5 == 1
	void  sendRC5(unsigned long data,  int nbits);
#endif
#if SEND_RC6 == 1
	void  sendRC6 (unsigned long data,  int nbits);
#
#if SEND_SAMSUNG == 1
	void  sendSAMSUNG (unsigned long data,  int nbits);
#endif

void sendRaw(unsigned int buf[], int len, int hz);
void mark(int time);
void space(int time);
void enableIROut(int khz);