/*
 */

#include <EEPROM.h>

#define ANALOG_IN     A0
#define VIBRATION     2

#define INITIAL_VALUE      200
#define SENSOR_OPEN_LIMIT  1024
#define THRESHOLD          0.8
#define HUG_ON_TIME        300
#define HUG_OFF_TIME       1200
#define VIBRATION_TIME     700

#define LOOP_SLEEP       50
#define LOOP_LONG_SLEEP  500

#define EEPROM_COUNT    0

#define HISTORY_SIZE_PO2   4
#define HISTORY_SIZE       (1 << HISTORY_SIZE_PO2)

uint16_t sensorHistory[HISTORY_SIZE];
uint8_t sensorHistoryPosition = 0;
uint32_t sensorHistorySum;
uint32_t sensorRecentSum = 0;
uint8_t sensorSumCount = 0;
uint16_t sensorThreashold;

uint16_t count = 0;
uint16_t sensorValue = 0; 
uint32_t sensorHistoryAvg;
uint32_t loopNumber = 0;
uint32_t nextPing;

#define SCRATCH_SIZE  (sizeof(count) + sizeof(sensorHistoryAvg) + sizeof(sensorValue) + sizeof(loopNumber) + sizeof(nextPing))
uint8_t *scratchData = (uint8_t*)&count;

#define RECENT_COUNT_PO2   3
#define COUNT_LIMIT        (1 << RECENT_COUNT_PO2)

uint32_t debounce = 0;
uint32_t startVibration = 0;
boolean vibrating = false;
boolean sentBatteryLow = false;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(57600);
  
  // on readBytes, return after 5ms or when the buffer is full
  Serial.setTimeout(5);

  pinMode(ANALOG_IN, INPUT_PULLUP);
  
  pinMode(VIBRATION, OUTPUT);
  digitalWrite(VIBRATION, LOW);

  count = EEPROM.read(EEPROM_COUNT) + (EEPROM.read(EEPROM_COUNT+1) << 8);
  
  for (int i = 0; i < HISTORY_SIZE; i++) {
    sensorHistory[i] = INITIAL_VALUE;
  }
  sensorHistorySum = HISTORY_SIZE * INITIAL_VALUE;
  sensorHistoryAvg = INITIAL_VALUE;
  sensorThreashold = (uint16_t)((float)INITIAL_VALUE * THRESHOLD);

  updateScratchData();

  nextPing = (60000 / LOOP_SLEEP);
}

boolean state = false;
boolean prevState = false;
boolean newState = false;
boolean counted = false;
char serialCommand;

void loop() {
  // read the analog in value:
  sensorValue = analogRead(ANALOG_IN);
  
  // Ignore anything too high
  if (sensorValue < SENSOR_OPEN_LIMIT) {
    sensorRecentSum += sensorValue;
    if (++sensorSumCount == COUNT_LIMIT) {
      sensorRecentSum >>= RECENT_COUNT_PO2;
      sensorHistorySum -= sensorHistory[sensorHistoryPosition];
      sensorHistory[sensorHistoryPosition] = sensorRecentSum;
      sensorHistorySum += sensorRecentSum;
      if (++sensorHistoryPosition == HISTORY_SIZE) {
        sensorHistoryPosition = 0;
      }
      sensorSumCount = 0;
      sensorRecentSum = 0;
      sensorHistoryAvg = sensorHistorySum >> HISTORY_SIZE_PO2;
      sensorThreashold = (uint16_t)((float)sensorHistoryAvg * THRESHOLD);
    }
  }
  
  newState = (sensorValue < sensorThreashold);
  if (newState != prevState) {
    debounce = loopNumber + (state ? HUG_OFF_TIME : HUG_ON_TIME) / LOOP_SLEEP;
    prevState = newState;
  } else if (loopNumber > debounce) {
    state = newState;
    if (state && !counted) {
      counted = true;
      count++;
      EEPROM.write(EEPROM_COUNT, count & 0xFF);
      EEPROM.write(EEPROM_COUNT+1, (count >> 8) & 0xFF);

      Serial.println(String("count:") + String(count));

      startVibration = loopNumber;
      vibrating = true;
      digitalWrite(VIBRATION, HIGH);
    } else if (!state) {
      counted = false;
    }
  }
  
  // battery goes low while vibrating. Ignore it.
  if (!vibrating) {
    uint16_t batteryV = Bean.getBatteryVoltage();
    if (batteryV <= 210) {
      if (!sentBatteryLow) {
        sentBatteryLow = true;
        Serial.println("battery:0");
      }
    } else {
      sentBatteryLow = false;
    }
  }
  
  if (loopNumber >= nextPing) {
    Serial.println(String("ping:") + String(loopNumber));
    nextPing = loopNumber + (600000 / LOOP_SLEEP);
  } 

  updateScratchData();

  if (sensorValue >= SENSOR_OPEN_LIMIT || loopNumber > startVibration + (VIBRATION_TIME / LOOP_SLEEP)) {  
    digitalWrite(VIBRATION, LOW);
    vibrating = false;
  }
  
  if (Serial.available()) {
    if (Serial.readBytes(&serialCommand, 1) == 1) {
      switch (serialCommand) {
        case 'r':
          count = 0;
          EEPROM.write(EEPROM_COUNT, 0);
          EEPROM.write(EEPROM_COUNT+1, 0);
          break;
  
        default:
          Serial.println(String("Unknown:") + String(serialCommand));
          break;
      }
    }
  }

  Bean.sleep((sensorValue < SENSOR_OPEN_LIMIT) ? LOOP_SLEEP : LOOP_LONG_SLEEP);
  if (sensorValue < SENSOR_OPEN_LIMIT) {
    ++loopNumber;
  } else {
    loopNumber += LOOP_LONG_SLEEP / LOOP_SLEEP;
  }
}

void updateScratchData() {
  Bean.setScratchData(1, scratchData, SCRATCH_SIZE);
}

