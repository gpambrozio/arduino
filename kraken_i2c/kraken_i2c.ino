/*****************************************************************************
 ****************************************************************************/

#if defined(__AVR_ATtiny85__)    // Trinket

#include "TinyWireS.h"
#define Wire TinyWireS

#else

//#include "Wire.h"

#endif

#include <TCL.h>
#include "structs.h"

#define SLAVE_ADDRESS  0x04

#define TCL_CLOCKPIN 3
#define TCL_DATAPIN  4

#define LEDS  50

void setup() {
#ifdef Serial
  Serial.begin(9600);
  Serial.println("Starting");
#endif
  TCL.begin(TCL_CLOCKPIN, TCL_DATAPIN);
  TCL.setAll(LEDS,0,0,0);

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

 // define callbacks for i2c communication
  Wire.onReceive(receivedData);
  Wire.onRequest(dataRequested);
}

int height = 2;
uint16_t wheelPos;

void loop1() {
  rainbowCycle(2);
}

void loop() {
  // Color pixels based on rainbow gradient
  RGB color = Wheel(wheelPos >> 2);
  wheelPos++;
  wheelPos %= (256 << 2);
  TCL.sendEmptyFrame();
  for (int i=0; i<2 ;i++) {
    for (int x=0; x<LEDS/2; x++) {
      if ((i == 0 && x < height) || (i == 1 && ((LEDS/2) - x - 1) < height)) {
        TCL.sendColor(color.r, color.g, color.b);
      } else {
        TCL.sendColor(0, 0, 0);
      }
    }
  }
  TCL.sendEmptyFrame();

}

// callback for received data
#if defined(__AVR_ATtiny85__)    // Trinket
void receivedData(uint8_t byteCount) {
#else
void receivedData(int byteCount) {
#endif
#ifdef Serial
  Serial.print("Received ");
  Serial.println(byteCount);
#endif
  while (Wire.available()) {
    int number = Wire.read();
#ifdef Serial
    Serial.println(number);
#endif
    if (number == 0x55) {
      height = 0;
    } else {
      height = number;
    }
  }
}

// callback for sending data
void dataRequested(){
  Wire.write(height);
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition g -r -b - back to g
RGB Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
    return (RGB){ WheelPos * 3, 255 - WheelPos * 3, 0 };
  } 
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return (RGB){ 255 - WheelPos * 3, 0, WheelPos * 3 };
  } 
  else {
    WheelPos -= 170; 
    return (RGB){ 0, WheelPos * 3, 255 - WheelPos * 3 };
  }
}

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 256; j++) {     // 3 cycles of all 256 colors in the wheel
    TCL.sendEmptyFrame();
    for (i=0; i < LEDS; i++) {
      RGB color = Wheel( (i + j) % 255);
      TCL.sendColor(color.r, color.g, color.b);
    }  
    TCL.sendEmptyFrame();
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  int i, j;
  
  for (j=0; j < 256 * 5; j++) {     // 5 cycles of all 25 colors in the wheel
    TCL.sendEmptyFrame();
    for (i=0; i < LEDS; i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 96 is to make the wheel cycle around
      RGB color = Wheel( ((i * 256 / LEDS) + j) % 256);
      TCL.sendColor(color.r, color.g, color.b);
    }  
    TCL.sendEmptyFrame();
    delay(wait);
  }
}

