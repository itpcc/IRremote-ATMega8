#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"

#define ESP_DEBUG 1

#define true  1
#define false 0

#define SERIAL_RESULT_BUFFER_SIZE 101

#define ESP_STATE_INIT 0
#define ESP_STATE_SETMODE 1
#define ESP_STATE_CONNECT 2
#define ESP_STATE_CONNECT_RESPONSE 3
#define ESP_STATE_CONNECT_MUX 4
#define ESP_STATE_CONNECT_CLOSE_EXIST_CONNECTION 5
#define ESP_SET_UDP_SERVER 6
#define ESP_STATE_SUCCESS 255

uint8_t ESP_CURRENT_STATE = 0;
char serialResult[SERIAL_RESULT_BUFFER_SIZE];


//http://efundies.com/avr-timer-interrupts-in-c/
volatile uint8_t interruptDelayComplete = 0;

void interruptDelayStop(){
	TCCR0 = 0;
	TCNT0 = 0;
	TIMSK &= ~_BV(TOIE0); //Disable Timer0 Overflow interrupt
}

void interruptDelayLoop(){
	TCCR0 = 0; // close timer temporary

	TCNT0 = 0x7C; // Count Every 8 ms
	TIMSK |= _BV(TOIE0); //Enable Timer0 Overflow interrupt
	sei(); //enable global overflow
	TCCR0 |= _BV(CS02) | _BV(CS00); //clkIO /1024 start
}

void interruptDelaySetup(uint8_t sec){
	if(sec < 4){
		interruptDelayComplete = (64*sec)-1;
		interruptDelayLoop();
	}
}

ISR(TIMER0_OVF_vect){
	if(interruptDelayComplete > 0){
		interruptDelayComplete--;
		interruptDelayLoop();
	}else{
		interruptDelayStop();
	}
}

#define interruptDelayUnFinish (interruptDelayComplete > 0)

uint8_t receiveSerial(){
	memset(serialResult, 0, SERIAL_RESULT_BUFFER_SIZE-1);
	#if ESP_DEBUG
		UART_puts("--> WAIT INCOMING <--\r\n");
	#endif

	interruptDelaySetup(2);
	while(UART_rbuflen() == 0 && interruptDelayUnFinish);
	if(   UART_rbuflen() > 0 ){
		UART_gets(serialResult, SERIAL_RESULT_BUFFER_SIZE-1);		
		if(strlen(serialResult)){ //sometime it return empty string :/
			#if ESP_DEBUG
				UART_puts("-->RECEIVE:"); UART_puts(serialResult);UART_puts("<--\r\n");
			#endif

			if(strstr(serialResult, "busy p..") != NULL){
				#if ESP_DEBUG
					UART_puts("--> BUSY <--\r\n");
				#endif
				_delay_ms(100);
				return receiveSerial();
			}

			return true;
		}else{
			#if ESP_DEBUG
				UART_puts("--> EMPTY STRING <--\r\n");
			#endif
			return receiveSerial(); // so, we've to try again
		}
	}

	return false;
}
uint8_t checkReturn(char *compareWord){
	
	if(receiveSerial()){
		#if ESP_DEBUG
			UART_puts("-->Compare:"); UART_puts(serialResult); UART_puts(" WITH:"); UART_puts(compareWord); UART_puts("<--\r\n");
		#endif

		if(strstr(serialResult, compareWord) != NULL){
			return true;
		}else{
			return false;
		}
	}else{
		#if ESP_DEBUG
			UART_puts("--> Return:"); UART_puts(serialResult); UART_puts("<--\r\n");
		#endif
		return false;
	}
}

uint8_t sendCommand(char *sentCommand, char *compareWord){
	UART_putc('\r'); UART_putc('\n'); UART_puts(sentCommand); UART_putc('\r'); UART_putc('\n');
	if(!checkReturn(sentCommand))
		return false;

	return checkReturn(compareWord);
}

uint8_t nmea_get_checksum(const char *sentence){
    const char *n = sentence + 1; // Plus one, skip '$'
    uint8_t chk = 0;

    /* While current char isn't '*' or sentence ending (newline) */
    while ('*' != *n && '\r' != *n && '\n' != *n) {
        if ('\0' == *n || n - sentence > SERIAL_RESULT_BUFFER_SIZE) {
            /* Sentence too long or short */
            return 0;
        }
        chk ^= (uint8_t) *n;
        n++;
    }

    return chk;
}


uint8_t detectNMEA(){
	char *currStrPos, *dataPtr;
	uint8_t checksum, checksumResult;

	#if ESP_DEBUG
		char numberResult[8];
	#endif

	if(!receiveSerial())	return false;
	if((currStrPos = strstr(serialResult, "+IPD,")) == NULL)	return false;
	currStrPos += 5;

	// No care String lenght
	if((currStrPos = strchr(currStrPos, '$')) == NULL)	return false;
	#if ESP_DEBUG
		UART_puts("-->NMEA Data:"); UART_puts(currStrPos); UART_puts("<--\r\n");
	#endif

	if((checksumResult = nmea_get_checksum(currStrPos)) == 0)	return false;
	dataPtr = currStrPos + 1;

	#if ESP_DEBUG
		UART_puts("-->NMEA Checksum calculate:"); UART_puts(itoa(checksumResult, numberResult, 16)); UART_puts("<--\r\n");
	#endif

	if((currStrPos = strchr(currStrPos, '*')) == NULL)	return false;
	checksum = strtol(currStrPos+1, NULL, 16);
	#if ESP_DEBUG
		UART_puts("-->NMEA Checksum return:"); UART_puts(itoa(checksum, numberResult, 16)); UART_puts("<--\r\n");
	#endif
	if(checksum != checksumResult) return false;

	return true;
}

int main(){
	uint8_t retryCount = 0;
	UART_init();
	DDRB = _BV(1);
	PORTB &= ~_BV(1);
	serialResult[SERIAL_RESULT_BUFFER_SIZE-1] = '\0';
	#if ESP_DEBUG
		UART_puts("-->>ATMega Start! <<--\r\n");
	#endif
	UART_puts("\r\nAT+RST\r\n");
	_delay_ms(2000);
	UART_flush();
	while(1){
		_delay_ms(100);
		switch(ESP_CURRENT_STATE){
			case ESP_STATE_INIT:
				if(sendCommand("AT", "OK"))
					ESP_CURRENT_STATE = ESP_STATE_SETMODE;
			break;
			case ESP_STATE_SETMODE:
				if(sendCommand("AT+CWMODE=3", "OK")){
					ESP_CURRENT_STATE = ESP_STATE_CONNECT;
					UART_flush();
				}
			break;
			case ESP_STATE_CONNECT:
				UART_flush();
				UART_puts("\r\nAT+CWJAP=\"aisfibre_2.4G_F57D50\",\"6FF57D50\"\r\n");
				interruptDelaySetup(2);
				while(interruptDelayUnFinish);
				ESP_CURRENT_STATE = ESP_STATE_CONNECT_RESPONSE;
				retryCount = 0;
			break;
			case ESP_STATE_CONNECT_RESPONSE:
				UART_flush();
				if(sendCommand("AT+CIPSTATUS", "STATUS:")){
					retryCount = strtol(strchr(serialResult, ':'), NULL, 10);
					#if ESP_DEBUG
						UART_puts("-->WIFI CIP Status:"); UART_putc('0'+retryCount); UART_puts("<--\r\n");
					#endif
					if(retryCount >= 2){
						ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
						retryCount = 0;
					}else{
						ESP_CURRENT_STATE = ESP_STATE_CONNECT;
					}
				}else if(strstr(serialResult, "CONNECT")){
					ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
					retryCount = 0;
				}else{
					++retryCount;
					#if ESP_DEBUG
						UART_puts("-->WIFI Connect Attemp:"); UART_putc('0'+retryCount); UART_puts("<--\r\n");
					#endif
					if(retryCount > 5)
						ESP_CURRENT_STATE = ESP_STATE_CONNECT;
				}
			break;
			case ESP_STATE_CONNECT_MUX:
				if(sendCommand("AT+CIPMUX=0", "OK") || strstr(serialResult, "ERROR"))
					ESP_CURRENT_STATE = ESP_STATE_CONNECT_CLOSE_EXIST_CONNECTION;
			break;
			case ESP_STATE_CONNECT_CLOSE_EXIST_CONNECTION:
				if(sendCommand("AT+CIPCLOSE", "OK") || strstr(serialResult, "ERROR") || strstr(serialResult, "FAIL")){
					ESP_CURRENT_STATE = ESP_SET_UDP_SERVER;
					retryCount = 0;
				}
			break;
			case ESP_SET_UDP_SERVER:				
				UART_puts("\r\nAT+CIPSTART=\"UDP\",\"0\",0,10000,2\r\n");
				interruptDelaySetup(2); while(interruptDelayUnFinish);
				if(checkReturn("CONNECT")){
					ESP_CURRENT_STATE = ESP_STATE_SUCCESS;
					UART_flush();
				}else if(strstr(serialResult, "ERROR")){
					ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
				}else{
					++retryCount;
					#if ESP_DEBUG
						UART_puts("-->WIFI Connect Attemp:"); UART_putc('0'+retryCount); UART_puts("<--\r\n");
					#endif
					if(retryCount > 5)
						ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
				}
			break;
			case ESP_STATE_SUCCESS:
				detectNMEA();
				UART_flush();
				#if ESP_DEBUG
					UART_puts("-->ESP Return:"); UART_puts(serialResult); UART_puts("<--\r\n");
				#endif
			break;
		}
		_delay_ms(100);
	}

	return 0;
}
