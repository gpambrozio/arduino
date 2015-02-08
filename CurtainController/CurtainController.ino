/* 
*/

#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define FULL_MOTION      7    // in turns
#define OFF_DELAY        1    // in ms

#define DEBOUNCE_TIME    20   // in ms

#define CURTAIN4

#ifdef CURTAIN4

#define RADIO_CURTAIN     RADIO_CURTAIN1
#define MOTORS            4
#define RADIO_CE          8
#define RADIO_CSN         10

static int ROTATION_SENSORS[MOTORS]   = {A0, A1, A2, A3};
static int MOTOR_PWMS[MOTORS]         = { 9,  6,  5,  3};
static int MOTOR_DIRECTIONS[MOTORS*2] = {A4, A5,  4,  4,  7,  7,  2,  2};

#else

#define RADIO_CURTAIN     RADIO_CURTAIN2
#define MOTORS            2
#define RADIO_CE          8
#define RADIO_CSN         10

static int ROTATION_SENSORS[MOTORS]   = {A0, A1};
static int MOTOR_PWMS[MOTORS]         = { 9,  6};
static int MOTOR_DIRECTIONS[MOTORS*2] = {A4, A5,  4,  7};

#endif

#define DIRECTION_DOWN  1
#define DIRECTION_UP    -1

#define SIGNAL_UP       HIGH
#define SIGNAL_DOWN     LOW

#define EEPROM_POSITION 0

int positions[MOTORS];
int targetPositions[MOTORS];
byte directionMask = 0xFF;
RF24 radio(RADIO_CE, RADIO_CSN);

volatile byte pinStates = 0;
byte currentPinState = 0;
byte finalPinState = 0;
byte lastPinState = 0;
unsigned long debounceTimes[MOTORS];

int inByte = 0;         // incoming serial byte
unsigned long mirfData;

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  printf_begin();
  Serial.println("Starting");
  
  for (int i=0;i<MOTORS;i++) {
    pinMode(ROTATION_SENSORS[i], INPUT_PULLUP);
    pciSetup(ROTATION_SENSORS[i]);
    analogWrite(MOTOR_PWMS[i], 0);
    digitalWrite(MOTOR_PWMS[i], LOW);
    digitalWrite(MOTOR_DIRECTIONS[i*2+0], LOW);
    digitalWrite(MOTOR_DIRECTIONS[i*2+1], LOW);
    pinMode(MOTOR_PWMS[i], OUTPUT);
    pinMode(MOTOR_DIRECTIONS[i*2+0], OUTPUT);
    pinMode(MOTOR_DIRECTIONS[i*2+1], OUTPUT);
    positions[i] = targetPositions[i] = min(FULL_MOTION, max(0, EEPROM.read(EEPROM_POSITION+i)));
  }

  START_RADIO(radio, RADIO_CURTAIN);
  radio.printDetails();
  for (int i=0;i<MOTORS;i++) {
    printf_P(PSTR("Position of %d is %d\n"), i, positions[i]);
  }
}

void setPosition(int motor, int pos) {
  if (pos >= 0 && pos <= FULL_MOTION) {
    positions[motor] = pos;
    EEPROM.write(EEPROM_POSITION+motor, pos);
    printf_P(PSTR("Setposition of %d to %d\n"), motor, pos);
  }
}

void move(byte motor, int direction, int turns) {
  int finalPosition = positions[motor] + turns * direction;
  if (finalPosition < 0) {
    finalPosition = 0;
  } else if (finalPosition > FULL_MOTION) {
    finalPosition = FULL_MOTION;
  }
  
  if (finalPosition != positions[motor]) {
    targetPositions[motor] = finalPosition;
    printf_P(PSTR("Motor %d will move %c from %d to %d\n"), motor, (direction == DIRECTION_DOWN) ? 'D' : 'U', positions[motor], targetPositions[motor]);
    
    digitalWrite(MOTOR_DIRECTIONS[motor*2+0], (direction == DIRECTION_DOWN) ? SIGNAL_DOWN : SIGNAL_UP);
    if (MOTOR_DIRECTIONS[motor*2+0] != MOTOR_DIRECTIONS[motor*2+1]) {
      digitalWrite(MOTOR_DIRECTIONS[motor*2+1], (direction == DIRECTION_DOWN) ? SIGNAL_UP : SIGNAL_DOWN);
    }
    digitalWrite(MOTOR_PWMS[motor], HIGH);
    directionMask &= 0xFF ^ (1 << motor);
    if (direction == DIRECTION_DOWN) {
      directionMask |= (1 << motor);
    }
  } else {
    printf_P(PSTR("Motor %d will not move. Position is %d\n"), motor, positions[motor]);
  }
}

void loop() {
  // Debounce inputs
  byte state = pinStates;
  if (state != currentPinState) {
    byte diff = state ^ currentPinState;
    byte currentMask = 1;
    unsigned long lastChange;
    for (int i = 0; i < MOTORS; i++) {
      if (diff & currentMask) {
        lastChange = millis() - debounceTimes[i];
        if (lastChange > DEBOUNCE_TIME) {
          finalPinState &= 0xFF ^ currentMask;
          if (state & currentMask) {
            finalPinState |= currentMask;
          }
        }
        debounceTimes[i] = millis();
      }
      currentMask <<= 1;
    }

    currentPinState = state;
  }
  
  if (finalPinState != lastPinState) {
    byte diff = finalPinState ^ lastPinState;
    byte currentMask = 1;
    for (int i = 0; i < MOTORS; i++) {
      if ((diff & currentMask) && ((finalPinState & currentMask) == 0)) {
        printf_P(PSTR("Press of on %d\n"), i);
        setPosition(i, positions[i] + ((directionMask & currentMask) ? DIRECTION_DOWN : DIRECTION_UP));
        if (targetPositions[i] == positions[i]) {
          digitalWrite(MOTOR_PWMS[i], LOW);
          printf_P(PSTR("Stopping motor %d\n"), i);
        }
      }
      currentMask <<= 1;
    }
    lastPinState = finalPinState;
  }

  if (Serial.available()) {  // Look for char in serial que and process if found
    inByte = Serial.read();
    printf_P(PSTR("Command %c\n"), inByte);
    switch (inByte) {
      case 'm':
        move(Serial.parseInt(), Serial.parseInt(), Serial.parseInt());
        break;
        
      default:
        break;
    }
  }

  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    printf_P(PSTR("Received radio %04x command %c\n"), mirfData, inByte);
    switch (inByte) {
      default:
        break;
    }
  }
}

void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// handle pin change interrupt for A0 to A5 here 
ISR (PCINT1_vect) {
  byte state = 0;
  for (int i=0;i<MOTORS;i++) {
    if (digitalRead(ROTATION_SENSORS[i]) == LOW) {
      state |= (1 << i);
    }
  }
  pinStates = state;
}

