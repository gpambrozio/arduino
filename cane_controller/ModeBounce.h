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
    virtual String name() { return "Knight"; }
    virtual void init() {
      cycleIndex = 0;
    }

    virtual bool step(unsigned long dt) {
      if (cycleIndex % SCALE == 0) {
        int j = cycleIndex / SCALE;
        if (j >= NUM_LEDS) j = NUM_LEDS * 2 - j - 1;
        for (int16_t i=0; i<NUM_LEDS; i++) {
          int distance = abs(j - i);
          int brightness = map(distance, 0, BOUNCE_SIZE, 255, 0);
          if (brightness <= 0.0) {
            leds[i] = CRGB::Black;
          } else {
            leds[i] = wheel(i*3, brightness);
          }
        }
      }
      if (++cycleIndex >= NUM_LEDS*2*SCALE) {
        cycleIndex = 0;
        return true;
      }
      return false;

    }
  private:
    int cycleIndex;
};

#endif

