#ifndef CaneCommon_h
#define CaneCommon_h

#include <Adafruit_NeoPixel.h>

#define LEDS  66
#define INITIAL_LEDS 10

#define ONE_G 10.0

#define TAP_THRESHOLD  (ONE_G/6)

#define NUMBER_OF_AXIS  3

#define X  0
#define Y  1
#define Z  2

#define NUMBER_OF_READINGS  8 // number of readings to run average from.

extern float currentX;
extern Adafruit_NeoPixel strip;

#endif

