/*
 */
#include <SPI.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define STAY_OUTPUT 11
#define AWAY_OUTPUT 12

int inByte = 0;         // incoming serial byte
unsigned long lastContactTime = 0;
unsigned long mirfData;

RF24 radio(2,3);

void setup()
{
  pinMode(STAY_OUTPUT, INPUT);
  digitalWrite(STAY_OUTPUT, LOW);

  pinMode(AWAY_OUTPUT, INPUT);
  digitalWrite(AWAY_OUTPUT, LOW);

  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //
  // Setup and configure rf radio
  //

  START_RADIO(radio, RADIO_SERVER);

  Serial.println("starting");
  
  lastContactTime = millis();
}

void press(int output)
{
  digitalWrite(output, LOW);
  pinMode(output, OUTPUT);
  delay(1500);
  pinMode(output, INPUT);
}

void sendRadioData(int pipe)
{
  radio.stopListening();
  radio.openWritingPipe(pipes[pipe]);
  radio.write((byte *)&mirfData, sizeof(unsigned long));
  radio.startListening();
}

void loop()
{
  // if we get a valid byte, read analog ins:
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    switch (inByte) {
      case 'A':
        Serial.println("Received Away");
        press(AWAY_OUTPUT);
        break;
      
      case 'S':
        Serial.println("Received Stay");
        press(STAY_OUTPUT);
        break;
        
      case 'O':
        Serial.println("Received Open");
        inByte = Serial.read();
        mirfData = (inByte - '0') * 10;
        inByte = Serial.read();
        mirfData += (inByte - '0');
        mirfData <<= 8;
        mirfData |= 1;
        sendRadioData(RADIO_DRAPE);
        break;
        
      case 'C':
        Serial.println("Received Close");
        inByte = Serial.read();
        mirfData = (inByte - '0') * 10;
        inByte = Serial.read();
        mirfData += (inByte - '0');
        mirfData <<= 8;
        mirfData |= 2;
        sendRadioData(RADIO_DRAPE);
        break;
        
      case 'P':
        Serial.println("Received Panic");
        mirfData = 'P';
        sendRadioData(RADIO_ALARM);
        break;
        
      case 'M':
        Serial.println("Received Mobile");
        inByte = Serial.read();
        mirfData = (inByte - '0') * 100;
        inByte = Serial.read();
        mirfData += (inByte - '0') * 10;
        inByte = Serial.read();
        mirfData += (inByte - '0');
        mirfData <<= 8;
        mirfData |= 'T';
        sendRadioData(RADIO_MOBILE);
        break;
        
      case 'R':
        Serial.println("Received Sprinkler");
        mirfData = 'R';
        inByte = Serial.read();
        mirfData |= inByte << 8;
        inByte = Serial.read();
        mirfData |= ((unsigned long)inByte) << 16;
        sendRadioData(RADIO_SPRINKLER);
        break;
        
      case 'B':
        Serial.println("Received Mobile Bananas");
        mirfData = 'B';
        sendRadioData(RADIO_MOBILE);
        break;
        
      default:
        Serial.print("Received ");
        Serial.println(inByte);
    }
    
    lastContactTime = millis();
  }
  
  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    switch(inByte) {
      case 'A':
        Serial.println("Received Alarm");
        break;

      default:
        Serial.print("Received ");
        Serial.println(mirfData);
    }
  }
}


