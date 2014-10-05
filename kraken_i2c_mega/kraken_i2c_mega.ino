/*****************************************************************************
 ****************************************************************************/

#define SLAVE_ADDRESS  0x08
#define BUS_SYNC_COMMAND 0x55

#define EXTRA_GND_1     38
#define EXTRA_GND_2     39

#define FIRST_LIGHT   22
#define NUMBER_OF_LIGHTS 16

#include "Wire.h"
#include "SPI.h"

typedef enum {
  LedCommandInvalid = 0,
  LedCommandLevel,        // Gets 1 byte: level
  LedCommandPulse,        // Gets 2 bytes: time on, time off
  LedCommandCount
} LedCommand;

#define MAX_COMMAND_BUFFER  10

volatile uint8_t receivePosition = 0;
volatile LedCommand command = LedCommandInvalid;
volatile uint8_t receiveBuffer[MAX_COMMAND_BUFFER];
volatile unsigned long pulsePosition = 0;

// callback for received data
void receivedData(int byteCount) {
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
  Serial.begin(9600);
  Serial.println("Starting");
  
  digitalWrite(EXTRA_GND_1, LOW);
  pinMode(EXTRA_GND_1, OUTPUT);
  digitalWrite(EXTRA_GND_2, LOW);
  pinMode(EXTRA_GND_2, OUTPUT);

  for(int x=0; x<NUMBER_OF_LIGHTS; x++) {
    digitalWrite(FIRST_LIGHT+x, HIGH);
    pinMode(FIRST_LIGHT+x, OUTPUT);
  }

  command = LedCommandPulse;
  receiveBuffer[0] = 1;
  receiveBuffer[1] = 10;
  receivePosition = 2;

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

 // define callbacks for i2c communication
  Wire.onReceive(receivedData);
  Wire.onRequest(dataRequested);
}

void loop() {
  for(int x=0; x<NUMBER_OF_LIGHTS; x++) {
    digitalWrite(FIRST_LIGHT+x, LOW);
  }
  delay(1000);
}

void loop1() {
  switch(command) {
    case LedCommandLevel:  // Gets 1 byte: level
      if (receivePosition >= 1) {
        int level = receiveBuffer[0];
        for(int i = 0; i < NUMBER_OF_LIGHTS; i++) {
          digitalWrite(FIRST_LIGHT + i, i < level ? LOW : HIGH);
        }
      }
      break;
      
    case LedCommandPulse:
      if (receivePosition >= 2) {
        unsigned long span = (millis() - pulsePosition) / 100;
        span %= (receiveBuffer[0] + receiveBuffer[1]);
        int level = HIGH;
        if (span < receiveBuffer[0])
          level = LOW;
        for(int i = 0; i < NUMBER_OF_LIGHTS; i++) {
          digitalWrite(FIRST_LIGHT + i, level);
        }
      }
      break;
  }
  delay(10);
}

