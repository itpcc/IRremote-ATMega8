#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERIAL_RESULT_BUFFER_SIZE 112

unsigned long IRCommand;
int nbits;

char* getCurrentColumn(char *currentPoint, char **nextpoint){
	*nextpoint = strchr(currentPoint, ',');
	printf("OK\n");
	if(!nextpoint) *nextpoint = strchr(currentPoint, '*');
	printf("OK\n");
	printf("*nextpoint = %c\n", **nextpoint);
	if(nextpoint){
        printf("*nextpoint = %c\n", **nextpoint);
		**nextpoint = '\0';
		(*nextpoint)++;
	}
	printf("f*** OK\n");
	return currentPoint;
}

void getIRCommand(char *ptr){
	char *endptr;
	IRCommand = strtoul(ptr, &endptr, 16);
	++endptr;
	nbits = (int)strtol(endptr, NULL, 16);
}


int main(){
    char _commandDetail[64] = "0,00001,NEC,A1DE21DE,32";
    char *commandDetail = _commandDetail;
    char commandID[8];
    char *tmp;
    printf("%s\n", getCurrentColumn(commandDetail, &tmp)); //Remaining
    commandDetail = tmp;

    strcpy(commandID, getCurrentColumn(commandDetail, &tmp)); //command detail
    commandDetail = getCurrentColumn(tmp, &tmp); // Protocol
    printf("\r\n-->PROTOCOL:"); printf(commandDetail); printf("<--\r\n");
    printf("CommandID: %s\n", commandID);
    getIRCommand(tmp);
    printf("A1DE21DE -> %x\n", IRCommand);
    return 1;
}
