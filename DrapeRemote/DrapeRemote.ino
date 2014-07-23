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

#define OPEN_OUTPUT 5
#define CLOSE_OUTPUT 9
#define STOP_OUTPUT 4

int inByte = 0;         // incoming serial byte
unsigned long mirfData;

RF24 radio(2,3);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup()
{
  digitalWrite(OPEN_OUTPUT, LOW);
  digitalWrite(STOP_OUTPUT, LOW);
  digitalWrite(CLOSE_OUTPUT, LOW);
  pinMode(OPEN_OUTPUT, INPUT);
  pinMode(STOP_OUTPUT, INPUT);
  pinMode(CLOSE_OUTPUT, INPUT);

  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("Start");

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(sizeof(unsigned long));
  
  radio.openReadingPipe(1,pipes[0]);
  
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);

  //
  // Start listening
  //

  radio.startListening();
}

void press(int output)
{
  digitalWrite(output, LOW);
  pinMode(output, OUTPUT);
  delay(500);
  pinMode(output, INPUT);
}

void loop()
{
  if (radio.available()) {
    Serial.println("Received data ");
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    Serial.println(mirfData);
    unsigned long command = (mirfData & 0xFF);
    unsigned long time = 1000 * ((mirfData >> 8) & 0xFF);
    Serial.println(time);
    switch (command) {
      case 1:
        press(OPEN_OUTPUT);
        delay(time);
        press(STOP_OUTPUT);
        break;

      case 2:
        press(CLOSE_OUTPUT);
        delay(time);
        press(STOP_OUTPUT);
        break;
    }
  }
}

