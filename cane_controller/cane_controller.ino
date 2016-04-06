#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>

#define PIN 8

#define LEDS  66
#define INITIAL_LEDS 10

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

/* Assign a unique ID to these sensors */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

/*
 * variables for smoothing of the readings.
 */
const int numberOfAxis = 3;
const int numberOfReadings = 8; // number of readings to run average from.
int index = 0;

// readings from the calibration run. Using the calibration sketch from Adafruit
// to find these.
// https://learn.adafruit.com/lsm303-accelerometer-slash-compass-breakout/calibration
// the order of the readings stored in the array:
// acc x, y z

// calibration run lowest readings.
float minReadings[numberOfAxis] = 
  {-11.73, -14.32, -17.02};
// calibration run highest readings.
float maxReadings[numberOfAxis] = 
  {11.18, 12.59, 14.91};
float readings[numberOfAxis];
float readingSums[numberOfAxis];
float sreadings[numberOfAxis][numberOfReadings];

void setup() {
  Serial.begin(115200);
  Serial.println("Cane controller"); Serial.println("");

  /* Initialise the accelerometer */
  if (!accel.begin()) {
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }

  strip.begin();
  strip.setBrightness(40);
  strip.show(); // Initialize all pixels to 'off'

  bounceSetup();
  for (int i=0;i<2;) {
    strip.show();
    if (bounce()) i++;
    delay(1);
  }
}

#define X  0
#define Y  1
#define Z  1

#define MIN_ACCEL   -100
#define MAX_ACCEL    100

unsigned long invertedStart = 0;
boolean detectedInversion = false;
boolean detectedLongInversion = false;
boolean resetLongInversion = false;
boolean detectedTap = false;
boolean isUpsideDown = false;
int numberOfTaps = 0;
float lastX = -1;

typedef enum : byte {
//  ModeDifferential,
  ModeReactive,
  ModeRainbow,
  ModeTheaterChaseRainbow,
  ModeBounce,
  ModeLight,
  ModeOff,
  ModeLast
} Mode;

byte mode = 0;
byte lastMode = ModeLast;
int cycleIndex = 0;

void loop() {
  sensors_event_t accevent;
  accel.getEvent(&accevent);

  readings[X] = accevent.acceleration.x;
  readings[Y] = accevent.acceleration.y;
  readings[Z] = accevent.acceleration.z;
  populateReadings();

  Serial.print(millis());
  for (int i = 0; i < numberOfAxis; i++) {
    Serial.print(','); Serial.print(readings[i]); 
  }
  Serial.println("");

  float currentX = normalizedReading(X);
  unsigned long invertedTime = millis() - invertedStart;
  if ((currentX > 0.0 && lastX < 0.0) ||
      (currentX < 0.0 && lastX > 0.0)) {
    Serial.print("Inverted: "); Serial.println(invertedTime);
    if (invertedTime > 2 && invertedTime < 100 && currentX < 0.0) {
      numberOfTaps++;
    }
    invertedStart = millis();
    resetLongInversion = false;
  } else if (invertedStart > 0) {
    if (invertedTime > 500 && currentX < 0.0 && numberOfTaps > 0) {
      detectedTap  = true;
    } else if (invertedTime > 2000 && !resetLongInversion) {
      detectedLongInversion = true;
      resetLongInversion = true;
    } else if (invertedTime > 1000) {
      if (isUpsideDown && currentX < 0) {
        isUpsideDown = false;
        detectedInversion = true;
      } else if (!isUpsideDown && currentX > 0) {
        isUpsideDown = true;
        detectedInversion = true;
      }
    }
  }

  if (detectedTap) {
    Serial.print("Taps detected: "); Serial.println(numberOfTaps);
    if (numberOfTaps >= 3) {
      if (++mode >= ModeLast) mode = 0;
    }
    detectedTap = false;
    numberOfTaps = 0;
  }

  if (detectedInversion) {
    if (isUpsideDown) {
      if (++mode >= ModeLast) mode = 0;
    }
    detectedInversion = false;
  }

  if (detectedLongInversion) {
    if (isUpsideDown) {
      mode = ModeLight;
    }
    detectedLongInversion = false;
  }

  if (mode != lastMode) {
    cycleIndex = 0;
    lastMode = mode;
    strip.setBrightness(40);
    switch (mode) {
      case ModeBounce:
        bounceSetup();
        break;
    }
  }

  switch (mode) {
//    case ModeDifferential:
//    {
//      float difference = lastX - currentX;
//      int lights = constrain(map(abs(difference), 0, 20, 0, LEDS), 0, LEDS);
//      Serial.print("Diff = "); Serial.print(difference); Serial.print("  l = "); Serial.println(lights);
//      for (uint16_t i=0; i<LEDS; i++) {
//        if (LEDS - i < lights)
//          strip.setPixelColor(i, 0x0);
//        else
//          strip.setPixelColor(i, Wheel(i*3));
//      }
//      break;
//    }
    
    case ModeReactive:
    {
      int lights = map(abs(currentX), 0, MAX_ACCEL, 0, LEDS);
      for (uint16_t i=0; i<LEDS; i++) {
        if (i < lights)
          strip.setPixelColor(i, 0x0);
        else
          strip.setPixelColor(i, Wheel(i*3));
      }
      break;
    }

    case ModeBounce:
      bounce();
      break;

    case ModeLight:
      strip.setBrightness(70);
      for (uint16_t i=0; i<LEDS; i++) {
        strip.setPixelColor(i, i<LEDS/2 ? 0 : 0xFFFFFF);
      }
      break;
      
    case ModeOff:
      for (uint16_t i=0; i<LEDS; i++) {
        strip.setPixelColor(i, 0);
      }
      break;

    case ModeRainbow:
      rainbowCycle();
      break;

    case ModeTheaterChaseRainbow:
      theaterChaseRainbow();
      break;
  }
  strip.show();
  lastX = currentX;
}

float normalizedReading(byte axis) {
  return (readingSums[axis]/numberOfReadings - minReadings[axis]) * (MAX_ACCEL - MIN_ACCEL) / (maxReadings[axis] - minReadings[axis]) + MIN_ACCEL;
}

void populateReadings() {
  for (int i = 0; i < numberOfAxis; i++) {
    readingSums[i] -= sreadings[i][index];
    sreadings[i][index] = readings[i];
    readingSums[i] += readings[i];
  }
  if (++index >= numberOfReadings) { 
    index = 0; 
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
  uint16_t i;

  for(i=0; i< LEDS; i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / LEDS) + cycleIndex) & 255));
  }
  if (++cycleIndex >= 256*5) cycleIndex = 0;
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow() {
  int cycleMod = (cycleIndex % 30) / 10;
  int j = cycleIndex / 30;
  for (int i=0; i < LEDS; i++) {
    strip.setPixelColor(i, i % 3 == cycleMod ? Wheel( (i+j) % 255) : 0);    //turn every third pixel on
  }
  if (++cycleIndex >= 256*30) cycleIndex = 0;
}

void bounceSetup() {
  for (int i=0;i<LEDS;i++) {
    if (i < INITIAL_LEDS) {
      strip.setPixelColor(i, Wheel(i*3));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
}

bool bounce() {
  if (cycleIndex % 3 == 0) {
    int j = cycleIndex / 3;
    strip.setPixelColor(j<(LEDS-INITIAL_LEDS) ? j : (LEDS*2-INITIAL_LEDS-j-1), 0);
    strip.setPixelColor(j<(LEDS-INITIAL_LEDS) ? (j+INITIAL_LEDS) : ((LEDS-INITIAL_LEDS)*2-j-1), Wheel(j*3));
  }
  if (++cycleIndex >= (LEDS-INITIAL_LEDS)*6) {
    cycleIndex = 0;
    return true;
  }
  return false;
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

