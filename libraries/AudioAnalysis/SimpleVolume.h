#ifndef SimpleVolume_h
#define SimpleVolume_h

#include "Arduino.h"

class SimpleVolume {
public:
    SimpleVolume(uint8_t volumePin, uint8_t samples, int maxSignal, int center = 512);
    ~SimpleVolume();
    
    int getVolume();
    
private:
    int *vol;

    int maxSignal;
    uint8_t samples;
    uint8_t volumePin;
    uint8_t volCount;    // Frame counter for storing past volume data

    int lvl;          // Current "dampened" audio level
    int minLvlAvg;    // For dynamic adjustment of graph low & high
    int maxLvlAvg;
    int height;
    
    int center;

    uint16_t minLvl;
    uint16_t maxLvl;
};
  
#endif
