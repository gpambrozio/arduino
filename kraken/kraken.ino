/*****************************************************************************
 ****************************************************************************/

#include <TCL.h>
#import "structs.h"

#define TCL_CLOCKPIN 1
#define TCL_DATAPIN  0

#define MIC_PIN      1    // Analog 1 is pin 2

#define LEDS  50

#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   12  // Length of buffer for dynamic level adjustment
#define TOP       ((LEDS / 2) + 2) // Allow dot to go slightly off scale



//Input a value 0 to 255 to get a color value.
//The colours are a transition g -r -b - back to g
RGB Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
    return (RGB){ WheelPos * 3, 255 - WheelPos * 3, 0 };
  } 
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return (RGB){ 255 - WheelPos * 3, 0, WheelPos * 3 };
  } 
  else {
    WheelPos -= 170; 
    return (RGB){ 0, WheelPos * 3, 255 - WheelPos * 3 };
  }
}

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 256; j++) {     // 3 cycles of all 256 colors in the wheel
    TCL.sendEmptyFrame();
    for (i=0; i < LEDS; i++) {
      RGB color = Wheel( (i + j) % 255);
      TCL.sendColor(color.r, color.g, color.b);
    }  
    TCL.sendEmptyFrame();
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  int i, j;
  
  for (j=0; j < 256 * 5; j++) {     // 5 cycles of all 25 colors in the wheel
    TCL.sendEmptyFrame();
    for (i=0; i < LEDS; i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 96 is to make the wheel cycle around
      RGB color = Wheel( ((i * 256 / LEDS) + j) % 256);
      TCL.sendColor(color.r, color.g, color.b);
    }  
    TCL.sendEmptyFrame();
    delay(wait);
  }
}

void setup() {
  TCL.begin(TCL_CLOCKPIN, TCL_DATAPIN);
  TCL.setAll(LEDS,0,0,0);
}

int height;
int lvl = 0;
int vol[SAMPLES];
int volCount;
int minLvlAvg, maxLvlAvg;
int minLvl, maxLvl;
uint16_t wheelPos;

void loop1() {
  rainbowCycle(2);
}

void loop() {
  height   = analogRead(MIC_PIN);                        // Raw reading from mic
  height   = abs(height - 512 - DC_OFFSET); // Center on zero
  height   = (height <= NOISE) ? 0 : (height - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + height) >> 3;    // "Dampened" reading (else looks twitchy)

  vol[volCount] = height;                 // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;

  // Color pixels based on rainbow gradient
  RGB color = Wheel(wheelPos >> 2);
  wheelPos++;
  wheelPos %= (256 << 2);
  TCL.sendEmptyFrame();
  for(int i=0; i<2 ;i++) {
    for(int x=0; x<LEDS/2; x++) {
      if ((i == 0 && x < height) || (i == 1 && ((LEDS/2) - x - 1) < height)) {
        TCL.sendColor(color.r, color.g, color.b);
      } else {
        TCL.sendColor(0, 0, 0);
      }
    }
  }
  TCL.sendEmptyFrame();

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];

  for(int i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl)      minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

