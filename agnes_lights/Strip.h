#ifndef Strip_h
#define Strip_h

#include "AddressableCharacteristic.h"

class Strip
{
  public:
    Strip(BLEUuid bleuuid, int leds, int pin, int maxBrightness);
    virtual void begin();
    virtual void loop();
    virtual void setupService();
    virtual void writeCallback(uint8_t* data, uint16_t len, uint16_t offset);
  
  protected:
    Adafruit_NeoPixel strip;
    AddressableCharacteristic characteristic;

    struct CharactericticData {
      char mode = 'C';
      uint8_t targetBrightness = 0;
      uint8_t cycleDelay = 1;
      // Padding. Sizes are always multiple of 4
      uint8_t a = 0x55;
      uint32_t color = 0;
    };
    CharactericticData charactericticData;
    uint16_t brightness = 0;
    uint16_t cyclePosition = 0;
    unsigned long nextCycleChange = 0;
    int maxBrightness;

    // Fill the dots one after the other with a color
    virtual void colorWipe(uint32_t c);
    
    // Slightly different, this makes the rainbow equally distributed throughout
    virtual void rainbowCycle();
    
    // Theatre-style crawling lights with rainbow effect
    virtual void theaterChaseRainbow();
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    virtual uint32_t wheel(byte wheelPos);
};

#endif
