/* 
*/

#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define OFF_DELAY        1    // in ms

#define DEBOUNCE_TIME    20   // in ms

#define CURTAIN4

#ifdef CURTAIN4

#define RADIO_CURTAIN     RADIO_CURTAIN1
#define MOTORS            4
#define RADIO_CE          8
#define RADIO_CSN         10

static int ROTATION_SENSORS[MOTORS]   = {A0, A1, A2, A3};
static int MOTOR_PWMS[MOTORS]         = { 9,  5,  6,  3};
static int MOTOR_DIRECTIONS[MOTORS*2] = {A4, A4, A5, A5,  4,  7,  2,  2};
static byte FULL_MOTION[MOTORS]       = {15, 18, 18, 15};
static byte INVERT[MOTORS]            = { 1,  0,  0,  0};

#else

#define RADIO_CURTAIN     RADIO_CURTAIN2
#define MOTORS            2
#define RADIO_CE          8
#define RADIO_CSN         9

static int ROTATION_SENSORS[MOTORS]   = {A0, A1};
static int MOTOR_PWMS[MOTORS]         = { 6,  3};
static int MOTOR_DIRECTIONS[MOTORS*2] = { 7, A5,  4,  5};
static byte FULL_MOTION[MOTORS]       = {18, 18};
static byte INVERT[MOTORS]            = { 0,  0};

#endif

#define DIRECTION_DOWN  1
#define DIRECTION_UP    -1

#define SIGNAL_UP       LOW
#define SIGNAL_DOWN     HIGH

#define EEPROM_POSITION 0

RF24 radio(RADIO_CE, RADIO_CSN);

int positions[MOTORS];
int targetPositions[MOTORS];
byte directionMask = 0xFF;

volatile byte pinStates = 0;
byte currentPinState = 0;
byte finalPinState = 0;
byte lastPinState = 0;
unsigned long debounceTimes[MOTORS];

int inByte = 0;         // incoming serial byte
unsigned long mirfData;

void pciSetup(byte pin);

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void setup() {
  for (int i=0;i<MOTORS;i++) {
    pinMode(ROTATION_SENSORS[i], INPUT_PULLUP);
    pciSetup(ROTATION_SENSORS[i]);
    digitalWrite(MOTOR_PWMS[i], LOW);
    digitalWrite(MOTOR_DIRECTIONS[i*2+0], LOW);
    digitalWrite(MOTOR_DIRECTIONS[i*2+1], LOW);
    pinMode(MOTOR_PWMS[i], OUTPUT);
    pinMode(MOTOR_DIRECTIONS[i*2+0], OUTPUT);
    pinMode(MOTOR_DIRECTIONS[i*2+1], OUTPUT);
  }
  for (int i=0;i<MOTORS;i++) {
    positions[i] = targetPositions[i] = min(FULL_MOTION[i], max(0, EEPROM.read(EEPROM_POSITION+i)));
  }
  Serial.begin(9600);           // set up Serial library at 9600 bps
  printf_begin();
  Serial.println("Starting");
  
  START_RADIO(radio, RADIO_CURTAIN);
  radio.printDetails();
  for (int i=0;i<MOTORS;i++) {
    printf_P(PSTR("Position of %d is %d\n"), i, positions[i]);
  }
}

void setPosition(int motor, int pos) {
  if (pos >= 0 && pos <= FULL_MOTION[motor]) {
    positions[motor] = pos;
    EEPROM.write(EEPROM_POSITION+motor, pos);
    printf_P(PSTR("Setposition of %d to %d\n"), motor, pos);
  }
}

void moveTo(byte motor, int finalPosition) {
  finalPosition = min(FULL_MOTION[motor], max(0, finalPosition));
  if (finalPosition != positions[motor]) {
    int direction = (finalPosition > positions[motor]) ? DIRECTION_DOWN : DIRECTION_UP;
    targetPositions[motor] = finalPosition;
    printf_P(PSTR("Motor %d will move %c from %d to %d\n"), motor, (direction == DIRECTION_DOWN) ? 'D' : 'U', positions[motor], targetPositions[motor]);
    
    int signal = ((direction == DIRECTION_DOWN && INVERT[motor] == 0) || 
                  (direction == DIRECTION_UP && INVERT[motor] == 1)) ? SIGNAL_DOWN : SIGNAL_UP;
    digitalWrite(MOTOR_DIRECTIONS[motor*2+0], signal);
    if (MOTOR_DIRECTIONS[motor*2+0] != MOTOR_DIRECTIONS[motor*2+1]) {
      digitalWrite(MOTOR_DIRECTIONS[motor*2+1], (signal == SIGNAL_DOWN) ? SIGNAL_UP : SIGNAL_DOWN);
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

void move(byte motor, int direction, int turns) {
  int finalPosition = positions[motor] + turns * direction;
  moveTo(motor, finalPosition);
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
      {
        move(Serial.parseInt(), Serial.parseInt(), Serial.parseInt());
        break;
      }
      case 'M':
      {
        moveTo(Serial.parseInt(), Serial.parseInt());
        break;
      }
      case 'p':
      {
        int motor = Serial.parseInt();
        setPosition(motor, Serial.parseInt());
        targetPositions[motor] = positions[motor];
        break;
      }
    }
  }

  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    printf_P(PSTR("Received radio %08x command %c\n"), mirfData, inByte);
    switch (inByte) {
      case 'm':
      {
        byte motor = (mirfData >>  8) & 0xFF;
        int direction = ((mirfData >> 16) & 0xFF) ? DIRECTION_DOWN : DIRECTION_UP;
        int turns = (mirfData >> 24) & 0xFF;
        move(motor, direction, turns);
        break;
      }
      case 'M':
      {
        byte motor = (mirfData >>  8) & 0xFF;
        int position = (mirfData >> 16) & 0xFF;
        moveTo(motor, position);
        break;
      }
      case 'p':
      {
        byte motor = (mirfData >>  8) & 0xFF;
        int position = (mirfData >> 16) & 0xFF;
        setPosition(motor, position);
        targetPositions[motor] = positions[motor];
        break;
      }
      case 'c':
      {
        for (int i=0;i<MOTORS;i++) {
          moveTo(i, FULL_MOTION[i]);
        }
        break;
      }
      case 'o':
      {
        for (int i=0;i<MOTORS;i++) {
          moveTo(i, 0);
        }
        break;
      }
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

