#ifndef ModeLight_h
#define ModeLight_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeSimple : public Mode
{
  public:
    explicit ModeSimple(byte b, uint32_t c, int p) : brightness(b), color(c), pixels(p) {}
    virtual String name() { return "Simple"; }
    virtual void init() {}
    virtual bool step(unsigned long dt) {
      FastLED.setBrightness(brightness);
      for (uint16_t i=0; i<NUM_LEDS; i++) {
        leds[i] =  i<pixels ? CRGB::Black : color;
      }
      return true;
    }
  private:
    byte brightness;
    uint32_t color;
    int pixels;
};

#endif

