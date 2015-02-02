/* 
*/

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"

#define FULL_MOTION      7    // in turns
#define OFF_DELAY        1    // in ms

#define BUTTON           A0
#define ROTATION_SENSOR  7

#define DIRECTION_DOWN  BACKWARD
#define DIRECTION_UP    FORWARD

#define EEPROM_POSITION 0

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *myMotor = AFMS.getMotor(1);

int position = 0;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Starting");
  
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(ROTATION_SENSOR, INPUT_PULLUP);

  AFMS.begin();  // create with the default frequency 1.6KHz
  
  myMotor->setSpeed(255);
  position = EEPROM.read(EEPROM_POSITION);
}

void setPosition(int pos) {
  position = pos;
  EEPROM.write(EEPROM_POSITION, position);
}

void move(int direction, int turns) {
  myMotor->run(direction);
  while (turns > 0) {
    while(digitalRead(ROTATION_SENSOR) == HIGH) {
      delay(10);
    }
    while(digitalRead(ROTATION_SENSOR) == LOW) {
      delay(10);
    }
    turns--;
    setPosition(position + ((direction == FORWARD) ? 1 : -1));
  }
  delay(OFF_DELAY);
  myMotor->run(RELEASE);
}

void loop() {
  if (digitalRead(BUTTON) == HIGH) {
    if (position == 0) {
      move(DIRECTION_DOWN, FULL_MOTION);
    } else {
      move(DIRECTION_UP, FULL_MOTION);
    }
  }
}
