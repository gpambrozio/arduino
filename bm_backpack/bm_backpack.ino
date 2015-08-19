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

#include "SimpleVolume.h"

#define AGC_CONTROL  17

#define DATA_PIN  10   // Yellow
#define CLOCK_PIN 11   // Green

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(25, DATA_PIN, CLOCK_PIN);

// MUSIC STUFF  ---------------------------------------------------------
#define SAMPLES   12  // Length of buffer for dynamic level adjustment
#define MIC_PIN      A2  // Microphone is attached to this analog pin
#define TOP   6  // A bit more to go off scale.

SimpleVolume sv = SimpleVolume(MIC_PIN, SAMPLES, TOP, 256);

void setup() {
  pinMode(AGC_CONTROL, INPUT);

  strip.begin();
  strip.show();

  Serial.begin(9600);
}

void drawLine(uint16_t x, uint16_t height, uint32_t color) {
  for (int i=0;i<5;i++) {
    if (i<height) {
      strip.setPixelColor((x % 2 == 1 ? (4 - i) : i) + x * 5, color);
    } else {
      strip.setPixelColor((x % 2 == 1 ? (4 - i) : i) + x * 5, 0);
    }
  }
  strip.show();
}

int i;
uint8_t currentColumn;

void loop() {
  int height = sv.getVolume();

  // Color pixels based on rainbow gradient
  drawLine(currentColumn, height, Wheel(map(height,0,TOP,30,150)));
  if (++currentColumn >= 5) currentColumn = 0;

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

