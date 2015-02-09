/*
 */
#include <SPI.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define STAY_OUTPUT 11
#define AWAY_OUTPUT 12

int inByte = 0;         // incoming serial byte
int inByte2 = 0;
unsigned long lastContactTime = 0;
unsigned long mirfData;

RF24 radio(2, 3);  // CE, CSN

void setup()
{
  pinMode(STAY_OUTPUT, INPUT);
  digitalWrite(STAY_OUTPUT, LOW);

  pinMode(AWAY_OUTPUT, INPUT);
  digitalWrite(AWAY_OUTPUT, LOW);

  // start serial port at 9600 bps:
  Serial.begin(9600);

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
        
      case 'T':
        Serial.println("Received Sprinkler Time");
        inByte2 = Serial.read();
        inByte = Serial.read();
        mirfData = (unsigned long)(inByte - '0') * 1000;
        inByte = Serial.read();
        mirfData += (unsigned long)(inByte - '0') * 100;
        inByte = Serial.read();
        mirfData += (unsigned long)(inByte - '0') * 10;
        inByte = Serial.read();
        mirfData += (unsigned long)(inByte - '0');
        mirfData <<= 16;
        mirfData |= inByte2 << 8;
        mirfData |= 'T';
        sendRadioData(RADIO_SPRINKLER);
        break;
        
      case 'D':
        Serial.println("Received Sprinkler Disable");
        inByte2 = Serial.read();
        inByte = Serial.read();
        mirfData = (unsigned long)(inByte - '0') * 1000;
        inByte = Serial.read();
        mirfData += (unsigned long)(inByte - '0') * 100;
        inByte = Serial.read();
        mirfData += (unsigned long)(inByte - '0') * 10;
        inByte = Serial.read();
        mirfData += (unsigned long)(inByte - '0');
        mirfData <<= 16;
        mirfData |= inByte2 << 8;
        mirfData |= 'D';
        sendRadioData(RADIO_SPRINKLER);
        break;
        
      case 'B':
        Serial.println("Received Mobile Bananas");
        mirfData = 'B';
        sendRadioData(RADIO_MOBILE);
        break;
        
      case 'L':
        Serial.println("Received Light");
        inByte = Serial.read();
        if (inByte == 's') {
          mirfData = 's';
          sendRadioData(RADIO_LIGHTBALL);
        } else if (inByte == 'l') {
          unsigned long brightness = Serial.parseInt();
          mirfData = (brightness << 8) | inByte;
          sendRadioData(RADIO_LIGHTBALL);
        } else if (inByte == 'r') {
          unsigned long brightness = Serial.parseInt();
          mirfData = (brightness << 8) | inByte;
          sendRadioData(RADIO_LIGHTBALL);
        } else if (inByte == 'p') {
          unsigned long brightness = Serial.parseInt();
          mirfData = (brightness << 8) | inByte;
          sendRadioData(RADIO_LIGHTBALL);
        } else if (inByte == 'o' || inByte == 'c') {
          unsigned long steps = Serial.parseInt();
          mirfData = (steps << 8) | inByte;
          sendRadioData(RADIO_LIGHTBALL);
        }
      
        break;
        
      case 'c':
      {
        unsigned long motorPosition = Serial.parseInt();
        unsigned long motor = motorPosition % 10;
        unsigned long position = motorPosition / 10;
        if (motor < 4) {
          mirfData = (motor << 8) | (position << 16) | 'M';
          sendRadioData(RADIO_CURTAIN1);
        } else if (motor < 6) {
          mirfData = ((motor-4) << 8) | (position << 16) | 'M';
          sendRadioData(RADIO_CURTAIN1);
        }
        
        break;
      }
        
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


