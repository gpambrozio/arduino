/* 
*/

#include <SPI.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define MOTOR         3
#define MOTOR_MAX     255
#define MOTOR_MIN     42

#define FIRST_BUTTON  5
#define ANY_BUTTON    (FIRST_BUTTON+0)
#define BUTTON_A      (FIRST_BUTTON+1)
#define BUTTON_B      (FIRST_BUTTON+2)
#define BUTTON_C      (FIRST_BUTTON+3)
#define BUTTON_D      (FIRST_BUTTON+4)
#define BUTTONS       5

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
// First = CE. Second = CS
RF24 radio(2, 10);

void setup() {
  digitalWrite(MOTOR, LOW);
  pinMode(MOTOR, OUTPUT);

  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Starting"); 

  START_RADIO(radio, RADIO_MOBILE);
}

int motorPower = MOTOR_MIN;
unsigned long mirfData;
byte inByte;
byte buttonState = 0;
long debounce = 0;

#define SPEED_UP    { if (motorPower == 0) motorPower = MOTOR_MIN; else if (motorPower < MOTOR_MAX) motorPower++; Serial.println(motorPower); }
#define SPEED_DOWN  { if (motorPower > MOTOR_MIN) motorPower--; else motorPower = 0; Serial.println(motorPower); };

void loop() {
  analogWrite(MOTOR, motorPower);
  
  byte newButtonState = 0;
  for (byte i=0; i<BUTTONS;i++) {
    newButtonState <<= 1;
    if (digitalRead(FIRST_BUTTON+i)) {
      newButtonState |= 1;
    }
  }
  
  byte change = newButtonState ^ buttonState; 
  if (change) {
    debounce = millis();
    
    // Button A
    if ((change & B1000) && (newButtonState & B1000)) {
      SPEED_DOWN
    }

    // Button B
    if ((change & B0100) && (newButtonState & B0100)) {
      SPEED_UP
    }
    
    // Button C
    if ((change & B0010) && (newButtonState & B0010)) {
      motorPower = 0;
      Serial.println(motorPower);
    }
    
    // Button D
    if ((change & B0001) && (newButtonState & B0001)) {
      motorPower = MOTOR_MAX;
      Serial.println(motorPower);
    }
    
    buttonState = newButtonState;
  } else if (buttonState & B10000) {
    if (millis() - debounce > 50) {
      buttonState = 0;
    }
  }

  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    switch (inByte) {
      case '+':
        Serial.println("Received +");
        SPEED_UP
        break;
        
      case '-':
        Serial.println("Received -");
        SPEED_DOWN
        break;
        
      default:
        Serial.print("Received data ");
        Serial.println(mirfData);
    }
  }
}

