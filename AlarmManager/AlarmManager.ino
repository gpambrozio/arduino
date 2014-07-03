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
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

#define STAY_OUTPUT 11
#define AWAY_OUTPUT 12

int inByte = 0;         // incoming serial byte
unsigned long lastContactTime = 0;
unsigned long mirfData;

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

  Mirf.spi = &MirfHardwareSpi;
  Mirf.csnPin = 3;
  Mirf.cePin = 2;
  Mirf.init();
  
  /*
   * Configure reciving address.
   */
   
  Mirf.setRADDR((byte *)"serv1");
  
  /*
   * Set the payload length to sizeof(unsigned long) the
   * return type of millis().
   *
   * NB: payload on client and server must be the same.
   */
   
  Mirf.payload = sizeof(unsigned long);
  
  /*
   * Write channel and payload config then power up reciver.
   */
   
  /*
   * To change channel:
   * 
   * Mirf.channel = 10;
   *
   * NB: Make sure channel is legal in your area.
   */
   
  Mirf.config();

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
      press(AWAY_OUTPUT);
      break;
      
    case 'S':
      press(STAY_OUTPUT);
      break;
    }
    
    Serial.print("Received ");
    Serial.println(inByte);
    lastContactTime = millis();
  }
  if (Mirf.dataReady()) {
    Mirf.getData((byte *)&mirfData);
    Serial.print("Received data ");
    Serial.println(mirfData);
  }
}


