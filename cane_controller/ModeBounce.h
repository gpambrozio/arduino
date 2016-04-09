#ifndef ModeBounce_h
#define ModeBounce_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeBounce : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() {
      cycleIndex = 0;
      for (int i=0;i<LEDS;i++) {
        if (i < INITIAL_LEDS) {
          strip.setPixelColor(i, wheel(i*3));
        } else {
          strip.setPixelColor(i, 0);
        }
      }
    }

    virtual bool step(unsigned long dt) {
      if (cycleIndex % 2 == 0) {
        int j = cycleIndex / 2;
        strip.setPixelColor(j<(LEDS-INITIAL_LEDS) ? j : (LEDS*2-INITIAL_LEDS-j-1), 0);
        strip.setPixelColor(j<(LEDS-INITIAL_LEDS) ? (j+INITIAL_LEDS) : ((LEDS-INITIAL_LEDS)*2-j-1), wheel(j*3));
      }
      if (++cycleIndex >= (LEDS-INITIAL_LEDS)*4) {
        cycleIndex = 0;
        return true;
      }
      return false;

    }
  private:
    int cycleIndex;
};

#endif

