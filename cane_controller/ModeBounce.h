#ifndef ModeBounce_h
#define ModeBounce_h

#include "Mode.h"
#include "CaneCommon.h"

#ifdef IS_BEAN
#define SCALE 1
#else
#define SCALE 2
#endif

#define BOUNCE_SIZE  10

class ModeBounce : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() {
      cycleIndex = 0;
    }

    virtual bool step(unsigned long dt) {
      if (cycleIndex % SCALE == 0) {
        int j = cycleIndex / SCALE;
        if (j >= LEDS) j = LEDS * 2 - j - 1;
        for (int16_t i=0; i<LEDS; i++) {
          int distance = abs(j - i);
          int brightness = map(distance, 0, BOUNCE_SIZE, 255, 0);
          if (brightness <= 0.0) {
            strip.setPixelColor(i, 0);
          } else {
            strip.setPixelColor(i, wheel(i*3, brightness));
          }
        }
      }
      if (++cycleIndex >= LEDS*2*SCALE) {
        cycleIndex = 0;
        return true;
      }
      return false;

    }
  private:
    int cycleIndex;
};

#endif

