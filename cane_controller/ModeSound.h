#ifndef ModeSound_h
#define ModeSound_h

#include "Mode.h"
#include "CaneCommon.h"
#include "SimpleVolume.h"

#define AUDIO_INPUT  A1

class ModeSound : public Mode
{
  public:
    using Mode::Mode;
    virtual void init() { 
      cycleIndex = 0;
      analyzer.start();
    }
    virtual bool step(unsigned long dt) {
      int lights = analyzer.getVolume();
      for (uint16_t i=0; i<NUM_LEDS; i++) {
        if (i < lights)
          leds[NUM_LEDS-i-1] = wheel(((cycleIndex/2)+i) & 0xFF);
        else
          leds[NUM_LEDS-i-1] = CRGB::Black;
      }
      if (++cycleIndex >= 256*2) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }
  private:
    int cycleIndex;
    SimpleVolume analyzer = SimpleVolume(AUDIO_INPUT, 16, NUM_LEDS+NUM_LEDS/5, 500);
};

#endif

