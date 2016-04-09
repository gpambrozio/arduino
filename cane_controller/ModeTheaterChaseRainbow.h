#ifndef ModeTheaterChaseRainbow_h
#define ModeTheaterChaseRainbow_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeTheaterChaseRainbow : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { cycleIndex = 0; }
    virtual bool step(unsigned long dt) {
      int cycleMod = (cycleIndex % 30) / 10;
      int j = cycleIndex / 30;
      for (int i=0; i < LEDS; i++) {
        strip.setPixelColor(i, i % 3 == cycleMod ? wheel( (i+j) % 255) : 0);    //turn every third pixel on
      }
      if (++cycleIndex >= 256*30) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }

  private:
    int cycleIndex;
};

#endif

