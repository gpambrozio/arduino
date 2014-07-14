/*
  SerialLCD Library - Hello World
 
 Demonstrates the use a 16x2 LCD SerialLCD driver from Seeedstudio.
 
 This sketch prints "Hello, Seeeduino!" to the LCD
 and shows the time.
 
 Library originally added 16 Dec. 2010
 by Jimbo.we 
 http://www.seeedstudio.com
 */

// include the library code:
#include <SerialLCD.h>
#include <SoftwareSerial.h> //this is a must

// initialize the library
SerialLCD slcd(11,12);//this is a must, assign soft serial pins

#define ANALOG_IN  A0
#define LED   13

unsigned long start5 = 0, start4 = 0, start3 = 0;
unsigned long end5 = 0, end4 = 0, end3 = 0;

void setup() {
  // set up
  slcd.begin();
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
  start5 = millis();
}

void loop() {
  // Print a message to the LCD.
  slcd.setCursor(0, 0);
  int volt = analogRead(ANALOG_IN);
  float voltage = 500.0 * ((float)volt / 1023.0);
  volt = (int)voltage;
  if (volt < 450 && end5 == 0) end5 = millis();
  if (volt < 400 && volt > 300 && start4 == 0) start4 = millis();
  if (volt < 300 && start3 == 0) { end4 = millis(); start3 = millis(); }
  if (volt < 200 && end3 == 0) { end3 = millis(); }
  slcd.print((int)voltage, DEC);
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  // print the number of seconds since reset:
  slcd.setCursor(0, 1);
  slcd.print("5=");
  if (end5 > 0)
    slcd.print((end5-start5)/60000, DEC);
  else
    slcd.print((millis()-start5)/60000, DEC);
  if (start4 > 0) {
    slcd.print("  4=");
    if (end4 > 0)
      slcd.print((end4-start4)/60000, DEC);
    else
      slcd.print((millis()-start4)/60000, DEC);
  }
  if (start3 > 0) {
    slcd.print("  3=");
    if (end3 > 0)
      slcd.print((end3-start3)/60000, DEC);
    else
      slcd.print((millis()-start3)/60000, DEC);
  }
}
