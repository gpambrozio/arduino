#include "Common.h"
#include "Strip.h"

#define PIN_OUTSIDE 15
#define PIN_INSIDE 16

#define MAX_BRIGHTNESS_OUTSIDE 200
#define MAX_BRIGHTNESS_INSIDE 40

BLEService service = BLEService(0x2234);

BLEDis diService;    // DIS (Device Information Service) helper class instance

#define READ_BUFSIZE                    (20)
 
Strip inside  = Strip(0x2235, 259, PIN_INSIDE, MAX_BRIGHTNESS_INSIDE);
Strip outside = Strip(0x2236, 150, PIN_OUTSIDE, MAX_BRIGHTNESS_OUTSIDE);

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DL("Start");
  pinMode(LED_RED, OUTPUT);

  delay(1000);

  inside.begin();
  outside.begin();
  
  DL("Starting BT");
  Bluefruit.begin();
  Bluefruit.autoConnLed(false);

  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("AgnesLights");

  // Set the connect/disconnect callback handlers
  Bluefruit.setConnectCallback(connectCallback);
  Bluefruit.setDisconnectCallback(disconnectCallback);

  DL("Configuring the Device Information Service");
  diService.setManufacturer("VanTomation");
  diService.setModel("Lights");
  diService.begin();

  // Setup the Heart Rate Monitor service using
  // BLEService and BLECharacteristic classes
  DL("Configuring Service");
  setupService();

  // Setup the advertising packet(s)
  DL("Setting up the advertising payload(s)");
  startAdv();

  DL("Ending setup");
}

unsigned long nextBlink = 0;
void loop() {
  if (millis() >= nextBlink) {
    if (millis() >= nextBlink + 10) {
      digitalWrite(LED_RED, LOW);
      nextBlink = millis() + 15000;
      DL("Loop");
    } else {
      digitalWrite(LED_RED, HIGH);
    }
  }

  inside.loop();
  outside.loop();
}

void setupService()
{
  // Configure the service
  service.begin();

  // Note: You must call .begin() on the BLEService before calling .begin() on
  // any characteristic(s) within that service definition.. Calling .begin() on
  // a BLECharacteristic will cause it to be added to the last BLEService that
  // was 'begin()'ed!

  inside.setupService();
  outside.setupService();
}

void startAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(service);
  
  // There is no room for 'Name' in the Advertising packet
  // Use the optional secondary Scan Response packet for 'Name' instead
  Bluefruit.ScanResponse.addName();

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

void connectCallback(uint16_t conn_handle) {
  D("Connected to ");
  char centralName[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, centralName, sizeof(centralName));
  DL(centralName);
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason) {
  DL("Disconnected");
}
