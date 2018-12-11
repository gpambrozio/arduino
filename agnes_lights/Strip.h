#ifndef Strip_h
#define Strip_h

#include "AddressableCharacteristic.h"

class Strip
{
  public:
    Strip(BLEUuid bleuuid, int leds, int pin, int max_brightness);
    virtual void begin();
    virtual void loop();
    virtual void setupService();
    virtual void writeCallback(uint8_t* data, uint16_t len, uint16_t offset);
  
  protected:
    Adafruit_NeoPixel strip;
    AddressableCharacteristic characteristic;

    struct CharactericticData {
      uint8_t mode = 'C';
      uint16_t target_brightness = 0;
      uint16_t cycleDelay = 1;
      uint32_t color = 0;
    };
    CharactericticData charactericticData;
    uint16_t brightness = 0;
    uint16_t cyclePosition = 0;
    unsigned long next_cycle_change = 0;
    int max_brightness;

    // Fill the dots one after the other with a color
    virtual void colorWipe(uint32_t c);
    
    // Slightly different, this makes the rainbow equally distributed throughout
    virtual void rainbowCycle();
    
    // Theatre-style crawling lights with rainbow effect
    virtual void theaterChaseRainbow();
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    virtual uint32_t Wheel(byte WheelPos);
};

#endif
