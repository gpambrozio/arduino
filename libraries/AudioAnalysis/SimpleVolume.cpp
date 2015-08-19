#import "SimpleVolume.h"

#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal

SimpleVolume::SimpleVolume(uint8_t volumePin, uint8_t samples, int maxSignal, int center) :
volumePin(volumePin), maxSignal(maxSignal), samples(samples), center(center) {
    if ((vol = (int *)malloc(samples * sizeof(int)))) {
        memset(vol, 0, samples * sizeof(int));
    }
    volCount = 

    lvl = 
    minLvlAvg = 
    height = 0;
    maxLvlAvg = 1;
}

SimpleVolume::~SimpleVolume() {
    if (vol) free(vol);
}

int SimpleVolume::getVolume() {
    height = analogRead(volumePin);                        // Raw reading from mic
    height = abs(height - center - DC_OFFSET); // Center on zero
    height = (height <= NOISE) ? 0 : (height - NOISE);             // Remove noise/hum
    lvl = ((lvl * 7) + height) >> 3;    // "Dampened" reading (else looks twitchy)

    vol[volCount] = height;                 // Save sample for dynamic leveling
    if (++volCount >= samples) volCount = 0; // Advance/rollover sample counter
 
    // Calculate bar height based on dynamic min/max levels (fixed point):
    height = maxSignal * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

    if (height < 0L)       height = 0;      // Clip output
    else if (height > maxSignal) height = maxSignal;

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
        
    return height;
}