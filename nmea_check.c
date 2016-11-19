#include <stdio.h>
#include <string.h>

#define SERIAL_RESULT_BUFFER_SIZE 112

char nmea_get_checksum(const char *sentence){
    const char *n = sentence + 1; // Plus one, skip '$'
    char chk = 0;

    /* While current char isn't '*' or sentence ending (newline) */
    while ('*' != *n && '\r' != *n && '\n' != *n) {
        if ('\0' == *n || n - sentence > SERIAL_RESULT_BUFFER_SIZE) {
            /* Sentence too long or short */
            return 0;
        }
        chk ^= (char) *n;
        n++;
    }

    return chk;
}

int main(){
    printf("-->%u\n", nmea_get_checksum("$IRCIP,192.168.1.106*"));
    return 1;
}
