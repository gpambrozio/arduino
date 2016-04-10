#ifndef ModeTheaterChaseRainbow_h
#define ModeTheaterChaseRainbow_h

#include "Mode.h"
#include "CaneCommon.h"

#ifdef IS_BEAN
#define SCALE 3
#else
#define SCALE 10
#endif

class ModeTheaterChaseRainbow : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { cycleIndex = 0; }
    virtual bool step(unsigned long dt) {
      int cycleMod = (cycleIndex % (3*SCALE)) / SCALE;
      int j = cycleIndex / (3*SCALE);
      for (int i=0; i < LEDS; i++) {
        strip.setPixelColor(i, i % 3 == cycleMod ? wheel( (i+j) % 255) : 0);    //turn every third pixel on
      }
      if (++cycleIndex >= 256*3*SCALE) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }

  private:
    int cycleIndex;
};

#endif

