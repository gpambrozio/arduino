#ifndef ModeRainbow_h
#define ModeRainbow_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeRainbow : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { cycleIndex = 0; }
    virtual bool step(unsigned long dt) {
      for(uint16_t i=0; i< LEDS; i++) {
        strip.setPixelColor(i, wheel(((i * 256 / LEDS) + cycleIndex) & 255));
      }
      if (++cycleIndex >= 256*5) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }
  private:
    int cycleIndex;
};

#endif

