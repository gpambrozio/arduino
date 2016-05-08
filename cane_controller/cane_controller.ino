#include <Wire.h>

#ifdef IS_BEAN
#include "Bean.h"
#else
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#endif

#define TEST_TAPS  0

#include "CaneCommon.h"

#include "Mode.h"
#include "ModeReactive.h"
#include "ModeRainbow.h"
#include "ModeBounce.h"
#include "ModeSimple.h"
#include "ModeTheaterChaseRainbow.h"
#include "ModeSound.h"
#include "ModeJuggle.h"
#include "ModeBPM.h"

#ifdef IS_BEAN
#define NEOPIXEL_PIN 4
#else
#define NEOPIXEL_PIN 8
#endif

#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

#ifndef IS_BEAN
/* Assign a unique ID to these sensors */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
#endif

#if TEST_TAPS
#define MAX_TAPS   10

unsigned long tapTimes[MAX_TAPS];
float tapDeltas[MAX_TAPS][2];

#endif

/*
 * variables for smoothing of the readings.
 */
int index = 0;

float readings[NUMBER_OF_AXIS];
float readingSums[NUMBER_OF_AXIS];
float sreadings[NUMBER_OF_AXIS][NUMBER_OF_READINGS];

float currentX;
unsigned long invertedStart = 0;
unsigned long tapStart = 0;
boolean tapUp = false;
boolean detectedInversion = false;
boolean detectedLongInversion = false;
boolean resetLongInversion = false;
boolean detectedTap = false;
boolean isUpsideDown = false;
int numberOfTaps = 0;
float lastX = -1;

Mode *modes[] = {
//  new ModeSound(),
  new ModeReactive(),
  new ModeRainbow(),
  new ModeTheaterChaseRainbow(),
  new ModeJuggle(),
  new ModeBounce(),
  new ModeBPM(100),
  new ModeSimple(128, CRGB::White, NUM_LEDS/2),
};

#define NUMBER_OF_MODES  (sizeof(modes) / sizeof(Mode *))
#define MODE_LIGHT_INDEX  (NUMBER_OF_MODES - 1)

byte mode = 0;
byte lastMode = 255;

void setup() {
  Serial.begin(115200);
  Serial.println("Cane controller"); Serial.println("");

#ifdef IS_BEAN
  randomSeed(Bean.getAccelerationX());
#else
  /* Initialise the accelerometer */
  if (!accel.begin()) {
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  sensors_event_t accevent;
  accel.getEvent(&accevent);
  randomSeed(10000 * accevent.acceleration.x);
#endif

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,NEOPIXEL_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  ModeBounce bounce = ModeBounce();
  bounce.init();
  for (int i=0;i<2;) {
    FastLED.show();
    if (bounce.step(10)) i++;
  }
}

void loop() {
#ifdef IS_BEAN
  AccelerationReading accel = Bean.getAcceleration();
  // The Bean has a different orientation so XYZ don't match
  readings[X] = ONE_G * (float)accel.yAxis / 512.0 * (float)accel.sensitivity;
  readings[Y] = ONE_G * (float)accel.xAxis / 512.0 * (float)accel.sensitivity;
  readings[Z] = ONE_G * (float)accel.zAxis / 512.0 * (float)accel.sensitivity;
#else
  sensors_event_t accevent;
  accel.getEvent(&accevent);
  readings[X] = accevent.acceleration.x;
  readings[Y] = accevent.acceleration.y;
  readings[Z] = accevent.acceleration.z;
#endif

  populateReadings();

//  Serial.print(millis());
//  for (int i = 0; i < NUMBER_OF_AXIS; i++) {
//    Serial.print(','); Serial.print(readings[i]); 
//  }
//  Serial.println("");

  currentX = normalizedReading(X);
  unsigned long invertedTime = millis() - invertedStart;
  if ((currentX > 0.0 && lastX < 0.0) ||
      (currentX < 0.0 && lastX > 0.0)) {
//    Serial.print("Inverted: "); Serial.println(invertedTime);
    invertedStart = millis();
    resetLongInversion = false;
  } else if (invertedStart > 0) {
    if (invertedTime > 2000 && !resetLongInversion) {
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

  float diff = currentX - lastX;
  if (diff > TAP_THRESHOLD && !tapUp) {
    #if TEST_TAPS
      if (numberOfTaps < MAX_TAPS) {
        tapTimes[numberOfTaps] = millis();
        tapDeltas[numberOfTaps][0] = lastX;
        tapDeltas[numberOfTaps][1] = currentX;
      }
    #endif
    numberOfTaps++;
    tapUp = true;
    tapStart = millis();
  } else if (-diff > TAP_THRESHOLD && tapUp) {
    tapUp = false;
  } else if (tapStart > 0 && millis() - tapStart > 500) {
    tapStart = 0;
    detectedTap = true;
  }

  if (detectedTap) {
    #if TEST_TAPS
      if (Bean.getConnectionState()) {
        Serial.print("Taps detected: "); Serial.println(numberOfTaps);
        for (int i=0;i<numberOfTaps;i++) {
          Serial.print(tapTimes[i]); Serial.print(','); Serial.print(tapDeltas[i][0]); Serial.print(','); Serial.print(tapDeltas[i][1]); Serial.print(','); Serial.println(tapDeltas[i][1] - tapDeltas[i][0]);
        }
      }
    #endif
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

  if (Serial.available()) {
    char received[10];
    byte bytesRead = Serial.readBytesUntil('\n', received, 10);
    if (bytesRead >= 2) {
      switch (received[0]) {
        case 'M':
        {
          byte receivedMode = received[1] - '0';
          if (receivedMode < NUMBER_OF_MODES) {
            mode = receivedMode;
          }
          break;
        }

        case 'd':
        {
          Serial.println("Modes");
          for (int i=0; i < NUMBER_OF_MODES; i++) {
            Serial.println(modes[i]->name());
          }
          Serial.println("-");
        }
      }
    }
  }
  
  if (mode != lastMode) {
    modes[mode]->init();
    lastMode = mode;
    FastLED.setBrightness(BRIGHTNESS);
#ifdef IS_BEAN
    Bean.setScratchData(1, &mode, 1);
#endif
    Serial.println(String("Mode:") + mode);
  }
  modes[mode]->step(10);
  FastLED.show();  
  lastX = currentX;
}

float normalizedReading(byte axis) {
  return (readingSums[axis]/NUMBER_OF_READINGS);
}

void populateReadings() {
  for (int i = 0; i < NUMBER_OF_AXIS; i++) {
    readingSums[i] -= sreadings[i][index];
    sreadings[i][index] = readings[i];
    readingSums[i] += readings[i];
  }
  if (++index >= NUMBER_OF_READINGS) { 
    index = 0; 
  }
}

