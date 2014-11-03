#include <Adafruit_NeoPixel.h>

#define PIN 3
#define BUTTON A1

typedef enum {
  ModeRainbow = 0,
  ModeRainbowCycle,
  ModeTheaterChaseRainbow,
  ModeBounce,
  ModeJet,
  ModeRandom,
  ModeOff,
  ModeCount
} Mode;

Mode currentMode = ModeRainbow;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  pinMode(A1, INPUT_PULLUP);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

unsigned long buttonStart = 0;
uint16_t counter;

void loop() {
  if (digitalRead(BUTTON) == LOW && millis() > buttonStart + 500) {
    buttonStart = millis();
    currentMode = (Mode)((int)currentMode + 1);
    if (currentMode >= ModeCount) currentMode = (Mode)0;
    counter = 0;
    randomSeed(millis());
  }

  switch(currentMode) {
    case ModeRainbow:
      rainbow(20, counter++);
      if (counter >= 256) counter = 0;
      break;
      
    case ModeRainbowCycle:
      rainbowCycle(20, counter++);
      if (counter >= 256 * 5) counter = 0;
      break;
      
    case ModeTheaterChaseRainbow:
      theaterChaseRainbow(50, counter++);
      if (counter >= 256 * 3) counter = 0;
      break;
      
    case ModeBounce:
      bounceRainbow(50, counter++);
      if (counter >= 256 * strip.numPixels()) counter = 0;
      break;
      
    case ModeJet:
      jetRainbow(50, counter++);
      if (counter >= 128 * strip.numPixels()) counter = 0;
      break;

    case ModeRandom:
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(random(256)));
      }
      strip.show();
      delay(100);
      break;
      
    case ModeOff:
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
      strip.show();
      delay(50);
      break;
  }
}

void rainbow(uint8_t wait, uint16_t counter) {
  uint16_t j = counter;

  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i+j) & 255));
  }
  strip.show();
  delay(wait);
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, uint16_t counter) {
  uint16_t j = counter;

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
  delay(wait);
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait, uint16_t counter) {
  uint16_t q = counter % 3;
  uint16_t j = counter / 3;
  for (int i=0; i < strip.numPixels(); i=i+3) {
    strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
  }
  strip.show();
 
  delay(wait);
 
  for (int i=0; i < strip.numPixels(); i=i+3) {
    strip.setPixelColor(i+q, 0);        //turn every third pixel off
  }
}

void bounceRainbow(uint8_t wait, uint16_t counter) {
  uint16_t j = counter / strip.numPixels();
  uint16_t q = counter % strip.numPixels();

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    if (i == q || (strip.numPixels() - i) == q)
      strip.setPixelColor(i, Wheel(((q * 256 / strip.numPixels()) + j) & 255));
    else
      strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(wait);
}

void jetRainbow(uint8_t wait, uint16_t counter) {
  uint16_t j = counter * 2 / strip.numPixels();
  uint16_t q = counter % (strip.numPixels() / 2);

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    if (i == q || (strip.numPixels() - i - 1) == q)
      strip.setPixelColor(i, Wheel(((q * 256 / strip.numPixels()) + j) & 255));
    else
      strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(wait);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

