#ifndef ModeReactive_h
#define ModeReactive_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeReactive : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { cycleIndex = 0; }
    virtual bool step(unsigned long dt) {
      int lights = map(abs(currentX), 0, MAX_ACCEL, 0, LEDS);
      for (uint16_t i=0; i<LEDS; i++) {
        if (i < lights)
          strip.setPixelColor(i, 0x0);
        else
          strip.setPixelColor(i, wheel(((cycleIndex/4)+i) & 0xFF));
      }
      if (++cycleIndex >= 256*4) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }
  private:
    int cycleIndex;
};

#endif

