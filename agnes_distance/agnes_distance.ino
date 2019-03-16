/*
 * Sparkfun ESP32 Thing
 */

#include <BLEDevice.h>

#define SERVICE_UUID        (BLEUUID((uint16_t) 0x8242))
#define CHARACTERISTIC_UUID (BLEUUID((uint16_t) 0x4242))

#define LED     5

#define TX2     25
#define RX2     26

#define NAME    "Behinds"

// defines variables

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
BLEAdvertising *pAdvertising;

HardwareSerial SerialTFMini(2);

float distance = 0;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.begin(115200); // Starts the serial communication
  
  SerialTFMini.begin(115200, SERIAL_8N1, TX2, RX2);

  BLEDevice::init(NAME);
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ
                    );

  pCharacteristic->setValue(distance);
  pService->start();

  BLEAdvertisementData avdData = BLEAdvertisementData();
  avdData.setShortName(NAME);
  avdData.setCompleteServices(pService->getUUID());
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponseData(avdData);
  pAdvertising->start();

  Serial.println("started");
}

int distanceCM = 0;
int lastDistance = 0;
int strength = 0;

void loop() {
  distanceCM = 0;
  getTFminiData(&distanceCM, &strength);
  
  if (distanceCM && lastDistance != distanceCM) {
    lastDistance = distanceCM;
    distance = 10.0 * distanceCM;
    pCharacteristic->setValue(distance);
    Serial.println(distance);
  }
}

// From https://github.com/TFmini/TFmini-Arduino#tfmini_arduino_hardware_serialpolling
void getTFminiData(int* distance, int* strength) {
  static char i = 0;
  static int rx[9];
  if (SerialTFMini.available()) {  
    rx[i] = SerialTFMini.read();
    if (rx[0] != 0x59) {
      i = 0;
    } else if (i == 1 && rx[1] != 0x59) {
      i = 0;
    } else if (i == 8) {
      int checksum = 0;
      for (char j = 0; j < 8; j++) {
        checksum += rx[j];
      }
      if (rx[8] == (checksum & 0xFF)) {
        *distance = rx[2] + rx[3] * 256;
        *strength = rx[4] + rx[5] * 256;
      }
      i = 0;
    } else {
      i++;
    } 
  }  
}
