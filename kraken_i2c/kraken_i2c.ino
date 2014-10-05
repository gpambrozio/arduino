/*****************************************************************************
 ****************************************************************************/
#include "structs.h"
#include <TCL.h>

#include "TinyWireS.h"
#define Wire TinyWireS

#define TCL_CLOCKPIN 3
#define TCL_DATAPIN  4

#define LEDS  25
#define LINES 4
#define SLAVE_ADDRESS  0x07
#define BUS_SYNC_COMMAND 0x55

typedef enum {
  LedCommandInvalid = 0,
  LedCommandRainbow,      // Gets 1 byte: start color (Wheel Index)
  LedCommandLevel,        // Gets 2 bytes: level1 and 2
  LedCommandColor,        // Gets 3 bytes: wheel pos, level1 and level2
  LedCommandBounce,       // Gets 4 bytes: RGB and speed
  LedCommandPulse,        // Gets 5 bytes: RGB, time on, time off
  LedCommandColorRGB,     // Gets 5 bytes: RGB, level1 and level2
  LedCommandCount
} LedCommand;

#define MAX_COMMAND_BUFFER  10

volatile uint8_t receivePosition = 0;
volatile LedCommand command = LedCommandInvalid;
volatile uint8_t receiveBuffer[MAX_COMMAND_BUFFER];
volatile unsigned long pulsePosition = 0;
uint16_t wheelPos;

// callback for received data
void receivedData(uint8_t byteCount) {
  while (Wire.available()) {
    int number = Wire.read();
    if (number == BUS_SYNC_COMMAND) {
      receivePosition = 0;
      command = LedCommandInvalid;
    } else if (receivePosition == 0 && number >= 1 && number < LedCommandCount) {
      command = (LedCommand)number;
      receivePosition++;
      pulsePosition = millis();
    } else if (receivePosition <= MAX_COMMAND_BUFFER) {
      receiveBuffer[receivePosition-1] = number;
      receivePosition++;
    }
  }
}

// callback for sending data
void dataRequested(){
  Wire.write(BUS_SYNC_COMMAND);
}

void setup() {
  TCL.begin(TCL_CLOCKPIN, TCL_DATAPIN);
  TCL.setAll(LEDS*LINES,0,0,0);
  
//  command = LedCommandColorRGB;
//  receiveBuffer[0] = 255;
//  receiveBuffer[1] = 255;
//  receiveBuffer[2] = 255;
//  receiveBuffer[3] = 25;
//  receiveBuffer[4] = 25;
//  receivePosition = 5;
//
  command = LedCommandLevel;
  receiveBuffer[0] = 50;
  receiveBuffer[1] = 50;
  receivePosition = 2;                                                                                        ;

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

 // define callbacks for i2c communication
  Wire.onReceive(receivedData);
  Wire.onRequest(dataRequested);
}

void loop1() {
  TCL.sendEmptyFrame();
  for (int x=0; x<LEDS; x++) {
    TCL.sendColor(200, 0, 0);
  }
  for (int x=0; x<LEDS; x++) {
    TCL.sendColor(0, 0, 200);
  }
  for (int x=0; x<LEDS; x++) {
    TCL.sendColor(200, 0, 0);
  }
  for (int x=0; x<LEDS; x++) {
    TCL.sendColor(0, 0, 200);
  }
  TCL.sendEmptyFrame();
}

void loop() {
  wheelPos++;
  wheelPos %= (256);
  TCL.sendEmptyFrame();
  
  switch(command) {
    case LedCommandRainbow:  // Gets 1 byte: start color (Wheel Index)
      if (receivePosition >= 1) {
        int pos = receiveBuffer[0];
        for (int x=0; x<LEDS * LINES; x++) {
          RGB color = Wheel(pos++);
          TCL.sendColor(color.r, color.g, color.b);
          pos &= 0xff;
        }
      }
      break;
      
    case LedCommandLevel:        // Gets 2 bytes: level1 and 2
      if (receivePosition >= 2) {
        // Color pixels based on rainbow gradient
        RGB color = Wheel(wheelPos);
        
        for (int i=0; i<LINES ;i++) {
          for (int x=0; x<LEDS; x++) {
            if ((i == 0 && x < receiveBuffer[0]) ||
                (i == 1 && (LEDS - x - 1) < receiveBuffer[0]) ||
                (i == 2 && x < receiveBuffer[1]) ||
                (i == 3 && (LEDS - x - 1) < receiveBuffer[1])
                ) {
              TCL.sendColor(color.r, color.g, color.b);
            } else {
              TCL.sendColor(0, 0, 0);
            }
          }
        }
      }
      break;

    case LedCommandColor:        // Gets 3 bytes: wheel pos, level1 and level2
      if (receivePosition >= 3) {
        RGB color = Wheel(receiveBuffer[0]);
        for (int i=0; i<LINES ;i++) {
          for (int x=0; x<LEDS; x++) {
            if ((i == 0 && x < receiveBuffer[1]) ||
                (i == 1 && (LEDS - x - 1) < receiveBuffer[1]) ||
                (i == 2 && x < receiveBuffer[2]) ||
                (i == 3 && (LEDS - x - 1) < receiveBuffer[2])
                ) {
              TCL.sendColor(color.r, color.g, color.b);
            } else {
              TCL.sendColor(0, 0, 0);
            }
          }
        }
      }
      break;
      
    case LedCommandBounce:       // Gets 4 bytes: RGB and speed
      if (receivePosition >= 4) {
      }
      break;
      
    case LedCommandPulse:        // Gets 5 bytes: RGB, time on, time off
      if (receivePosition >= 5) {
        unsigned long span = millis() - pulsePosition;
        span %= (receiveBuffer[3] + receiveBuffer[4]);
        RGB color;
        if (span < receiveBuffer[3])
          color = (RGB){ receiveBuffer[0], receiveBuffer[1], receiveBuffer[2] };
        else
          color = (RGB){ 0, 0, 0 };
        for (int x=0; x<LEDS * LINES; x++) {
          TCL.sendColor(color.r, color.g, color.b);
        }
      }
      break;

    case LedCommandColorRGB:        // Gets 3 bytes: wheel pos, level1 and level2
      if (receivePosition >= 5) {
        RGB color = (RGB){ receiveBuffer[0], receiveBuffer[1], receiveBuffer[2] };
        for (int i=0; i<LINES ;i++) {
          for (int x=0; x<LEDS; x++) {
            if ((i == 0 && x < receiveBuffer[1]) ||
                (i == 1 && (LEDS - x - 1) < receiveBuffer[1]) ||
                (i == 2 && x < receiveBuffer[2]) ||
                (i == 3 && (LEDS - x - 1) < receiveBuffer[2])
                ) {
              TCL.sendColor(color.r, color.g, color.b);
            } else {
              TCL.sendColor(0, 0, 0);
            }
          }
        }
      }
      break;
      
  }

  TCL.sendEmptyFrame();
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition g -r -b - back to g
RGB Wheel(byte WheelPos) {
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

