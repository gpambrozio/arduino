#ifndef ModeSound_h
#define ModeSound_h

#include "Mode.h"
#include "CaneCommon.h"

// Microphone connects to Analog Pin 0.  Corresponding ADC channel number
// varies among boards...it's ADC0 on Uno and Mega, ADC7 on Leonardo.
// Other boards may require different settings; refer to datasheet.
#define ADC_CHANNEL 4

#define HISTORY_COUNT  16
#define MIN_LIGHTS     3

uint64_t      captureSum;
volatile byte samplePos = 0;     // Buffer position counter

class ModeSound : public Mode
{
  public:
    using Mode::Mode;
    virtual String name() { return "Audio"; }
    virtual void init() { 
      cycleIndex = 0;
      captureSum = 0;
      levelHistoryPos = 0;
      samplePos = 0;
      rollingLevel = 0;
      rollingMax = 512;
      memset(&levelHistory, 0, sizeof(levelHistory));
      
      // Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion 
      ADMUX  = _BV(REFS0) | ADC_CHANNEL; // Channel sel, right-adj, use AVcc pin
      ADCSRA = _BV(ADEN)  | // ADC enable
               _BV(ADSC)  | // ADC start
               _BV(ADATE) | // Auto trigger
               _BV(ADIE)  | // Interrupt enable
               _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
      ADCSRB = 0;                // Free run mode, no high MUX bit
      DIDR0  = 1 << ADC_CHANNEL; // Turn off digital input for ADC pin

      sei(); // Enable interrupts
    }
    virtual bool step(unsigned long dt) {
      while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish

      int thisCapture = captureSum >> 7;   // divide by 128
      captureSum = 0;
      samplePos = 0;                   // Reset sample counter
      ADCSRA |= _BV(ADIE);             // Resume sampling interrupt

      levelHistory[levelHistoryPos] = thisCapture;
      if (++levelHistoryPos >= HISTORY_COUNT) {
        levelHistoryPos = 0;
      }
      int maxLevel = 128;   // For when it' too silent.
      for (uint16_t i=0; i<HISTORY_COUNT; i++) {
        if (levelHistory[i] > maxLevel) {
          maxLevel = levelHistory[i];
        }
      }
      rollingMax = ((rollingMax * 3) + maxLevel) >> 2;
      rollingLevel = (rollingLevel + thisCapture) >> 1;

      float lights = (float)rollingLevel * (float)(NUM_LEDS - MIN_LIGHTS) / (float)rollingMax + MIN_LIGHTS;
      for (uint16_t i=0; i<NUM_LEDS; i++) {
        if (i < lights) {
          leds[NUM_LEDS-i-1] = wheel((cycleIndex+i) & 0xFF);
        } else if (i+1 < lights) {
          leds[NUM_LEDS-i-1] = wheel((cycleIndex+i) & 0xFF, (lights - i) * 255);
        } else {
          leds[NUM_LEDS-i-1] = CRGB::Black;
        }
      }
      if (++cycleIndex >= 256) {
        cycleIndex = 0;
        return true;
      }
      return false;
    }
  private:
    uint16_t cycleIndex;
    uint16_t rollingMax, rollingLevel;
    byte levelHistoryPos;
    int levelHistory[HISTORY_COUNT];
};

ISR(ADC_vect) { // Audio-sampling interrupt
  int16_t sample = ADC;
  captureSum += abs(sample - 512);
  if(++samplePos >= 128) ADCSRA &= ~_BV(ADIE); // Buffer full, interrupt off
}

#endif

