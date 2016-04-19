#include "Arduino.h"
#include <Adafruit_NeoPixel.h>

#include "Mode.h"
#include "CaneCommon.h"

Mode::Mode() {
}

uint32_t Mode::wheel(byte WheelPos, int brightness) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
   return strip.Color((brightness * (255 - WheelPos * 3)) >> 8, 0, (brightness * WheelPos * 3) >> 8);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, (brightness * WheelPos * 3) >> 8, (brightness * (255 - WheelPos * 3)) >> 8);
  } else {
   WheelPos -= 170;
   return strip.Color((brightness * WheelPos * 3) >> 8, (brightness * (255 - WheelPos * 3)) >> 8, 0);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Mode::wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void Mode::init() {
}

bool Mode::step(unsigned long dt) {
}

