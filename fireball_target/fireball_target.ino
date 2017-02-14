#include "FastLED.h"

#define LED_TYPE  P9813   // Cool neon
//#define LED_TYPE  WS2801   // Adafruit

#define TOTAL_LEDS    100
#define NUM_LEDS      24
#define LEDS_TO_SKIP  0

#define DATA_PIN  13
#define CLOCK_PIN 11

CRGB leds[TOTAL_LEDS];

// Create a 24 bit color value from R,G,B
#define color(r, g, b)  CRGB(r, g, b)

#define SWITCH 4

void setup() {
  // put your setup code here, to run once:
  pinMode(SWITCH, INPUT_PULLUP);

  delay(500); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,CLOCK_PIN,RGB>(leds, TOTAL_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);

  Serial.begin(115200);
  Serial.println("Started");

  int i;
  for (i=0; i < TOTAL_LEDS; i++) {
    leds[i] = color(0,0,0);
  }
  FastLED.show();
  colorWipe(10);
  colorWipe(color(0,0,0), 10);
}

void loop() {
  if (digitalRead(SWITCH) == LOW) {
    Serial.println("SWITCH");
    colorWipe(20);
    delay(200);
    colorWipe(color(0,0,0), 20);
    for (int i=0;i<4;i++) {
      colorWipeFast();
      delay(200);
      colorWipeFast(color(0, 0, 0));
      delay(200);
    }
  }
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
CRGB Wheel(byte WheelPos) {
  if (WheelPos < 85) {
   return color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint8_t wait) {
  int i;
  for (i=LEDS_TO_SKIP; i < NUM_LEDS; i++) {
      leds[i] = Wheel(i*255/NUM_LEDS);
      FastLED.show();
      delay(wait);
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(CRGB color, uint8_t wait) {
  int i;
  for (i=NUM_LEDS-1; i >= LEDS_TO_SKIP; i--) {
      leds[i] = color;
      FastLED.show();
      delay(wait);
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipeFast(CRGB color) {
  int i;
  for (i=LEDS_TO_SKIP; i < NUM_LEDS; i++) {
      leds[i] = color;
  }
  FastLED.show();
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipeFast() {
  int i;
  for (i=LEDS_TO_SKIP; i < NUM_LEDS; i++) {
      leds[i] = Wheel(i*255/NUM_LEDS);
  }
  FastLED.show();
}


