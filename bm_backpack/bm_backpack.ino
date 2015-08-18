/*
PICCOLO is a tiny Arduino-based audio visualizer...a bit like
Spectro, but smaller, with microphone input rather than line-in.

Hardware requirements:
 - Most Arduino or Arduino-compatible boards (ATmega 328P or better).
 - Adafruit Bicolor LED Matrix with I2C Backpack (ID: 902)
 - Adafruit Electret Microphone Amplifier (ID: 1063)
 - Optional: battery for portable use (else power through USB)
Software requirements:
 - elm-chan's ffft library for Arduino

Connections:
 - 3.3V to mic amp+ and Arduino AREF pin <-- important!
 - GND to mic amp-
 - Analog pin 0 to mic amp output
 - +5V, GND, SDA (or analog 4) and SCL (analog 5) to I2C Matrix backpack

Written by Adafruit Industries.  Distributed under the BSD license --
see license.txt for more information.  This paragraph must be included
in any redistribution.
*/

#include <SPI.h>
#include <Adafruit_WS2801.h>

#define MIC_PIN      A2  // Microphone is attached to this analog pin

#define AGC_CONTROL  17

#define TOP   6  // A bit more to go off scale.

#define DATA_PIN  10   // Yellow
#define CLOCK_PIN 11   // Green

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(25, DATA_PIN, CLOCK_PIN);

// MUSIC STUFF  ---------------------------------------------------------
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   12  // Length of buffer for dynamic level adjustment

int vol[SAMPLES];

uint8_t volCount;    // Frame counter for storing past volume data
uint8_t currentColumn;

int lvl;          // Current "dampened" audio level
int minLvlAvg;    // For dynamic adjustment of graph low & high
int maxLvlAvg;
int height;

uint16_t minLvl;
uint16_t maxLvl;

void setup() {
  pinMode(AGC_CONTROL, INPUT);

  strip.begin();
  strip.show();

  memset(vol, 0, sizeof(int) * SAMPLES);
}

void drawLine(uint16_t x, uint16_t height, uint32_t color) {
  for (int i=0;i<5;i++) {
    if (i<height) {
      strip.setPixelColor((x % 2 == 1 ? (4 - i) : i) + x * 5, 0);
    } else {
      strip.setPixelColor((x % 2 == 1 ? (4 - i) : i) + x * 5, color);
    }
  }
  strip.show();
}

int i;

void loop() {

  height   = analogRead(MIC_PIN);                        // Raw reading from mic
  height   = abs(height - 512 - DC_OFFSET); // Center on zero
  height   = (height <= NOISE) ? 0 : (height - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + height) >> 3;    // "Dampened" reading (else looks twitchy)

  vol[volCount] = height;                 // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;

  // Color pixels based on rainbow gradient
  drawLine(currentColumn, height, Wheel(map(height,0,TOP,30,150)));
  if (++currentColumn >= 5) currentColumn = 0;

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];

  for(i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl)      minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
  
  delay(2);
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = (byte)(0.05 * r);
  c <<= 8;
  c |= (byte)(0.05 * g);
  c <<= 8;
  c |= (byte)(0.05 * b);
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

