/*
  LiquidCrystal Library - Hello World
 
 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the 
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.
 
 This sketch prints "Hello World!" to the LCD
 and shows the time.
 
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>

#define clockAddress 0x68

#define RELAY1   10
#define RELAY2   11

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(0, 2, 6, 7, 8, 9);

long previousMillis = 0;  // will store last time Temp was updated
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
byte test; 

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers, 
// Probably need to put in checks for valid numbers.
void setDateDs1307()                
{
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
  Wire.endTransmission();
}

// Gets the date and time from the ds1307 and prints result
void getDateDs1307() {
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
  Serial.print(year, DEC);
}


void setup() {
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
  
  Wire.begin();
  Serial.begin(9600);
  
  getDateDs1307();
}

int command = 0;

void loop() {
  if (Serial.available()) {  // Look for char in serial que and process if found
    command = Serial.read();
    if (command == 'T') {      //If command = "T" Set Date
      setDateDs1307();
      getDateDs1307();
      Serial.println("");
    }
    else if (command == 'Q') {  //If command = "Q" RTC1307 Memory Functions
      delay(100);
      if (Serial.available()) {
        command = Serial.read(); 
        
        // If command = "1" RTC1307 Initialize Memory - All Data will be set to 255 (0xff).  
        // Therefore 255 or 0 will be an invalid value.  
        if (command == '1') { 
          
          // 255 will be the init value and 0 will be cosidered an error that 
          // occurs when the RTC is in Battery mode. 
          Wire.beginTransmission(clockAddress);
          
          // Set the register pointer to be just past the date/time registers.
          Wire.write(byte(0x08));  
          for (int i = 1; i <= 27; i++) {
            Wire.write(byte(0xff));
            delay(100);
          }   
          Wire.endTransmission();
          getDateDs1307();
          Serial.println(": RTC1307 Initialized Memory");
        }
        else if (command == '2') {      //If command = "2" RTC1307 Memory Dump
          getDateDs1307();
          Serial.println(": RTC 1307 Dump Begin");
          Wire.beginTransmission(clockAddress);
          Wire.write(byte(0x00));
          Wire.endTransmission();
          Wire.requestFrom(clockAddress, 64);
          for (int i = 1; i <= 64; i++) {
            test = Wire.read();
            Serial.print(i);
            Serial.print(":");
            Serial.println(test, DEC);
          }
          Serial.println(" RTC1307 Dump end");
        } 
      }  
    }
    Serial.print("Command: ");
    Serial.println(command);  // Echo command CHAR in ascii that was sent
  }

  command = 0;  // reset command                  
  delay(100);

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(hour);
  lcd.print(":");
  lcd.print(minute);
  lcd.print(":");
  lcd.print(second);
}

