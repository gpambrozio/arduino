#ifndef ModeLight_h
#define ModeLight_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeSimple : public Mode
{
  public:
    explicit ModeSimple(byte b, uint32_t c, int p) : brightness(b), color(c), pixels(p) {};
    virtual void init() {}
    virtual bool step(unsigned long dt) {
      strip.setBrightness(brightness);
      for (uint16_t i=0; i<LEDS; i++) {
        strip.setPixelColor(i, i<pixels ? 0 : color);
      }
      return true;
    }
  private:
    byte brightness;
    uint32_t color;
    int pixels;
};

#endif

