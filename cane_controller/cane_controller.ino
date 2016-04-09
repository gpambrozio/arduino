#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>

#include "CaneCommon.h"

#include "Mode.h"
#include "ModeReactive.h"
#include "ModeRainbow.h"
#include "ModeBounce.h"
#include "ModeSimple.h"
#include "ModeTheaterChaseRainbow.h"

#define PIN 8

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
int index = 0;

// readings from the calibration run. Using the calibration sketch from Adafruit
// to find these.
// https://learn.adafruit.com/lsm303-accelerometer-slash-compass-breakout/calibration
// the order of the readings stored in the array:
// acc x, y z

// calibration run lowest readings.
float minReadings[NUMBER_OF_AXIS] = 
  {-11.73, -14.32, -17.02};
// calibration run highest readings.
float maxReadings[NUMBER_OF_AXIS] = 
  {11.18, 12.59, 14.91};
float readings[NUMBER_OF_AXIS];
float readingSums[NUMBER_OF_AXIS];
float sreadings[NUMBER_OF_AXIS][NUMEBER_OF_READINGS];

float currentX;
unsigned long invertedStart = 0;
boolean detectedInversion = false;
boolean detectedLongInversion = false;
boolean resetLongInversion = false;
boolean detectedTap = false;
boolean isUpsideDown = false;
int numberOfTaps = 0;
float lastX = -1;

Mode *modes[] = {
  new ModeReactive(),
  new ModeRainbow(),
  new ModeTheaterChaseRainbow(),
  new ModeBounce(),
  new ModeSimple(70, 0xffffff, LEDS/2),
};

#define NUMBER_OF_MODES  (sizeof(modes) / sizeof(Mode *))
#define MODE_LIGHT_INDEX  (NUMBER_OF_MODES - 1)

byte mode = 0;
byte lastMode = 255;

void setup() {
  Serial.begin(115200);
  Serial.println("Cane controller"); Serial.println("");

  /* Initialise the accelerometer */
  if (!accel.begin()) {
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  sensors_event_t accevent;
  accel.getEvent(&accevent);
  randomSeed(10000 * accevent.acceleration.x);

  strip.begin();
  strip.setBrightness(40);

  ModeBounce bounce = ModeBounce();
  bounce.init();
  for (int i=0;i<2;) {
    strip.show();
    if (bounce.step(10)) i++;
    delay(1);
  }
}

void loop() {
  sensors_event_t accevent;
  accel.getEvent(&accevent);

  readings[X] = accevent.acceleration.x;
  readings[Y] = accevent.acceleration.y;
  readings[Z] = accevent.acceleration.z;
  populateReadings();

  Serial.print(millis());
  for (int i = 0; i < NUMBER_OF_AXIS; i++) {
    Serial.print(','); Serial.print(readings[i]); 
  }
  Serial.println("");

  currentX = normalizedReading(X);
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
      if (++mode >= NUMBER_OF_MODES) mode = 0;
    }
    detectedTap = false;
    numberOfTaps = 0;
  }

  if (detectedInversion) {
    if (isUpsideDown) {
      if (++mode >= NUMBER_OF_MODES) mode = 0;
    }
    detectedInversion = false;
  }

  if (detectedLongInversion) {
    if (isUpsideDown) {
      mode = MODE_LIGHT_INDEX;
    }
    detectedLongInversion = false;
  }

  if (mode != lastMode) {
    modes[mode]->init();
    lastMode = mode;
    strip.setBrightness(40);
  }
  modes[mode]->step(10);
  strip.show();
  lastX = currentX;
}

float normalizedReading(byte axis) {
  return (readingSums[axis]/NUMEBER_OF_READINGS - minReadings[axis]) * (MAX_ACCEL - MIN_ACCEL) / (maxReadings[axis] - minReadings[axis]) + MIN_ACCEL;
}

void populateReadings() {
  for (int i = 0; i < NUMBER_OF_AXIS; i++) {
    readingSums[i] -= sreadings[i][index];
    sreadings[i][index] = readings[i];
    readingSums[i] += readings[i];
  }
  if (++index >= NUMEBER_OF_READINGS) { 
    index = 0; 
  }
}

