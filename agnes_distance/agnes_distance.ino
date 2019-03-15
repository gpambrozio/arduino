/*
 * Ultrasonic Sensor HC-SR04 and Arduino Tutorial
 *
 * by Dejan Nedelkovski,
 * https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/
 *
 */

/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        (BLEUUID((uint16_t) 0x8242))
#define CHARACTERISTIC_UUID (BLEUUID((uint16_t) 0x4242))

#define LED      5

// defines variables

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
BLEAdvertising *pAdvertising;

#define BUFFER_SIZE  20

#define SOUND_SPEED_BY_2    0.1715 // 343 m/s divided by 2
#define MAX_DISTANCE   2500

float distance = 0;
float distanceBuffer[BUFFER_SIZE];
float distanceBufferSum = 0;
int distanceBufferPosition = 0;

void setup() {
  memset(distanceBuffer, 0, sizeof(float) * BUFFER_SIZE);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.begin(115200); // Starts the serial communication

  BLEDevice::init("AgnesBehinds");
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ
                    );

  pCharacteristic->setValue(distance);
  pService->start();

  BLEAdvertisementData avdData = BLEAdvertisementData();
  avdData.setShortName("Behinds");
  avdData.setCompleteServices(pService->getUUID());
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponseData(avdData);
  pAdvertising->start();
}

int distanceCM = 0;
int strength = 0;

void loop() {
  distanceCM = 0;
  getTFminiData(&distanceCM, &strength);
  
  if (distanceCM) {
    distance = 10.0 * distanceCM;
    pCharacteristic->setValue(distance);
    Serial.println(distance);
  }
}

// From https://github.com/TFmini/TFmini-Arduino#tfmini_arduino_hardware_serialpolling
void getTFminiData(int* distance, int* strength) {
  static char i = 0;
  char j = 0;
  int checksum = 0; 
  static int rx[9];
  if (Serial.available()) {  
    rx[i] = Serial.read();
    if (rx[0] != 0x59) {
      i = 0;
    } else if (i == 1 && rx[1] != 0x59) {
      i = 0;
    } else if (i == 8) {
      for (j = 0; j < 8; j++) {
        checksum += rx[j];
      }
      if (rx[8] == (checksum % 256)) {
        *distance = rx[2] + rx[3] * 256;
        *strength = rx[4] + rx[5] * 256;
      }
      i = 0;
    } else {
      i++;
    } 
  }  
}
