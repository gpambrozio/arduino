// Demo the quad alphanumeric display LED backpack kit
// scrolls through every character, then scrolls Serial
// input onto the display

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define LOOP_SLEEP       300
#define LOOP_LONG_SLEEP  600

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

void setup() {
  Serial.begin(57600);
  
  alpha4.begin(0x70);  // pass in the address

  alpha4.writeDigitRaw(3, 0x0);
  alpha4.writeDigitRaw(0, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  alpha4.writeDigitRaw(0, 0x0);
  alpha4.writeDigitRaw(1, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  alpha4.writeDigitRaw(1, 0x0);
  alpha4.writeDigitRaw(2, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  alpha4.writeDigitRaw(2, 0x0);
  alpha4.writeDigitRaw(3, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
}

bool compareScratch( ScratchData * scratch1, ScratchData * scratch2 ) {
  return (scratch1->length == scratch2->length) && 
    (memcmp(scratch1->data, scratch2->data, scratch1->length) == 0);
}
 
ScratchData lastScratch;
int8_t displayPosition = 0;
uint8_t stringSize = 0;
uint32_t loopNumber = 0;
uint32_t stopDisplay = 0;

void loop() {
  ScratchData thisScratch = Bean.readScratchData(1);
  
  if (!compareScratch( &thisScratch, &lastScratch )) {
    lastScratch = thisScratch;
    displayPosition = -3;
    stringSize = lastScratch.length;
    stopDisplay = loopNumber + (stringSize + 4) * 3;
  }

  if (loopNumber == stopDisplay) {
    alpha4.clear();
    alpha4.writeDisplay();
  } else if (loopNumber < stopDisplay) {
    // scroll down display
    for (uint8_t i=0;i<4;i++) {
      int8_t pos = displayPosition + i;
      if (pos >= 0 && pos < stringSize) {
        alpha4.writeDigitAscii(i, lastScratch.data[pos]);
      } else {
        alpha4.writeDigitAscii(i, ' ');
      }
    }
    
    if (++displayPosition > stringSize) {
      displayPosition = -3;
    }
   
    alpha4.writeDisplay();
  }
  
  Bean.sleep(loopNumber >= stopDisplay ? LOOP_LONG_SLEEP : LOOP_SLEEP);
  ++loopNumber;
}
