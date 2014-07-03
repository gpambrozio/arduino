/*
 */

#include <avr/pgmspace.h>
#include <SoftwareSerial.h> //this is a must
#include <Wire.h>

#define SLAVE_ADDRESS 0x04

#define FIRST_LIGHT   0
#define NUMBER_OF_LIGHTS 8

void setup() {
  for(int x=0; x<NUMBER_OF_LIGHTS; x++) {
    pinMode(FIRST_LIGHT+x, OUTPUT);
  }
  
  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

 // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);
}

void loop() {
  delay(100);
}

int lightPosition = 20;

// callback for received data
void receiveData(int byteCount){
    while(Wire.available()) {
        int number = Wire.read();
        if (number == 0x55) {
          lightPosition = 0;
        } else if (lightPosition >= 0 && lightPosition < NUMBER_OF_LIGHTS) {
          digitalWrite(FIRST_LIGHT+lightPosition, number == 1 ? HIGH: LOW);
          lightPosition++;
        }
    }
}

// callback for sending data
void sendData(){
  Wire.write(lightPosition);
}

