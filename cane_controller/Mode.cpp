#include "Arduino.h"

#include "Mode.h"
#include "CaneCommon.h"

Mode::Mode() {
}

CHSV Mode::wheel(byte WheelPos, int brightness) {
  CHSV hsv;
  hsv.hue = WheelPos;
  hsv.val = brightness;
  hsv.sat = 240;
  return hsv;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
CHSV Mode::wheel(byte WheelPos) {
  return wheel(WheelPos, 255);
}

void Mode::init() {
}

bool Mode::step(unsigned long dt) {
}

