#ifndef ModeReactive_h
#define ModeReactive_h

#include "Mode.h"
#include "CaneCommon.h"

#define ZERO_LEDS  12
#define DOTS_SCALE  3
#define DOT_ENCOUNTER_RIPPLE 10

class ModeReactive : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { cycleIndex = 0; fallingDot[0] = 0; dotRising[0] = false; fallingDot[0] = LEDS; dotRising[1] = false; }
    virtual bool step(unsigned long dt) {
      int lights = (int)((currentX < 0 ? -currentX : currentX) * (LEDS - ZERO_LEDS) / ONE_G);
      for (int16_t i=0; i<LEDS; i++) {
        int distance = min(abs(fallingDot[0]/DOTS_SCALE - i), abs(fallingDot[1]/DOTS_SCALE - i));
        if (i >= lights || distance == 0) {
          strip.setPixelColor(i, wheel(((cycleIndex/2)+i) & 0xFF));
        } else if (abs(fallingDot[0] - fallingDot[1]) <= 1 && distance <= DOT_ENCOUNTER_RIPPLE) {
          int brightness = map(distance, 0, DOT_ENCOUNTER_RIPPLE, 255, 40);
          strip.setPixelColor(i, wheel(((cycleIndex/2)+i) & 0xFF, brightness));
        } else {
          strip.setPixelColor(i, 0x0);
        }
      }
      for (uint16_t i=0; i<2; i++) {
        if (dotRising[i]) {
          if (--fallingDot[i] < 0) {
            fallingDot[i] = 0;
            dotRising[i] = false;
          }
        } else if (++fallingDot[i] >= lights*DOTS_SCALE) {
          fallingDot[i]--;
          dotRising[i] = true;
        }
      }
      if (++cycleIndex >= 256*2) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }
  private:
    int cycleIndex;
    int fallingDot[2];
    boolean dotRising[2];
};

#endif

