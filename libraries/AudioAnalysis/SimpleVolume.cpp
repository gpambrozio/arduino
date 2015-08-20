#import "SimpleVolume.h"

#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal

// #define DEBUG(n)    {Serial.print(n);Serial.print(' ');}
// #define DEBUGEND()  Serial.println(' ');

#define DEBUG(n)
#define DEBUGEND()

SimpleVolume::SimpleVolume(uint8_t volumePin, uint8_t samples, int maxSignal, int center) :
volumePin(volumePin), maxSignal(maxSignal), samples(samples), center(center) {
    if ((vol = (int *)malloc(samples * sizeof(int)))) {
        memset(vol, 0, samples * sizeof(int));
    }
    lvl = 10;

    volCount = 
    minLvlAvg = 0;
    maxLvlAvg = center;
}

SimpleVolume::~SimpleVolume() {
    if (vol) free(vol);
}

void SimpleVolume::start() {
    pinMode(volumePin, INPUT);
}

// Code here was based on https://learn.adafruit.com/led-ampli-tie/the-code
int SimpleVolume::getVolume() {
    int n = analogRead(volumePin);
    DEBUG(n);
    n = abs(n - center - DC_OFFSET);         // Center on zero
    DEBUG(n);
    n = (n <= NOISE) ? 0 : (n - NOISE);      // Remove noise/hum
    DEBUG(n);
    lvl = ((lvl * 3) + n) >> 2;              // "Dampened" reading (else looks twitchy)
    DEBUG(lvl);

    vol[volCount] = n;                       // Save sample for dynamic leveling
    if (++volCount >= samples) volCount = 0; // Advance/rollover sample counter
 
    // Calculate bar height based on dynamic min/max levels (fixed point):
    n = maxSignal * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

    if (n < 0L)             n = 0;      // Clip output
    else if (n > maxSignal) n = maxSignal;
    DEBUG(n);
    DEBUGEND();

    // Get volume range of prior frames
    minLvl = maxLvl = vol[0];

    for (uint8_t i=1; i<samples; i++) {
        if (vol[i] < minLvl)      minLvl = vol[i];
        else if (vol[i] > maxLvl) maxLvl = vol[i];
    }

    // minLvl and maxLvl indicate the volume range over prior frames, used
    // for vertically scaling the output graph (so it looks interesting
    // regardless of volume level).  If they're too close together though
    // (e.g. at very low volume levels) the graph becomes super coarse
    // and 'jumpy'...so keep some minimum distance between them (this
    // also lets the graph go to zero when no sound is playing):
    if ((maxLvl - minLvl) < maxSignal) maxLvl = minLvl + maxSignal;
    minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
    maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
        
    return n;
}