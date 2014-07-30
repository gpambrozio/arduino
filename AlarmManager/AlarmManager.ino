/*
  Serial Call and Response
 Language: Wiring/Arduino
 
 This program sends an ASCII A (byte of value 65) on startup
 and repeats that until it gets some data in.
 Then it waits for a byte in the serial port, and 
 sends three sensor values whenever it gets a byte in.
 
 Thanks to Greg Shakar and Scott Fitzgerald for the improvements
 
 Created 26 Sept. 2005
 by Tom Igoe
 modified 24 April 2012
 by Tom Igoe and Scott Fitzgerald

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/SerialCallResponse

 * Pins:
 * Hardware SPI:
 * MISO -> 12
 * MOSI -> 11
 * SCK -> 13
 *
 * Configurable:
 * CE -> 8
 * CSN -> 7
 */
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define STAY_OUTPUT 11
#define AWAY_OUTPUT 12

int inByte = 0;         // incoming serial byte
unsigned long lastContactTime = 0;
unsigned long mirfData;

RF24 radio(2,3);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

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

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(sizeof(unsigned long));
  
  radio.openReadingPipe(1,pipes[1]);
  
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);

  //
  // Start listening
  //

  radio.startListening();

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
        radio.stopListening();
        radio.openWritingPipe(pipes[0]);
        radio.write((byte *)&mirfData, sizeof(unsigned long));
        radio.startListening();
        break;
        
      case 'C':
        Serial.println("Received Close");
        inByte = Serial.read();
        mirfData = (inByte - '0') * 10;
        inByte = Serial.read();
        mirfData += (inByte - '0');
        mirfData <<= 8;
        mirfData |= 2;
        radio.stopListening();
        radio.openWritingPipe(pipes[0]);
        radio.write((byte *)&mirfData, sizeof(unsigned long));
        radio.startListening();
        break;
        
      default:
        Serial.print("Received ");
        Serial.println(inByte);
    }
    
    lastContactTime = millis();
  }
  
  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    Serial.print("Received data ");
    Serial.println(mirfData);
  }
}


