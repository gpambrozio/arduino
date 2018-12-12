#include "Common.h"

Strip::Strip(BLEUuid bleuuid, int leds, int pin, int maxBrightness) :

  // Parameter 1 = number of pixels in strip
  // Parameter 2 = Arduino pin number (most are valid)
  // Parameter 3 = pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
  //   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
  strip(leds, pin, NEO_GRB + NEO_KHZ800),
  maxBrightness(maxBrightness),
  characteristic(bleuuid, this)
  {}

void Strip::begin() {
  strip.begin();

  DL("Resetting strip");
  strip.setBrightness(maxBrightness);
  strip.show(); // Initialize all pixels to 'off'

  colorWipe(0xFF0000);
  delay(300);
  colorWipe(0x00FF00);
  delay(300);
  colorWipe(0x0000FF);
  delay(300);
  colorWipe(0);
}

void Strip::loop() {
  if (targetBrightness != brightness) {
    brightness += (targetBrightness > brightness) ? 1 : -1;
    strip.setBrightness(brightness);
  }
  switch (charactericticData.mode) {
    case 'C':
      colorWipe(charactericticData.color);
      break;

    case 'R':
      rainbowCycle();
      break;

    case 'T':
      theaterChaseRainbow();
      break;
  }

  if (millis() > nextCycleChange) {
    nextCycleChange += charactericticData.cycleDelay;
    cyclePosition++;
  }
}

void Strip::setupService() {
  characteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ | CHR_PROPS_WRITE);
  characteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  characteristic.setFixedLen(sizeof(CharactericticData));
  characteristic.setCallback();
  characteristic.begin();
  characteristic.write(&charactericticData, sizeof(CharactericticData));
}

void Strip::writeCallback(uint8_t* data, uint16_t len, uint16_t offset) {
  if (len < sizeof(CharactericticData)) {
    D("wrong size "); D(len); D(" should be "); DL(sizeof(CharactericticData));
    return;
  }
  memcpy(&charactericticData, data+offset, sizeof(CharactericticData));
  D("Got data "); D(len); D(" Mode "); DL(charactericticData.mode);
  targetBrightness = charactericticData.targetBrightness * maxBrightness / 100;
  // DON'T do any strip stuff here!
}

// Fill the dots one after the other with a color
void Strip::colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void Strip::rainbowCycle() {
  uint16_t j = cyclePosition;
  j &= 0xFF;

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, wheel((((i << 8) / strip.numPixels()) + j) & 0xFF));
  }
  strip.show();
}

// Theatre-style crawling lights with rainbow effect
void Strip::theaterChaseRainbow() {
  uint16_t j = cyclePosition;
  j %= 256 * 3;
  int q = j % 3;
  j /= 3;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
    strip.setPixelColor(i+q, wheel( (i+j) & 0xFF));    //turn every third pixel on
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Strip::wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if(wheelPos < 85) {
    return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if(wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}
