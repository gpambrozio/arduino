/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <bluefruit.h>
#include "DHT.h"

#define DHTPIN 7     // what digital pin we're connected to
#define RELAY  16
#define BUTTON 11

// Uncomment whatever type you're using!
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

/** Service Definitions
 * Monitor Service:  0x1234
 * Temperature Char: 0x1235
 * Humidity Char:    0x1236
 */
BLEService        service = BLEService(0x1234);
BLECharacteristic temperatureCharacteristic = BLECharacteristic(0x1235);
BLECharacteristic humidityCharacteristic = BLECharacteristic(0x1236);
BLECharacteristic onoffCharacteristic = BLECharacteristic(0x1237);
BLECharacteristic targetCharacteristic = BLECharacteristic(0x1238);

BLEDis diService;    // DIS (Device Information Service) helper class instance
BLEBas batteryService;    // BAS (Battery Service) helper class instance

void setup()
{
  digitalWrite(RELAY, LOW);
  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  Serial.begin(115200);

  dht.begin();
  
  Serial.println("Bluefruit52 HRM Example");
  Serial.println("-----------------------\n");

  // Initialise the Bluefruit module
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin();
  Bluefruit.autoConnLed(false);

  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);

  // Set the advertised device name (keep it short!)
  Serial.println("Setting Device Name to 'Feather52 HRM'");
  Bluefruit.setName("Thermostat");

  // Set the connect/disconnect callback handlers
  Bluefruit.setConnectCallback(connectCallback);
  Bluefruit.setDisconnectCallback(disconnectCallback);

  // Configure and Start the Device Information Service
  Serial.println("Configuring the Device Information Service");
  diService.setManufacturer("VanTomation");
  diService.setModel("Thermostat+Humid");
  diService.begin();

  // Start the BLE Battery Service and set it to 100%
  Serial.println("Configuring the Battery Service");
  batteryService.begin();
  batteryService.write(100);

  // Setup the Heart Rate Monitor service using
  // BLEService and BLECharacteristic classes
  Serial.println("Configuring Service");
  setupService();

  // Setup the advertising packet(s)
  Serial.println("Setting up the advertising payload(s)");
  startAdv();

  Serial.println("\nAdvertising");
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(service);

  // Include Name
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

uint16_t lastTemperature = 0;
uint16_t lastHuminity = 0;
uint8_t onoff = 0;
uint16_t targetTemperature = 20;

void setupService(void)
{
  // Configure the service
  service.begin();

  // Note: You must call .begin() on the BLEService before calling .begin() on
  // any characteristic(s) within that service definition.. Calling .begin() on
  // a BLECharacteristic will cause it to be added to the last BLEService that
  // was 'begin()'ed!

  // Configure the Temperature characteristic
  // Properties = Notify
  temperatureCharacteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  temperatureCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  temperatureCharacteristic.setFixedLen(sizeof(lastTemperature));
  temperatureCharacteristic.setUserDescriptor("Temperature");
  temperatureCharacteristic.begin();
  temperatureCharacteristic.notify(&lastTemperature, sizeof(lastTemperature));                   // Use .notify instead of .write!

  // Configure the humidity characteristic
  // Properties = Notify
  humidityCharacteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  humidityCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  humidityCharacteristic.setFixedLen(sizeof(lastHuminity));
  humidityCharacteristic.setUserDescriptor("Humidity");
  humidityCharacteristic.begin();
  humidityCharacteristic.notify(&lastHuminity, sizeof(lastHuminity));                   // Use .notify instead of .write!

  onoffCharacteristic.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ | CHR_PROPS_WRITE);
  onoffCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  onoffCharacteristic.setFixedLen(sizeof(onoff));
  onoffCharacteristic.setUserDescriptor("OnOff");
  onoffCharacteristic.setWriteCallback(onoffWriteCallback);
  onoffCharacteristic.begin();
  onoffCharacteristic.notify(&onoff, sizeof(onoff));

  targetCharacteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  targetCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  targetCharacteristic.setFixedLen(sizeof(targetTemperature));
  targetCharacteristic.setUserDescriptor("TargetTemp");
  targetCharacteristic.setWriteCallback(targetWriteCallback);
  targetCharacteristic.begin();
  targetCharacteristic.write(&targetTemperature, sizeof(targetTemperature));
}

void connectCallback(uint16_t conn_handle)
{
  Serial.print("Connected to ");
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));
  Serial.println(central_name);
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason)
{
  Serial.println("Disconnected");
  Serial.println("Advertising!");
}

void onoffWriteCallback(BLECharacteristic& chr, uint8_t* data, uint16_t len, uint16_t offset)
{
  Serial.print("OnOff changed to ");
  if (len < sizeof(onoff)) {
    Serial.print("wrong size "); Serial.println(len);
    return;
  }
  onoff = *data;
  Serial.println(onoff);
  onoffCharacteristic.notify(&onoff, sizeof(onoff));
  digitalWrite(RELAY, onoff ? HIGH : LOW);
}

void targetWriteCallback(BLECharacteristic& chr, uint8_t* data, uint16_t len, uint16_t offset)
{
  Serial.print("Target changed to ");
  if (len < sizeof(targetTemperature)) {
    Serial.print("wrong size "); Serial.println(len);
    return;
  }
  targetTemperature = *((uint16_t*)data);
  Serial.println(targetTemperature);
  targetCharacteristic.notify(&targetTemperature, sizeof(targetTemperature));
  onoff = 1;
  onoffCharacteristic.notify(&onoff, sizeof(onoff));
}

unsigned long nextTemperatureRead = 0;
unsigned long lastSuccessfullTemperatureRead = 0;
unsigned long nextBlink = 0;

void loop()
{
  if (millis() >= nextBlink) {
    if (millis() >= nextBlink + 10) {
      digitalWrite(LED_RED, LOW);
      nextBlink = millis() + 15000;
    } else {
      digitalWrite(LED_RED, HIGH);
    }
  }

  if (millis() >= nextTemperatureRead) {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    float t = dht.readTemperature();
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("NaN");
    } else {
      lastSuccessfullTemperatureRead = millis();
      
      lastTemperature = (uint16_t)(t * 10);
      lastHuminity = (uint16_t)(h * 10);
  
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print(" *C\t");
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %");

      // Note: We use .notify instead of .write!
      // The characteristic's value is still updated although notification is not sent
      temperatureCharacteristic.notify(&lastTemperature, sizeof(lastTemperature));
      humidityCharacteristic.notify(&lastHuminity, sizeof(lastHuminity));
    }

    nextTemperatureRead = millis() + 15000;
  }

  delay(100);
}

