/*
 
  The circuit:
 * LCD RS pin to digital pin 3
 * LCD Enable pin to digital pin 2
 * LCD D4 pin to digital pin 6
 * LCD D5 pin to digital pin 7
 * LCD D6 pin to digital pin 8
 * LCD D7 pin to digital pin 9
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 */

// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
#include <SPI.h>

#include "RTC.h"
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

// 1 = grass
// 2 = drip

#define DAYS_PERIOD_R1    2
#define DAYS_PERIOD_R2    3
#define MINUTES_R1       (7 * 60)
#define MINUTES_R2       (15 * 60)

byte start1[] = {3};
byte start2[] = {3};

#define RELAY1   10
#define RELAY2   14

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
// First = CE. Second = CS
RF24 radio(5, 4);

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(3, 2, 6, 7, 8, 9);

void twoDigit(int x) {
  if (x < 10)
    lcd.print("0");
  lcd.print(x);
}

int daysFromStartOfYear() {
  switch(month) {
    case 1:
      return dayOfMonth;
    case 2:
      return 31+dayOfMonth;
    case 3:
      return 59+dayOfMonth;
    case 4:
      return 90+dayOfMonth;
    case 5:
      return 120+dayOfMonth;
    case 6:
      return 151+dayOfMonth;
    case 7:
      return 181+dayOfMonth;
    case 8:
      return 212+dayOfMonth;
    case 9:
      return 243+dayOfMonth;
    case 10:
      return 273+dayOfMonth;
    case 11:
      return 304+dayOfMonth;
    case 12:
      return 334+dayOfMonth;
  }
}

unsigned long secondsIntoHour() {
  return minute * 60 + second;
}

void check(int *force, unsigned long *forceTimeOn, unsigned long *forceTimeOff, unsigned long *subtract, byte starts[], byte startsSize, int seconds, byte period, int output) {
  boolean state = false;
  boolean disabled = false;
  if (*forceTimeOn > 0 && millis() > *forceTimeOn) {
    *forceTimeOn = 0;
  }
  if (*forceTimeOff > 0 && millis() > *forceTimeOff) {
    *forceTimeOff = 0;
  }
  
  if (*force == -1) {
    state = true;
  } else if (*force == -2) {
    state = false;
    disabled = true;
    lcd.print("DS");
  } else if (*forceTimeOn > 0) {
    state = true;
  } else if (*forceTimeOff > 0) {
    disabled = true;
    long disabledHours = (millis() - *forceTimeOff) / (60 * 60000);
    twoDigit((int)disabledHours);
  } else if ((*force) == 0 && (daysFromStartOfYear() % period) == 0) {
    for (byte i = 0; i < startsSize; i++) {
      if (hour == starts[i]) {
        if (secondsIntoHour() < (seconds - *subtract)) {
          state = true;
        } else {
          *subtract = 0;
        }
        break;
      }
    }
  }
  if (state) {
    digitalWrite(output, HIGH);
    lcd.print("ON");
  } else {
    digitalWrite(output, LOW);
    if (!disabled) {
      lcd.print("--");
    }      
  }
}

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void setup() {
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  
  Serial.begin(9600);
  printf_begin();

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);

  RTCStart();
  RTCGetDateDs1307();
  
  START_RADIO(radio, RADIO_SPRINKLER);
  radio.printDetails();
}

int command = 0;
int aux1, aux2;
byte inByte;

int forceR1 = 0;
int forceR2 = 0;

unsigned long forceOn1 = 0;
unsigned long forceOn2 = 0;

unsigned long forceOff1 = 0;
unsigned long forceOff2 = 0;

unsigned long subtractT1 = 0;
unsigned long subtractT2 = 0;

unsigned long mirfData;

void loop() {
  if (Serial.available()) {  // Look for char in serial que and process if found
    command = Serial.read();
    Serial.print("Command: ");
    Serial.println(command);  // Echo command CHAR in ascii that was sent
    switch (command) {
      case 'T':      //If command = "T" Set Date
        RTCSetDateDs1307();
        RTCGetDateDs1307();
        Serial.println("");
        break;

      case 'R':
        aux1 = Serial.read();
        aux2 = Serial.read();
        if (aux1 == '1') {
          forceR1 = aux2 == '1' ? -1 : aux2 == '0' ? -2 : 0;
        } else if (aux1 == '2') {
          forceR2 = aux2 == '1' ? -1 : aux2 == '0' ? -2 : 0;
        }
        break;
    }
  }

  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    Serial.print("Received data ");
    Serial.print(inByte);
    Serial.print(' ');
    Serial.println(mirfData);
    switch (inByte) {
      case 'R':
        aux1 = (mirfData >>  8) & 0xFF;
        aux2 = (mirfData >> 16) & 0xFF;
        Serial.print("Received relay ");
        Serial.print((char)aux1);
        Serial.print(' ');
        Serial.println(aux2);
        if (aux1 == '1') {
          forceR1 = aux2 == '1' ? -1 : aux2 == '0' ? -2 : 0;
        } else if (aux1 == '2') {
          forceR2 = aux2 == '1' ? -1 : aux2 == '0' ? -2 : 0;
        }
        break;

      case 'T':
        aux1 = (mirfData >>  8) & 0xFF;
        aux2 = (mirfData >> 16) & 0xFFFF;
        Serial.print("Received time ");
        Serial.print((char)aux1);
        Serial.print(' ');
        Serial.println(aux2);
        if (aux1 == '1') {
          subtractT1 += (unsigned long)aux2;
          forceOn1 = millis() + (unsigned long)aux2 * 1000;
        } else if (aux1 == '2') {
          subtractT2 += (unsigned long)aux2;
          forceOn2 = millis() + (unsigned long)aux2 * 1000;
        }
        break;

      case 'D':
        aux1 = (mirfData >>  8) & 0xFF;
        aux2 = (mirfData >> 16) & 0xFFFF;
        Serial.print("Received disable ");
        Serial.print((char)aux1);
        Serial.print(' ');
        Serial.println(aux2);
        if (aux1 == '1') {
          forceOff1 = millis() + (unsigned long)aux2 * 60000;
        } else if (aux1 == '2') {
          forceOff2 = millis() + (unsigned long)aux2 * 60000;
        }
        break;
    }
  }

  delay(100);

  RTCGetDateDs1307();

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  switch(dayOfWeek) {
    case 1:
      lcd.print("Sun");
      break;
    case 2:
      lcd.print("Mon");
      break;
    case 3:
      lcd.print("Tue");
      break;
    case 4:
      lcd.print("Wed");
      break;
    case 5:
      lcd.print("Thu");
      break;
    case 6:
      lcd.print("Fri");
      break;
    case 7:
      lcd.print("Sat");
      break;
    default:
      lcd.print("??");
      lcd.print(dayOfWeek);
      break;
  }
  lcd.print(" ");
  twoDigit(month);
  lcd.print("/");
  twoDigit(dayOfMonth);
  lcd.print("/");
  twoDigit(year);
  lcd.print(" ");
  twoDigit(daysFromStartOfYear());

  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  twoDigit(hour);
  lcd.print(":");
  twoDigit(minute);
  lcd.print(":");
  twoDigit(second);
  lcd.print(" ");

  check(&forceR1, &forceOn1, &forceOff1, &subtractT1, start1, sizeof(start1)/sizeof(byte), MINUTES_R1, DAYS_PERIOD_R1, RELAY1);
  lcd.print(" ");
  check(&forceR2, &forceOn2, &forceOff2, &subtractT2, start2, sizeof(start2)/sizeof(byte), MINUTES_R2, DAYS_PERIOD_R2, RELAY2);
}

