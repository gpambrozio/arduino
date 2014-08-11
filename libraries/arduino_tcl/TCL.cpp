/*****************************************************************************
 * Tcl.cpp
 *
 * Copyright 2011-2012 Christpher De Vries
 * This program is distributed under the Artistic License 2.0, a copy of which
 * is included in the file LICENSE.txt
 ****************************************************************************/

#include "TCL.h"

TclClass TCL;

uint8_t TclClass::datapinmask, TclClass::clkpinmask;
volatile uint8_t *TclClass::dataport, *TclClass::clkport;
boolean TclClass::isSPI;

void TclClass::begin() {
#ifdef SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  isSPI = true;
#endif
}

void TclClass::begin(int clockPin, int dataPin) {
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  clkport     = portOutputRegister(digitalPinToPort(clockPin));
  clkpinmask  = digitalPinToBitMask(clockPin);
  dataport    = portOutputRegister(digitalPinToPort(dataPin));
  datapinmask = digitalPinToBitMask(dataPin);
  *clkport   &= ~clkpinmask;
  *dataport  &= ~datapinmask;
  isSPI = false;
}

void TclClass::setupDeveloperShield() {
  pinMode(TCL_MOMENTARY1, INPUT);
  pinMode(TCL_MOMENTARY2, INPUT);
  pinMode(TCL_SWITCH1, INPUT);
  pinMode(TCL_SWITCH2, INPUT);
  
  digitalWrite(TCL_MOMENTARY1, HIGH);
  digitalWrite(TCL_MOMENTARY2, HIGH);
  digitalWrite(TCL_SWITCH1, HIGH);
  digitalWrite(TCL_SWITCH2, HIGH);
}

void TclClass::end() {
#ifdef SPI
  SPI.end();
#endif
}

byte TclClass::makeFlag(byte red, byte green, byte blue) {
  byte flag = 0;

  flag = (red&0xC0)>>6;
  flag |= ((green&0xC0)>>4);
  flag |= ((blue&0xC0)>>2);
  return ~flag;
}

void TclClass::dioWrite(byte c) {
  for(byte bit = 0x80; bit; bit >>= 1) {
    if(c & bit) {
      *dataport |=  datapinmask;
    } else {
      *dataport &= ~datapinmask;
    }
    *clkport |=  clkpinmask;
    *clkport &= ~clkpinmask;
  }
}

void TclClass::sendFrame(byte flag, byte red, byte green, byte blue) {
  if (isSPI) {
#ifdef SPI
    SPI.transfer(flag);
    SPI.transfer(blue);
    SPI.transfer(green);
    SPI.transfer(red);
#endif
  } else {
    dioWrite(flag);
    dioWrite(blue);
    dioWrite(green);
    dioWrite(red);
  }
}

void TclClass::sendColor(byte red, byte green, byte blue) {
  byte flag;

  flag = makeFlag(red,green,blue);

  sendFrame(flag,red,green,blue);
}

void TclClass::sendEmptyFrame() {
  sendFrame(0x00,0x00,0x00,0x00);
}

void TclClass::setAll(int num_leds, byte red, byte green, byte blue) {
  sendEmptyFrame();
  for (int i=0; i<num_leds; i++) {
    sendColor(red, green, blue);
  }
  sendEmptyFrame();
}
