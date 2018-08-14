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
BLECharacteristic tempc = BLECharacteristic(0x1235);
BLECharacteristic humc = BLECharacteristic(0x1236);
BLECharacteristic onoffc = BLECharacteristic(0x1237);

BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas;    // BAS (Battery Service) helper class instance

// Advanced function prototypes
void startAdv(void);
void setupHRM(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

void setup()
{
  Serial.begin(115200);
  dht.begin();
  
  Serial.println("Bluefruit52 HRM Example");
  Serial.println("-----------------------\n");

  // Initialise the Bluefruit module
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin();

  // Set the advertised device name (keep it short!)
  Serial.println("Setting Device Name to 'Feather52 HRM'");
  Bluefruit.setName("Thermostat");

  // Set the connect/disconnect callback handlers
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  // Configure and Start the Device Information Service
  Serial.println("Configuring the Device Information Service");
  bledis.setManufacturer("VanTomation");
  bledis.setModel("Thermostat+Humid");
  bledis.begin();

  // Start the BLE Battery Service and set it to 100%
  Serial.println("Configuring the Battery Service");
  blebas.begin();
  blebas.write(100);

  // Setup the Heart Rate Monitor service using
  // BLEService and BLECharacteristic classes
  Serial.println("Configuring Service");
  setupService();

  // Setup the advertising packet(s)
  Serial.println("Setting up the advertising payload(s)");
  startAdv();

  Serial.println("Ready Player One!!!");
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

uint16_t tempdata = 0;
uint16_t humdata = 0;
uint8_t onoff = 0;

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
  tempc.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  tempc.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  tempc.setFixedLen(sizeof(tempdata));
  tempc.setUserDescriptor("Temperature");
  tempc.begin();
  tempc.notify(&tempdata, sizeof(tempdata));                   // Use .notify instead of .write!

  // Configure the humidity characteristic
  // Properties = Notify
  humc.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  humc.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  humc.setFixedLen(sizeof(humdata));
  humc.setUserDescriptor("Humidity");
  humc.begin();
  humc.notify(&humdata, sizeof(humdata));                   // Use .notify instead of .write!

  onoffc.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ | CHR_PROPS_WRITE);
  onoffc.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  onoffc.setFixedLen(sizeof(onoff));
  onoffc.setUserDescriptor("OnOff");
  onoffc.setWriteCallback(onoff_write_cb);
  onoffc.begin();
  onoffc.notify(&onoff, sizeof(onoff));                   // Use .notify instead of .write!
}

void connect_callback(uint16_t conn_handle)
{
  Serial.print("Connected to ");
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));
  Serial.println(central_name);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  Serial.println("Disconnected");
  Serial.println("Advertising!");
}

void onoff_write_cb(BLECharacteristic& chr, uint8_t* data, uint16_t len, uint16_t offset)
{
  Serial.print("OnOff changed to ");
  if (len < sizeof(onoff)) {
    Serial.print("wrong size "); Serial.println(len);
    return;
  }
  onoff = *data;
  Serial.println(onoff);
}

void loop()
{
  digitalToggle(LED_RED);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("NaN");
  } else {
    tempdata = (uint16_t)(t * 10);
    humdata = (uint16_t)(h * 10);

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C\t");
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println(" %");
  }

  if (Bluefruit.connected()) {
    Serial.println("Updating");
    
    // Note: We use .notify instead of .write!
    // The characteristic's value is still updated although notification is not sent
    tempc.notify(&tempdata, sizeof(tempdata));
    humc.notify(&humdata, sizeof(humdata));
  }

  // Only send update once per second
  delay(2000);
}

