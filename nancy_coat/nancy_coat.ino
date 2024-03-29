#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "SimpleVolume.h"

#define LED_PIN     2
#define MIC_PIN     A1  // Microphone is attached to this analog pin
#define GAIN_PIN    A0
#define EXTRA_GND   13

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(38, LED_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// MUSIC STUFF  ---------------------------------------------------------
#define SAMPLES    60   // Length of buffer for dynamic level adjustment
#define TOP        100
#define BAR_LENGTH 4

SimpleVolume sv = SimpleVolume(MIC_PIN, SAMPLES, TOP, 270);

typedef enum {
  modeSound = 0,
  modeSoundBar,
  modeRainbow,
  modeTheaterChaseRainbow,
  modeTotal
} mode;

byte currentMode;

void setup() {
  pinMode(EXTRA_GND, OUTPUT);
  digitalWrite(EXTRA_GND, LOW);
  
  // Floating = 60dB
  // Low = 50dB
  // High = 40dB
  pinMode(GAIN_PIN, INPUT);
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin(9600);
  sv.start();
  
  currentMode = EEPROM.read(0);
  EEPROM.write(0, (currentMode + 1 >= modeTotal) ? 0 : currentMode + 1);
}

uint16_t currentCycle = 0;
uint8_t cycleCount = 0;

void loop() {  
  switch (currentMode) {
    case modeSound:
    {
      int height = sv.getVolume();
      strip.setBrightness(height);
      if (++cycleCount > 10) {
        if (++currentCycle >= 256*5) currentCycle = 0;
        cycleCount = 0;
      }
      rainbowCycleStep(currentCycle);
      break;
    } 
    
    case modeSoundBar:
    {
      strip.setBrightness(TOP);
      int height = sv.getVolume();
      
      int barHeight = map(height, 0, TOP, 0, BAR_LENGTH);
      for (int i=0;i<BAR_LENGTH;i++) {
        uint32_t color = 0;
        if (barHeight >= i) {
          color = Wheel(height);
        }
        strip.setPixelColor(BAR_LENGTH - i - 1, color);
        strip.setPixelColor(strip.numPixels() - BAR_LENGTH + i, color);
      }
      
      int centerBar = (strip.numPixels() + 1) / 2 - BAR_LENGTH;
      barHeight = map(height, 0, TOP, 0, centerBar);
      for (int i=0;i<centerBar;i++) {
        uint32_t color = 0;
        if (barHeight >= i) {
          color = Wheel(height);
        }
        strip.setPixelColor(BAR_LENGTH + i, color);
        strip.setPixelColor(strip.numPixels() - BAR_LENGTH - i, color);
      }
      strip.show();
      break;
    } 
    
    case modeRainbow:
      strip.setBrightness(TOP);
      rainbowCycle(20);
      break;
      
    case modeTheaterChaseRainbow:
      strip.setBrightness(TOP);
      theaterChaseRainbow(50);
      break;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbowCycleStep(uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

