/*
  SerialLCD Library - Cursor
 
 Demonstrates the use a 16x2 LCD SerialLCD driver from Seeedstudio.
 
 This sketch prints "Hello World!" to the LCD and
 uses the cursor()  and noCursor() methods to turn
 on and off the cursor.
 
 Library originally added 16 Dec. 2010
 by Jimbo.we 
 Library modified 15 March,2012
 by Frankie.Chu
 http://www.seeedstudio.com
 */

// include the library code:
#include "SerialLCD.h"
#include <SoftwareSerial.h> //this is a must

int sensorPin = A2;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor
float temperature;

// initialize the library
SerialLCD slcd(4, 5);//this is a must, assign soft serial pins

void setup() {
  // set up
  slcd.begin();
  slcd.noCursor();
  slcd.noBacklight();
}

void loop() {
  sensorValue = analogRead(sensorPin);
  temperature = (float)(1023 - sensorValue) * 10000 / sensorValue; //get the resistance of the sensor;
  temperature = 1 / (log(temperature / 10000) / 3975 + 1/298.15) - 273.15;//convert to temperature via datasheet ;

  slcd.home();
  slcd.print("Temp: ");
  slcd.print(int(temperature), 10);
  slcd.print("    ");
}
