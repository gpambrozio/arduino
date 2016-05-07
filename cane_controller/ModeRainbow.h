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
      for(uint16_t i=0; i< NUM_LEDS; i++) {
        leds[i] = wheel(((i * 256 / NUM_LEDS) + cycleIndex) & 255);
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

