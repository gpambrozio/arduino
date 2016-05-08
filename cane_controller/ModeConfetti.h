#ifndef ModeConfetti_h
#define ModeConfetti_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeConfetti : public Mode
{
  public:
    using Mode::Mode;
    virtual String name() { return "Confetti"; }
    virtual void init() { cycleIndex = 0; fill_solid(leds, NUM_LEDS, CRGB::Black); }
    virtual bool step(unsigned long dt) {
      // random colored speckles that blink in and fade smoothly
      fadeToBlackBy(leds, NUM_LEDS, 20);
      int pos = random16(NUM_LEDS);
      leds[pos] += CHSV(cycleIndex + random8(64), 200, 255);
      cycleIndex++;
      return true;
    }
  private:
    uint8_t cycleIndex;
};

#endif

