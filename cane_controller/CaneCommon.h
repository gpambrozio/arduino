#ifndef CaneCommon_h
#define CaneCommon_h

#include <Adafruit_NeoPixel.h>

#define LEDS  66
#define INITIAL_LEDS 10

#define NUMBER_OF_AXIS  3

#define X  0
#define Y  1
#define Z  2

#define MIN_ACCEL   -100
#define MAX_ACCEL    100

#define NUMEBER_OF_READINGS  8 // number of readings to run average from.

extern float minReadings[NUMBER_OF_AXIS];
extern float maxReadings[NUMBER_OF_AXIS];
extern float readings[NUMBER_OF_AXIS];
extern float readingSums[NUMBER_OF_AXIS];
extern float sreadings[NUMBER_OF_AXIS][NUMEBER_OF_READINGS];
extern float currentX;

extern Adafruit_NeoPixel strip;

#endif

