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

uint8_t receivePosition = 0;
int receivedInput[NUMBER_OF_LIGHTS/8];

// callback for received data
void receivedData(int byteCount) {
  while (Wire.available()) {
    int number = Wire.read();
    if (number == BUS_SYNC_COMMAND) {
      receivePosition = 0;
    } else if (receivePosition < (NUMBER_OF_LIGHTS / 8)) {
      receivedInput[receivePosition++] = number;
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

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

 // define callbacks for i2c communication
  Wire.onReceive(receivedData);
  Wire.onRequest(dataRequested);
}

void loop() {
  for(int i = 0; i < NUMBER_OF_LIGHTS / 8; i++) {
    int number = receivedInput[i];
    for(int x = 0; x < 8; x++) {
      digitalWrite(FIRST_LIGHT + (i * 8) + x, (number & 1) == 1 ? LOW : HIGH);
      number = number >> 1;
    }
  }
  delay(10);
}

