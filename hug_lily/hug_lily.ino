/*
 */

#include <EEPROM.h>

#define ANALOG_IN     A2
#define GROUND_PIN    10
#define LED           13

#define THRESHOLD     150

int count = 0;

#define EEPROM_COUNT    0

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  
  pinMode(ANALOG_IN, INPUT_PULLUP);
  digitalWrite(ANALOG_IN, HIGH);  // set pullup on analog pin
  
  pinMode(GROUND_PIN, OUTPUT);
  digitalWrite(GROUND_PIN, LOW);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  count = EEPROM.read(EEPROM_COUNT) + (EEPROM.read(EEPROM_COUNT+1) << 8);
  updateCount(count);
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
    debounce = millis() + (state ? 1000 : 200);
    prevState = newState;
  } else if (millis() > debounce) {
    state = newState;
    if (state && !counted) {
      counted = true;
      count++;
      EEPROM.write(EEPROM_COUNT, count & 0xFF);
      EEPROM.write(EEPROM_COUNT+1, (count >> 8) & 0xFF);

      updateCount(count);
    } else if (!state) {
      counted = false;
    }
  }
  
  // print the results to the serial monitor:
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("    count = ");
  Serial.print(count);
  Serial.print("   state = ");
  Serial.println(state ? "ON" : "OFF");

  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(2);
}

void updateCount(int aux) {
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
}

