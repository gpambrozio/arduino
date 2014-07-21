#include "Adafruit_WS2801_trinket_onecolor.h"

// Example to control WS2801-based RGB LED Modules in a strand or strip
// Written by Adafruit - MIT license
/*****************************************************************************/

// Constructor for use with arbitrary clock/data pins:
Adafruit_WS2801::Adafruit_WS2801(uint16_t n, uint8_t dpin, uint8_t cpin) {
  begun   = false;
  numLEDs = n;
  updatePins(dpin, cpin);
}

// Release memory (as needed):
Adafruit_WS2801::~Adafruit_WS2801(void) {
}

// Activate hard/soft SPI as appropriate:
void Adafruit_WS2801::begin(void) {
  pinMode(datapin, OUTPUT);
  pinMode(clkpin , OUTPUT);
  begun = true;
}

// Change pin assignments post-constructor, using arbitrary pins:
void Adafruit_WS2801::updatePins(uint8_t dpin, uint8_t cpin) {

  if(begun == true) { // If begin() was previously invoked...
    // If previously using hardware SPI, turn that off:
    // Regardless, now enable output on 'soft' SPI pins:
    pinMode(dpin, OUTPUT);
    pinMode(cpin, OUTPUT);
  } // Otherwise, pins are not set to outputs until begin() is called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!

  datapin     = dpin;
  clkpin      = cpin;
  clkport     = portOutputRegister(digitalPinToPort(cpin));
  clkpinmask  = digitalPinToBitMask(cpin);
  dataport    = portOutputRegister(digitalPinToPort(dpin));
  datapinmask = digitalPinToBitMask(dpin);
}

uint16_t Adafruit_WS2801::numPixels(void) {
  return numLEDs;
}

void Adafruit_WS2801::show(void) {
  uint16_t i, j;
  uint8_t  bit;

  uint8_t pixels[3] = {r,b,g};

  // Write 24 bits per pixel:
  for(j=0; j<numLEDs; j++ ) {
    for(i=0; i<3; i++ ) {
      for(bit=0x80; bit; bit >>= 1) {
        if(pixels[i] & bit) *dataport |=  datapinmask;
        else                *dataport &= ~datapinmask;
        *clkport |=  clkpinmask;
        *clkport &= ~clkpinmask;
      }
    }
}
  delay(1); // Data is latched by holding clock pin low for 1 millisecond
}

// Set pixel color from separate 8-bit R, G, B components:
void Adafruit_WS2801::setPixelColor(uint8_t _r, uint8_t _g, uint8_t _b) {
    r = _r;
    g = _g;
    b = _b;
}

// Set pixel color from 'packed' 32-bit RGB value:
void Adafruit_WS2801::setPixelColor(uint32_t c) {
 r = c >> 16; // Red
 g = c >>  8; // Green
 b = c;         // Blue
}
