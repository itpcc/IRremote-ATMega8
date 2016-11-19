#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"

#define ESP_DEBUG 0

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
#define ESP_STATE_WAITIP 7
#define ESP_STATE_TCPCONNECT 8
#define ESP_STATE_SUCCESS 255

#define TCP_RETRY 30

uint8_t ESP_CURRENT_STATE = 0;
char serialResult[SERIAL_RESULT_BUFFER_SIZE];
char *commandName, *commandDetail;
char tcpIP[16];
unsigned long IRCommand; int nbits;

typedef unsigned long millis_t;
volatile millis_t _milliseconds, _targetSecond;


ISR(TIMER0_OVF_vect){
	++_milliseconds;
	TCNT0 = 0x06;
	TCCR0 = _BV(CS01) | _BV(CS00); // Prescaler: 64
}

void millis_Init(void){
	_milliseconds = 0;
	TCCR0 = _BV(CS01) | _BV(CS00); // Prescaler: 64
	TCNT0 = 0x06; // 256 - 6 = 250
	TIMSK |= _BV(TOIE0);
	sei(); //enable global interrupt
}

void interruptDelaySetup(int second){
	_targetSecond = _milliseconds + (second * 1000);
}

#define interruptDelayUnFinish() (_targetSecond > _milliseconds)

#if ESP_DEBUG
void Report_millis(){
	char currMillis[16];
	UART_puts("-->"); UART_puts(ultoa(_milliseconds, currMillis, 10)); UART_putc(';');
}
#endif

uint8_t receiveSerial(){
	memset(serialResult, 0, SERIAL_RESULT_BUFFER_SIZE-1);
	#if ESP_DEBUG
		Report_millis(); UART_puts(" WAIT INCOMING <--\r\n");
	#endif

	interruptDelaySetup(4);
	while(UART_rbuflen() == 0 && interruptDelayUnFinish());
	if(   UART_rbuflen() > 0 ){
		UART_gets(serialResult, SERIAL_RESULT_BUFFER_SIZE-1);
		if(strlen(serialResult)){ //sometime it return empty string :/
			#if ESP_DEBUG
				Report_millis(); UART_puts("RECEIVE:"); UART_puts(serialResult);UART_puts("<--\r\n");
			#endif

			if(strstr(serialResult, "busy p..") != NULL){
				#if ESP_DEBUG
					Report_millis(); UART_puts(" BUSY <--\r\n");
				#endif
				_delay_ms(500);
				return receiveSerial();
			}else if(strstr(serialResult, "WIFI DISCONNECT")){
				#if ESP_DEBUG
					Report_millis(); UART_puts(" WIFI State change:"); UART_puts(serialResult); UART_puts("<--\r\n");
				#endif
				return receiveSerial();
			}else if(strstr(serialResult, "WIFI GOT IP")){
				#if ESP_DEBUG
					Report_millis(); UART_puts(" WIFI State change:"); UART_puts(serialResult); UART_puts("<--\r\n");
				#endif
				if(ESP_CURRENT_STATE <= ESP_STATE_CONNECT_RESPONSE)
					ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
				return receiveSerial();
			}

			return true;
		}else{
			#if ESP_DEBUG
				Report_millis(); UART_puts(" EMPTY STRING <--\r\n");
			#endif
			_delay_ms(100);
			return receiveSerial(); // so, we've to try again
		}
	}

	#if ESP_DEBUG
		else if(!interruptDelayUnFinish()){
			Report_millis(); UART_puts(" Timeout <--\r\n");
		}		
	#endif

	return false;
}
uint8_t checkReturn(char *compareWord){
	
	if(receiveSerial()){
		#if ESP_DEBUG
			Report_millis(); UART_puts("Compare:"); UART_puts(serialResult); UART_puts(" WITH:"); UART_puts(compareWord); UART_puts("<--\r\n");
		#endif

		if(strstr(serialResult, compareWord) != NULL){
			return true;
		}else{
			return false;
		}
	}else{
		#if ESP_DEBUG
			Report_millis(); UART_puts(" Return:"); UART_puts(serialResult); UART_puts("<--\r\n");
		#endif
		return false;
	}
}

uint8_t checkUntilTimeout(char *compareWord, uint8_t maxWaitTime){
	millis_t timeoutTimestamp  = _milliseconds + (maxWaitTime * 1000);

	do{
		if(checkReturn(compareWord))
			return true;
	}while(_milliseconds < timeoutTimestamp);
	return false;
}

uint8_t sendCommand(char *sentCommand, char *compareWord){
	UART_putc('\r'); UART_putc('\n'); UART_puts(sentCommand); UART_putc('\r'); UART_putc('\n');
	if(!checkUntilTimeout(sentCommand, 1))
		return false;

	return checkUntilTimeout(compareWord,1);
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

	commandName = commandDetail = NULL;
	if(!checkUntilTimeout("+IPD,", 5))	return false;

	currStrPos = strstr(serialResult, "+IPD,");
	currStrPos += 5;

	// No care String lenght
	if((currStrPos = strchr(currStrPos, '$')) == NULL)	return false;
	#if ESP_DEBUG
		Report_millis(); UART_puts("NMEA Data:"); UART_puts(currStrPos); UART_puts("<--\r\n");
	#endif

	if((checksumResult = nmea_get_checksum(currStrPos)) == 0)	return false;
	dataPtr = currStrPos + 1;

	#if ESP_DEBUG
		Report_millis(); UART_puts("NMEA Checksum calculate:"); UART_puts(itoa(checksumResult, numberResult, 16)); UART_puts("<--\r\n");
	#endif

	if((currStrPos = strchr(currStrPos, '*')) == NULL)	return false;
	checksum = strtol(currStrPos+1, NULL, 16);
	#if ESP_DEBUG
		Report_millis(); UART_puts("NMEA Checksum return:"); UART_puts(itoa(checksum, numberResult, 16)); UART_puts("<--\r\n");
	#endif
	if(checksum != checksumResult) return false;

	commandName = dataPtr;
	currStrPos = strchr(dataPtr, ',');
	if(currStrPos != NULL){
		*currStrPos = '\0'; // to spilt between command name and data
		commandDetail = currStrPos+1;
		currStrPos = strchr(commandDetail, '*'); //find before checksum
	}else{
		commandDetail = NULL; // No command detail
		currStrPos = strchr(commandName, '*'); //find before checksum
	}
	
	*currStrPos = '\0'; //discard checksum

	return true;
}

void getIRCommand(char *ptr){
	char *tmp, *endptr;
	IRCommand = strtoul(tmp, &endptr, 16);
	++endptr;
	nbits = (int)strtol(endptr, NULL, 16);
}

//for easier variable borrowing
#define statusValue retryCount
#define nmeaChecksumValue retryCount

int main(){
	uint8_t retryCount = 0;
	char commandID[8] = "00000", number[8];

	millis_Init();
	UART_init();
	DDRB = (_BV(1) | _BV(2));
	PORTB &= ~(_BV(1) | _BV(2));
	serialResult[SERIAL_RESULT_BUFFER_SIZE-1] = '\0';
	#if ESP_DEBUG
		Report_millis(); UART_puts("ATMega Start! <<--\r\n");
	#endif
	UART_puts("\r\nAT+RST\r\n");
	_delay_ms(2000);
	UART_flush();
	while(1){
		while(ESP_CURRENT_STATE < ESP_STATE_TCPCONNECT){
			PORTB |= _BV(2);
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
					_delay_ms(4000);
					ESP_CURRENT_STATE = ESP_STATE_CONNECT_RESPONSE;
					retryCount = 0;
				break;
				case ESP_STATE_CONNECT_RESPONSE:
					UART_flush();
					if(sendCommand("AT+CIPSTATUS", "STATUS:")){
						statusValue = strtol(strchr(serialResult, ':'), NULL, 10);
						#if ESP_DEBUG
							Report_millis(); UART_puts("WIFI CIP Status:"); UART_putc('0'+statusValue); UART_puts("<--\r\n");
						#endif
						if(statusValue >= 2){
							ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
							retryCount = 0;
						}else{
							ESP_CURRENT_STATE = ESP_STATE_CONNECT;
						}
					}else if(strstr(serialResult, "OK")){
						retryCount = 0;
					}else if(strstr(serialResult, "CONNECT")){
						ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
						retryCount = 0;
					}else if(!strstr(serialResult, "ERROR")){
						++retryCount;
						_delay_ms(2000);
						#if ESP_DEBUG
							Report_millis(); UART_puts("WIFI Connect Attemp:"); UART_putc('0'+retryCount); UART_puts("<--\r\n");
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
					_delay_ms(500);
					UART_flush();
					if(sendCommand("AT+CIPSTART=\"UDP\",\"0\",0,10000,2", "CONNECT") || strstr(serialResult, "OK") || strstr(serialResult, "READY")){
						ESP_CURRENT_STATE = ESP_STATE_WAITIP;
						UART_flush();
					}else{
						++retryCount;
						#if ESP_DEBUG
							Report_millis(); UART_puts("WIFI Connect Attemp:"); UART_putc('0'+retryCount); UART_puts("<--\r\n");
						#endif
						if(retryCount > 5)
							ESP_CURRENT_STATE = ESP_STATE_CONNECT_MUX;
					}
				break;
				case ESP_STATE_WAITIP:
					if(detectNMEA() && commandName && commandDetail && strstr(commandName, "IRCIP")){
						#if ESP_DEBUG
							Report_millis(); UART_puts("NMEA Name:"); UART_puts(commandName); 
							if(commandDetail){
								UART_putc(':');
								UART_puts(commandDetail);
							}
							UART_puts("<--\r\n");
						#endif
						strlcpy(tcpIP, commandDetail, 16);
						 ESP_CURRENT_STATE = ESP_STATE_TCPCONNECT;
					}
					UART_flush();
					#if ESP_DEBUG
						Report_millis(); UART_puts("ESP Return:"); UART_puts(serialResult); UART_puts("<--\r\n");
					#endif
				break;
			}
			if(ESP_CURRENT_STATE < ESP_STATE_WAITIP)
				PORTB &= ~_BV(2);
			_delay_ms(100);
		}

		retryCount = 0;
		//Ready to connect TCP	
		while(retryCount <= TCP_RETRY){
			PORTB &= ~_BV(2);
			UART_puts("\r\nAT+CIPCLOSE\r\n");
			_delay_ms(100);
			UART_flush();
			UART_puts("\r\nAT+CIPSTART=\"TCP\",\""); UART_puts(tcpIP); UART_puts("\",10000\r\n");
			UART_flush();
			_delay_ms(500);
			strcpy(serialResult, "$IRREQ,"); strcat(serialResult, commandID); strcat(serialResult, "*");
			nmeaChecksumValue = nmea_get_checksum(serialResult); strcat(serialResult, itoa(nmeaChecksumValue, number, 16));
			UART_puts("\r\nAT+CIPSEND="); UART_puts(itoa(strlen(serialResult), number, 10)); UART_puts("\r\n");
			
			PORTB |= _BV(2);
			_delay_ms(500);
			UART_puts(serialResult);

			strcpy(commandID, "00000");
			if(detectNMEA()){
				PORTB &= ~_BV(2);
				if(strstr(commandName, "IRCMD") !== NULL){
					// CommandID
					commandDetail = strchr(commandDetail, ','); // Skip Remaining command count
					tmp = strchr(commandDetail, ','); tmp = '\0'; // Fake EOS
					strcpy(commandID, commandDetail);
					commandDetail = tmp + 1; // go to protocol

					//
					tmp = strchr(commandDetail, ','); tmp = '\0';
					if(strstr(commandDetail, "NEC")){
						getIRCommand();
						#if SEND_NEC
						sendNEC(IRCommand, nbits);
						#endif
					}else if(strstr(commandDetail, "RC5")){
						getIRCommand();
						#if SEND_RC5
						sendRC5(IRCommand, nbits);
						#endif
					}else if(strstr(commandDetail, "RC6")){
						getIRCommand();
						#if SEND_RC6
						sendRC6(IRCommand, nbits);
						#endif
					}else if(strstr(commandDetail, "PAN")){
						getIRCommand();
						#if SEND_PANASONIC
						sendPanasonic((unsigned long)nbits, IRCommand);
						#endif
					}else  if(strstr(commandDetail, "SAM")){
						getIRCommand();
						#if SEND_SAMSUNG
						sendSAMSUNG((unsigned long)nbits, IRCommand);
						#endif
					}
				}
			}else{
				++retryCount;
			}
		}

		ESP_CURRENT_STATE = ESP_STATE_INIT;
	}

	return 0;
}
