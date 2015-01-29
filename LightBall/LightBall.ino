/*
 Based on this project: http://davidbliss.com/2014/11/18/transforming-sphere-lamp/
 */

#include <SPI.h>
#include <Stepper.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define SOUND_INPUT  A3
#define EOM_INPUT    7
#define FULL_MOVEMENT  14800
#define INITIAL_SPEED 150

#define DC_OFFSET  256  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES 16
#define MAX_HEIGHT 255

#define EEPROM_MODE  0
#define EEPROM_BRIGHTNESS 1

RF24 radio(9, 10);   // CE, CSN
Stepper myStepper = Stepper(200, 4, 3, 5, 6);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(90, A0, NEO_GRB + NEO_KHZ800);

int inByte = 0;         // incoming serial byte
int inByte2 = 0;
int aux1, aux2;
int vol[SAMPLES];

uint8_t volCount;    // Frame counter for storing past volume data
uint8_t currentColumn;

int lvl;          // Current "dampened" audio level
int minLvlAvg;    // For dynamic adjustment of graph low & high
int maxLvlAvg;
int height;

uint16_t minLvl;
uint16_t maxLvl;

int stepperPosition = 0;
int stepperGoal = 0;

unsigned long mirfData;

typedef enum {
    LightModeSound = 0,
    LightModeRainbow,
    LightModeLight,
    LightModeCount,
} LightMode;

int mode = LightModeCount;
int brightness = 255;

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void close() {
  move(-FULL_MOVEMENT);
}

void open() {
  while (digitalRead(EOM_INPUT) != LOW) {
    myStepper.step(100);
  }
  stepperPosition = stepperGoal = FULL_MOVEMENT;
}

void move(int steps) {
  stepperGoal = stepperPosition + steps;
  if (stepperGoal < 0) {
    stepperGoal = 0;
  } else if (stepperGoal > FULL_MOVEMENT) {
    stepperGoal = FULL_MOVEMENT;
  }
}

void setBrightness(int b) {
  brightness = b;
  strip.setBrightness(brightness);
  strip.show();
  EEPROM.write(EEPROM_BRIGHTNESS, brightness);
}

void changeMode(int newMode, boolean force) {
  if (mode != newMode || force) {
    mode = newMode;
    EEPROM.write(EEPROM_MODE, mode);
    switch(mode) {
      case LightModeSound:
        colorWipe(0);
        for(int i=1; i<SAMPLES; i++) {
          vol[i] = 0;
        }
        maxLvl = 100;
        minLvl = 
        currentColumn = 
        volCount = 0;
        break;
        
      case LightModeRainbow:
        currentColumn = 0;
        break;
        
      case LightModeLight:
        colorWipe(strip.Color(255, 255, 255));
        break;
    }
  }
}

void setup() {
  // Stabilize power, etc...
  delay(200);

  Serial.begin(9600);
  printf_begin();
  
  pinMode(EOM_INPUT, INPUT_PULLUP);
  
  START_RADIO(radio, RADIO_LIGHTBALL);
  radio.printDetails();
  
  myStepper.setSpeed(INITIAL_SPEED);

  strip.begin();

  if (digitalRead(EOM_INPUT) != LOW) {
    open();
  } else {
    stepperPosition = stepperGoal = FULL_MOVEMENT;
  }
  setBrightness(EEPROM.read(EEPROM_BRIGHTNESS));
  changeMode(EEPROM.read(EEPROM_MODE), true);
}

void loop() {
  if (stepperGoal != stepperPosition) {
    int steps = stepperGoal - stepperPosition;
    if (steps < -10) {
      steps = -10;
    } else if (steps > 10) {
      steps = 10;
    }
    myStepper.step(steps);
    stepperPosition += steps;
  }

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
        move((inByte == 'c' ? -1 : 1) * steps);
        break;
      }
      
      case 's':
        changeMode(LightModeSound, false);
        break;
      
      case 'l':
        setBrightness(255);
        changeMode(LightModeLight, true);
        break;
      
      case 'r':
        setBrightness(255);
        changeMode(LightModeRainbow, true);
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
      case 'S':
      {
        unsigned long sp = ((mirfData >> 8) & 0xFFFF);
        Serial.print("Setting speed to ");
        Serial.println(sp);
        myStepper.setSpeed(sp);
        break;
      }

      case 'o':
      case 'c':
      {
        unsigned long steps = ((mirfData >> 8) & 0xFFFF);
        Serial.print("Moving ");
        Serial.println(steps);
        move((inByte == 'c' ? -1 : 1) * steps);
        break;
      }

      case 's':
        changeMode(LightModeSound, false);
        break;
      
      case 'l':
      {
        setBrightness(((mirfData >> 8) & 0xFF));
        changeMode(LightModeLight, true);
        break;
      }
      
      case 'r':
        setBrightness(((mirfData >> 8) & 0xFF));
        changeMode(LightModeRainbow, true);
        break;
    }
  }
  
  switch (mode) {
    case LightModeSound:
      height   = analogRead(SOUND_INPUT);                        // Raw reading from mic
      height   = abs(height - DC_OFFSET); // Center on zero
      height   = (height <= NOISE) ? 0 : (height - NOISE);             // Remove noise/hum
      lvl = ((lvl * 7) + height) >> 3;    // "Dampened" reading (else looks twitchy)
    
      vol[volCount] = height;                 // Save sample for dynamic leveling
      if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
     
      // Calculate bar height based on dynamic min/max levels (fixed point):
      height = MAX_HEIGHT * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
      if(height < 0L)       height = 0;      // Clip output
      else if(height > MAX_HEIGHT) height = MAX_HEIGHT;
    
      strip.setBrightness(height);
      rainbowCycle(currentColumn);
      if (++currentColumn >= 255 * 5) currentColumn = 0;

      // Get volume range of prior frames
      minLvl = maxLvl = vol[0];
    
      for(int i=1; i<SAMPLES; i++) {
        if(vol[i] < minLvl)      minLvl = vol[i];
        else if(vol[i] > maxLvl) maxLvl = vol[i];
      }
    
      // minLvl and maxLvl indicate the volume range over prior frames, used
      // for vertically scaling the output graph (so it looks interesting
      // regardless of volume level).  If they're too close together though
      // (e.g. at very low volume levels) the graph becomes super coarse
      // and 'jumpy'...so keep some minimum distance between them (this
      // also lets the graph go to zero when no sound is playing):
      if((maxLvl - minLvl) < MAX_HEIGHT) maxLvl = minLvl + MAX_HEIGHT;
      minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
      maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)    
      break;
      
    case LightModeRainbow:
      rainbowCycle(currentColumn);
      if (++currentColumn >= 255 * 5) currentColumn = 0;
      delay(5);
      break;
      
    case LightModeLight:
      // Nothing to do really
      break;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }
  strip.show();
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

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
}


