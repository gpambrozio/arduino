/*
 Based on this project: http://davidbliss.com/2014/11/18/transforming-sphere-lamp/
 */

#include <SPI.h>
#include <Stepper.h>
#include <Adafruit_NeoPixel.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define SOUND_INPUT  A3
#define EOM_INPUT    7
#define FULL_MOVEMENT  14700
#define INITIAL_SPEED 150

#define DC_OFFSET  256  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES 16
#define MAX_HEIGHT 255

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

boolean openState;
unsigned long mirfData;

typedef enum {
    LightModeSound = 0,
    LightModeRainbow,
    LightModeLight,
    LightModeCount,
} LightMode;

int mode = LightModeCount;

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void close() {
  myStepper.step(-FULL_MOVEMENT);
  openState = false;
}

void open() {
  while (digitalRead(EOM_INPUT) != LOW) {
    myStepper.step(100);
  }
  openState = true;
}

void changeMode(int newMode) {
  if (mode != newMode) {
    mode = newMode;
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
        strip.setBrightness(255);
        colorWipe(strip.Color(255, 255, 255));
        break;
    }
  }
}

void setup() {
  Serial.begin(9600);
  printf_begin();
  
  pinMode(EOM_INPUT, INPUT_PULLUP);
  
  strip.begin();
  colorWipe(strip.Color(255, 255, 255));

  START_RADIO(radio, RADIO_LIGHTBALL);
  radio.printDetails();
  
  myStepper.setSpeed(INITIAL_SPEED);

  if (digitalRead(EOM_INPUT) != LOW) {
    open();
  } else {
    close();
  }
  changeMode(LightModeSound);
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
      
      case 's':
        changeMode(LightModeSound);
        break;
      
      case 'l':
        changeMode(LightModeLight);
        break;
      
      case 'r':
        changeMode(LightModeRainbow);
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
    }
  }
  
  switch (mode) {
    case LightModeSound:
      height   = analogRead(SOUND_INPUT);                        // Raw reading from mic
//      Serial.println(height);
      height   = abs(height - DC_OFFSET); // Center on zero
      height   = (height <= NOISE) ? 0 : (height - NOISE);             // Remove noise/hum
      lvl = ((lvl * 7) + height) >> 3;    // "Dampened" reading (else looks twitchy)
    
      vol[volCount] = height;                 // Save sample for dynamic leveling
      if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
     
      // Calculate bar height based on dynamic min/max levels (fixed point):
      height = MAX_HEIGHT * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
      if(height < 0L)       height = 0;      // Clip output
      else if(height > MAX_HEIGHT) height = MAX_HEIGHT;
    
      strip.setPixelColor(currentColumn, Wheel(height));
      strip.setBrightness(height);
      strip.show();
      if (++currentColumn >= strip.numPixels()) currentColumn = 0;
    
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
      rainbowCycle(5, currentColumn);
      if (++currentColumn >= 255 * 5) currentColumn = 0;
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
void rainbowCycle(uint8_t wait, uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
  delay(wait);
}


