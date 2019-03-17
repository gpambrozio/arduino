#ifndef Common_h
#define Common_h

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>
#include "Strip.h"
#include "AddressableCharacteristic.h"

#define DEBUG

#ifdef DEBUG

#define D(d)  Serial.print(d)
#define DL(d) Serial.println(d)
#define DP(args...) Serial.printf(args)
#define MARK  {Serial.print(F("Running line "));Serial.println(__LINE__);}

#else

#define D(d)  {}
#define DL(d) {}
#define DP(...) {}
#define MARK  {}

#endif

void blink();

#endif
