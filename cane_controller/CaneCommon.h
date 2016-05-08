#ifndef CaneCommon_h
#define CaneCommon_h

#include "FastLED.h"

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define NUM_LEDS    66
#define BRIGHTNESS   40

extern CRGB leds[NUM_LEDS];

#define INITIAL_LEDS 10

#define ONE_G 10.0

#define TAP_THRESHOLD  (ONE_G/6)

#define NUMBER_OF_AXIS  3

#define X  0
#define Y  1
#define Z  2

#define NUMBER_OF_READINGS  8 // number of readings to run average from.

extern float currentX;

#endif

