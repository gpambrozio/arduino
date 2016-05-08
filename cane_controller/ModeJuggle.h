#ifndef ModeJuggle_h
#define ModeJuggle_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeJuggle : public Mode
{
  public:
    using Mode::Mode;
    virtual String name() { return "Juggle"; }
    virtual void init() { cycleIndex = 0; fill_solid(leds, NUM_LEDS, CRGB::Black); }
    virtual bool step(unsigned long dt) {
      // 2 colored dots, weaving in and out of sync with each other
      fadeToBlackBy(leds, NUM_LEDS, 20);
      byte dothue = 0;
      for( int i = 0; i < 2; i++) {
        leds[beatsin16(i*2+7, 0, NUM_LEDS)] |= CHSV(dothue + cycleIndex, 200, 255);
        dothue += 32;
      }
      if (++cycleIndex >= 255) cycleIndex = 0;
      return false;
    }
  private:
    int cycleIndex;
};

#endif

