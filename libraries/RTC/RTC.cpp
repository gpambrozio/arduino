#import "RTC.h"

#import <Wire.h>

#define clockAddress 0x68

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

void RTCStart() {
  Wire.begin();
}

byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// 4) DOW: 1 = Sun, 7 = Sat
// Assumes you're passing in valid numbers, 
// Probably need to put in checks for valid numbers.
void RTCSetDateDs1307() {
  while (Serial.available() < 13) {
    delay(1);
  }
  // Use of (byte) type casting and ascii math to achieve result.  
  second = (byte) ((Serial.read() - '0') * 10 + (Serial.read() - '0')); 
  minute = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  hour  = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  dayOfWeek = (byte) (Serial.read() - '0');
  dayOfMonth = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  month = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  year= (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  Wire.beginTransmission(clockAddress);
  Wire.write(byte(0x00));
  Wire.write(decToBcd(second));  // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));    // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.write(byte(0x00));
  Wire.endTransmission();
}

// Gets the date and time from the ds1307 and prints result
void RTCGetDateDs1307() {
  // Reset the register pointer
  Wire.beginTransmission(clockAddress);
  Wire.write(byte(0x00));
  Wire.endTransmission();

  Wire.requestFrom(clockAddress, 7);

  // A few of these need masks because certain bits are control bits
  second     = bcdToDec(Wire.read() & 0x7f);
  minute     = bcdToDec(Wire.read());
  
  // Need to change this if 12 hour am/pm
  hour       = bcdToDec(Wire.read() & 0x3f);  
  dayOfWeek  = bcdToDec(Wire.read());
  dayOfMonth = bcdToDec(Wire.read());
  month      = bcdToDec(Wire.read());
  year       = bcdToDec(Wire.read());
}

void RTCPrintDateToSerial() {
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.println(year, DEC);
}
