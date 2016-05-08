#ifndef ModeBPM_h
#define ModeBPM_h

#include "Mode.h"
#include "CaneCommon.h"

class ModeBPM : public Mode
{
  public:
    explicit ModeBPM(uint8_t bpm) : beatsPerMinute(bpm) {}
    virtual String name() { return "BPM"; }
    virtual void init() { cycleIndex = 0; fill_solid(leds, NUM_LEDS, CRGB::Black); }
    virtual bool step(unsigned long dt) {
      // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
      uint8_t beat = beatsin8(beatsPerMinute, 64, 255);
      for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette(PartyColors_p, cycleIndex+i, beat-cycleIndex+(i*2));
      }
      if (++cycleIndex > 255) cycleIndex = 0;
      return true;
    }
  private:
    int cycleIndex;
    uint8_t beatsPerMinute;
};

#endif

