/*
 Based on this project: http://davidbliss.com/2014/11/18/transforming-sphere-lamp/
 */

#include <SPI.h>
#include <Stepper.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define SOUND_INPUT  A3
#define FULL_MOVEMENT  14700
#define INITIAL_SPEED 150

#define EEPROM_STATE   0

RF24 radio(9, 10);   // CE, CSN
Stepper myStepper = Stepper(200, 4, 3, 5, 6);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(114, A0, NEO_GRB + NEO_KHZ800);

int inByte = 0;         // incoming serial byte
int inByte2 = 0;
int aux1, aux2;

#define STATE_CLOSE 0
#define STATE_OPEN  1

int state;
unsigned long lastContactTime = 0;
unsigned long mirfData;

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void move(boolean shouldOpen) {
  myStepper.step((shouldOpen ? 1 : -1) * FULL_MOVEMENT);
  state = shouldOpen ? STATE_OPEN : STATE_CLOSE;
  EEPROM.write(EEPROM_STATE, state);
}

void setup() {
  Serial.begin(9600);
  printf_begin();
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  START_RADIO(radio, RADIO_LIGHTBALL);
  radio.printDetails();
  
  myStepper.setSpeed(INITIAL_SPEED);

  state = EEPROM.read(EEPROM_STATE);
  move(state != STATE_OPEN);
}

void loop() {
  if (Serial.available()) {  // Look for char in serial que and process if found
    inByte = Serial.read();
    Serial.print("Command: ");
    Serial.println(inByte);  // Echo command CHAR in ascii that was sent
    switch (inByte) {
      case 'S':
      {
        int sp = Serial.parseInt();
        Serial.print("Setting speed to ");
        Serial.println(sp);
        myStepper.setSpeed(sp);
        break;
      }
        
      case 'o':
      case 'c':
      {
        int steps = Serial.parseInt();
        Serial.print("Moving ");
        Serial.println(steps);
        myStepper.step((inByte == 'c' ? -1 : 1) * steps);
        break;
      }
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
    }
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

