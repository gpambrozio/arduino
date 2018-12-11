#include "Common.h"

Strip::Strip(BLEUuid bleuuid, int leds, int pin, int max_brightness) :

  // Parameter 1 = number of pixels in strip
  // Parameter 2 = Arduino pin number (most are valid)
  // Parameter 3 = pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
  //   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
  strip(leds, pin, NEO_GRB + NEO_KHZ800),
  max_brightness(max_brightness),
  characteristic(bleuuid, this)
  {}

void Strip::begin() {
  strip.begin();

  DL("Resetting strip");
  strip.setBrightness(max_brightness);
  strip.show(); // Initialize all pixels to 'off'

  delay(1000);
  colorWipe(0xFF0000);
  delay(300);
  colorWipe(0x00FF00);
  delay(300);
  colorWipe(0x0000FF);
  delay(300);
  colorWipe(0);
}

void Strip::loop() {
  if (charactericticData.target_brightness != brightness) {
    brightness += (charactericticData.target_brightness > brightness) ? 1 : -1;
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

  if (millis() > next_cycle_change) {
    next_cycle_change += charactericticData.cycleDelay;
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
    D("wrong size "); DL(len);
    return;
  }
  D("Got data "); DL(len);
  memcpy(&charactericticData, data, sizeof(CharactericticData));
  if (charactericticData.mode == 'C') {
    colorWipe(charactericticData.color);
  }
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
    strip.setPixelColor(i, Wheel((((i << 8) / strip.numPixels()) + j) & 0xFF));
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
    strip.setPixelColor(i+q, Wheel( (i+j) & 0xFF));    //turn every third pixel on
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Strip::Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
