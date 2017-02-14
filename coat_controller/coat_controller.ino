#include "FastLED.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN_1  2
#define DATA_PIN_2  3
#define DATA_PIN_3  4
#define DATA_PIN_4  5

#define LED_TYPE    WS2812
#define COLOR_ORDER RGB

#define NUM_STRIPS 4
#define NUM_LEDS 49
CRGB leds[NUM_STRIPS][NUM_LEDS];

#define BRIGHTNESS          20
#define FRAMES_PER_SECOND  120

void setup() {
  delay(300); // delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN_1,COLOR_ORDER>(leds[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_2,COLOR_ORDER>(leds[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_3,COLOR_ORDER>(leds[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_4,COLOR_ORDER>(leds[3], NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {
  rainbow, 
  rainbowWithGlitter, 
  confetti, 
  sinelon, 
//  juggle, 
//  bpm, 
  };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 15 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  for (int i=0;i<NUM_STRIPS;i++) {
    fill_rainbow( leds[i], NUM_LEDS, gHue, 7);
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  for (int i=0;i<NUM_STRIPS;i++) {
    if( random8() < chanceOfGlitter) {
      leds[i][ random16(NUM_LEDS) ] += CRGB::White;
    }
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  for (int i=0;i<NUM_STRIPS;i++) {
    fadeToBlackBy( leds[i], NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[i][pos] += CHSV( gHue + random8(64), 200, 255);
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  for (int i=0;i<NUM_STRIPS;i++) {
    fadeToBlackBy( leds[i], NUM_LEDS, 20);
    int pos = beatsin16(13,0,NUM_LEDS);
    leds[i][pos] += CHSV( gHue, 255, 192);
  }
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for (int i=0;i<NUM_STRIPS;i++) {
    for( int j = 0; j < NUM_LEDS; i++) { //9948
      leds[i][j] = ColorFromPalette(palette, gHue+(j*2), beat-gHue+(j*10));
    }
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  for (int i=0;i<NUM_STRIPS;i++) {
    fadeToBlackBy( leds[i], NUM_LEDS, 20);
  }
  byte dothue = 0;
  for (int i=0;i<NUM_STRIPS;i++) {
    for( int i = 0; i < 8; i++) {
      leds[i][beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
}

