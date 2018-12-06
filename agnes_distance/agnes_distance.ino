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

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// defines pins numbers
#define trigPin  26
#define echoPin  25

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
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
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
  avdData.setCompleteServices(pService->getUUID());
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponseData(avdData);
  pAdvertising->start();
}

void loop() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH, (long)((float)MAX_DISTANCE / SOUND_SPEED_BY_2));
  
  if (duration > 20) {
    // Calculating the distance
    distance = (float)duration * SOUND_SPEED_BY_2;

    distanceBufferSum -= distanceBuffer[distanceBufferPosition];
    distanceBufferSum += distance;
    distanceBuffer[distanceBufferPosition] = distance;

    if (++distanceBufferPosition >= BUFFER_SIZE)
      distanceBufferPosition = 0;
    
    distance = distanceBufferSum / BUFFER_SIZE;
    pCharacteristic->setValue(distance);

    Serial.println(distance);
  }
}
