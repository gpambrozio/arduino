/*
 */

#include <SPI.h>
#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define PANIC_BUTTON 17
#define SIREN_IN      2

byte inByte;
unsigned long mirfData;

RF24 radio(14, 16);

void setup()
{
  pinMode(SIREN_IN, INPUT_PULLUP);

  digitalWrite(PANIC_BUTTON, LOW);
  pinMode(PANIC_BUTTON, OUTPUT);

  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //
  // Setup and configure rf radio
  //

  START_RADIO(radio, RADIO_ALARM);

  Serial.println("starting");
}

void press(int output)
{
  digitalWrite(output, HIGH);
  delay(1500);
  digitalWrite(output, LOW);
}

unsigned long startDebounce = 0;
int lastState = HIGH;
boolean alarmSent = false;

void loop()
{
  int state = digitalRead(SIREN_IN);
  if (state != lastState) {
    lastState = state;
    startDebounce = millis();
    alarmSent = false;
    Serial.println(state == HIGH ? "High Detected" : "Low detected");
  } else if (state == LOW && (millis() - startDebounce) >= 20 && !alarmSent) {
    Serial.println("Alarm detected");
    radio.stopListening();
    radio.openWritingPipe(pipes[RADIO_SERVER]);
    mirfData = 'A';
    alarmSent = radio.write((byte *)&mirfData, sizeof(unsigned long));
    radio.startListening();
  }
  
  // if we get a valid byte, read analog ins:
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    switch (inByte) {
      case 'P':
        Serial.println("Received Panic");
        press(PANIC_BUTTON);
        break;
        
      default:
        Serial.print("Received ");
        Serial.println(inByte);
    }
  }
  
  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    switch (inByte) {
      case 'P':
        Serial.println("Received Panic");
        press(PANIC_BUTTON);
        break;
        
      default:
        Serial.print("Received data ");
        Serial.println(mirfData);
    }
  }
}


