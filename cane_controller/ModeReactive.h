#ifndef ModeReactive_h
#define ModeReactive_h

#include "Mode.h"
#include "CaneCommon.h"

#define ZERO_LEDS  15

class ModeReactive : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { cycleIndex = 0; fallingDot = 0; dotRising = false; }
    virtual bool step(unsigned long dt) {
      int lights = map(abs(currentX), 0, ONE_G, 0, LEDS - ZERO_LEDS);
      for (uint16_t i=0; i<LEDS; i++) {
        if (i < lights && i != fallingDot/3) {
          strip.setPixelColor(i, 0x0);
        } else {
          strip.setPixelColor(i, wheel(((cycleIndex/2)+i) & 0xFF));
        }
      }
      if (dotRising) {
        if (--fallingDot < 0) {
          fallingDot = 0;
          dotRising = false;
        }
      } else if (++fallingDot >= lights*3) {
        fallingDot--;
        dotRising = true;
      }
      if (++cycleIndex >= 256*2) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }
  private:
    int cycleIndex;
    int fallingDot;
    boolean dotRising;
};

#endif

