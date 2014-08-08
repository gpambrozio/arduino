/*
 */

#include <EEPROM.h>

#define ANALOG_IN     A0
#define VIBRATION     2

#define THRESHOLD     370

int count = 0;

#define EEPROM_COUNT    0

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(57600);
  
  pinMode(ANALOG_IN, INPUT_PULLUP);
  
  pinMode(VIBRATION, OUTPUT);
  digitalWrite(VIBRATION, LOW);

  count = EEPROM.read(EEPROM_COUNT) + (EEPROM.read(EEPROM_COUNT+1) << 8);
  updateScratchData();
}

uint16_t sensorValue = 0;        // value read from the pot

unsigned long debounce = 0;
boolean state = false;
boolean prevState = false;
boolean newState = false;
boolean counted = false;

void loop() {
  // read the analog in value:
  sensorValue = analogRead(ANALOG_IN);
  newState = (sensorValue < THRESHOLD);
  if (newState != prevState) {
    debounce = millis() + (state ? 500 : 100);
    prevState = newState;
  } else if (millis() > debounce) {
    state = newState;
    if (state && !counted) {
      counted = true;
      count++;
      EEPROM.write(EEPROM_COUNT, count & 0xFF);
      EEPROM.write(EEPROM_COUNT+1, (count >> 8) & 0xFF);

      updateCount(count);
      updateScratchData();
    } else if (!state) {
      counted = false;
    }
  }
  
  Bean.setScratchData(2, (uint8_t *)&sensorValue, sizeof(sensorValue));
  // print the results to the serial monitor:
//  Serial.print("sensor = ");
//  Serial.println(sensorValue);
//  Serial.print("    count = ");
//  Serial.print(count);
//  Serial.print("   state = ");
//  Serial.println(state ? "ON" : "OFF");

  Bean.sleep(50);
}

void updateScratchData() {
  Bean.setScratchData(1, (uint8_t *)&count, sizeof(count));
}

void updateCount(int aux) {
  digitalWrite(VIBRATION, HIGH);
  Bean.setLed(0, 0, 255);
  unsigned long start = millis();
  
  Serial.println(count);
  
  Bean.sleep(1000 - (millis() - start));
  Bean.setLed(0,0,0);
  digitalWrite(VIBRATION, LOW);
}

