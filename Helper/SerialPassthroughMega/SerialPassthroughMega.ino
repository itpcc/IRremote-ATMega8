/*
  SerialPassthrough sketch

  Some boards, like the Arduino 101, the MKR1000, Zero, or the Micro,
  have one hardware serial port attached to Digital pins 0-1, and a
  separate USB serial port attached to the IDE Serial Monitor.
  This means that the "serial passthrough" which is possible with
  the Arduino UNO (commonly used to interact with devices/shields that
  require configuration via serial AT commands) will not work by default.

  This sketch allows you to  emulate the serial passthrough behaviour.
  Any text you type in the IDE Serial monitor will be written
  out to the serial port on Digital pins 0 and 1, and vice-versa.

  On the 101, MKR1000, Zero, and Micro, "Serial" refers to the USB Serial port
  attached to the Serial Monitor, and "Serial1" refers to the hardware
  serial port attached to pins 0 and 1. This sketch will emulate Serial passthrough
  using those two Serial ports on the boards mentioned above,
  but you can change these names to connect any two serial ports on a board
  that has multiple ports.

   Created 23 May 2016
   by Erik Nyquist
*/

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200); //ATMega
  Serial2.begin(115200); //ESP8266
}

char ch, recentCH;
char isForward = 4;
String str;

void loop() {
  if (Serial.available()) {      // If anything comes in Serial (USB),
    Serial2.write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)
  }

  if (Serial1.available()) {     // If anything comes in Serial1 (ATMega)
    ch = Serial1.read();
    if(recentCH == '\n' && ch == '-')
      isForward = 0;
    else if(isForward == 0)
      isForward = 1;
    else if(isForward == 1 && ch == '\r')
      isForward = 2;
    else if(isForward == 2 && ch == '\n')
      isForward = 3;
    else if(isForward == 3)
      isForward = 4;
    
    /*if(isForward == 0)
      Serial.print("ATMega: ");
    else */if(isForward == 4)
      Serial2.write(ch);
    
    Serial.write(ch);   // read it and send it out Serial (USB)
    recentCH = ch;
  }
  if (Serial2.available()) {     // If anything comes in Serial1 (ESP8266)
    ch = Serial2.read();
    Serial1.write(ch);   // read it and send it out Serial1 (pins 0 & 1)
    /*Serial.write(ch);
    if(ch == '\n') Serial.print("              ");*/
    /*str = Serial2.readString();
    Serial1.print(str);
    Serial.print("\r\nESP8266:");
    Serial.print(str);   // read it and send it out Serial (USB)*/
  }
}
