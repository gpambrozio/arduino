/*************************************************** 
 *  Agnes' keypad using adafruit's feather ESP32
 ****************************************************/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

// IMPORTANT: Wire might be broken for ESP32
// To fix see this: https://github.com/espressif/arduino-esp32/issues/741#issuecomment-374325594

#include <Wire.h>
#include <SPI.h>

#include <vector>

#include "Common.h"
#include "Mode.h"
#include "ModeDebug.h"
#include "ModeDrive.h"
#include "ModeHouse.h"

#define WLAN_AGNES

#include <Passwords.h>

// The built in LED
#define LED 13

#define BATTERY_PIN   A13
#define POWER_PIN     A2

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define numKeys (NUMTRELLIS * 16)

#define TFT_LIGHT  27
#define TFT_LIGHT_CHANNEL 0

WiFiMulti wifiMulti;
WebServer server(80);

Adafruit_Trellis matrix0 = Adafruit_Trellis();

// Just one
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);

// For 1.44" and 1.8" TFT with ST7735 use
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

Mode *modes[] = {
  new ModeDebug(),
  new ModeHouse(),
  new ModeDrive(),
};

#define NUMBER_OF_MODES  (sizeof(modes) / sizeof(Mode *))
byte mode = 0;

void setup() {

  Serial.begin(115200);
  Serial.println("Keypad");

  pinMode(BATTERY_PIN, INPUT);
  pinMode(POWER_PIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Wire.begin();
  Wire.setClock(100000);

  // begin() with the addresses of each panel in order
  trellis.begin(0x70);  // only one
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.clrLED(i);
  }
  trellis.writeDisplay();

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  img.setColorDepth(8);
  img.createSprite(TFT_WIDTH, TFT_HEIGHT);
  
  img.setTextColor(TFT_WHITE, TFT_BLACK);
  img.fillSprite(TFT_BLACK);
  img.setCursor(0, 0, 2);
  img.printf("Connecting to %s", WLAN_SSID);
  img.pushSprite(0, 0);

  wifiMulti.addAP(WLAN_SSID, WLAN_PASS);

  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Retrying...");
    delay(1000);
  }
  img.fillSprite(TFT_BLACK);
  img.setCursor(0, 0, 2);
  img.printf("Connected to %s ", WLAN_SSID);
  img.pushSprite(0, 0);
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("keypad")) {
    Serial.println("MDNS responder started");
  }

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("keypad");
  ArduinoOTA.begin();
  
  server.on("/", []() {
    trellis.begin(0x70);  // only one
    server.send(200, "text/plain", "OK");
  });
  server.begin();

  pinMode(TFT_LIGHT, OUTPUT);

  //setup channel 0 with frequency 312500 Hz
  sigmaDeltaSetup(TFT_LIGHT_CHANNEL, 8096);
  //attach TFT_LIGHT to channel 0
  sigmaDeltaAttachPin(TFT_LIGHT, 0);
  //initialize channel 0 to off
  sigmaDeltaWrite(TFT_LIGHT_CHANNEL, 255);

  // light up all the LEDs in order
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();
    delay(30);
  }
  // then turn them off
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();
    delay(30);
  }

  tft.fillScreen(TFT_BLACK);

  for (uint8_t i=0; i<NUMBER_OF_MODES; i++) {
    modes[i]->init();
  }
  
  Serial.println("setup done");
}

HTTPClient http;
WiFiClient client;

long nextTFTUpdate = 0;

float battery;
float power;

std::vector<String> commandsToSend;

#define LIGHT_CHANGE 32
#define MAX_LIGHT_POWER 255
#define MAX_LIGHT_BATTERY 96
#define MAX_LIGHT (power < 4.0 ? MAX_LIGHT_BATTERY : MAX_LIGHT_POWER)

int light = MAX_LIGHT_POWER;

void loop() {
  delay(30); // 30ms delay is required, dont remove me!

  // If a button was just pressed or released...
  if (trellis.readSwitches()) {
    byte previousMode = mode;
    
    if (trellis.justPressed(12)) {
      if (mode == 0) mode = NUMBER_OF_MODES - 1;
      else mode--;
    }
    if (trellis.justPressed(13)) {
      if (++mode >= NUMBER_OF_MODES) mode = 0;
    }
    if (trellis.justPressed(14)) {
      light -= LIGHT_CHANGE;
    }
    if (trellis.justPressed(15)) {
      light += LIGHT_CHANGE;
    }
    if (previousMode != mode) {
      nextTFTUpdate = 0;
      modes[previousMode]->tearDown();
      for (uint8_t i=0; i<numKeys; i++) {
        trellis.clrLED(i);
      }
      modes[mode]->setup();
    }
    modes[mode]->checkKeys();
    // tell the trellis to set the LEDs we requested
    trellis.writeDisplay();
  }

  if (!client.connected()) {
    Serial.println("connecting to server.");
    client.stop();
    if (client.connect(WiFi.gatewayIP(), 5000)) {
      Serial.println("connected to server.");
      client.print("Panel\n");
    }
  } else if (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.print("Received "); Serial.println(line);
    for (uint8_t i=0; i<NUMBER_OF_MODES; i++) {
      modes[i]->checkCommand(line);
    }
  } else if (!commandsToSend.empty()) {
    for (uint8_t i=0; i<commandsToSend.size(); i++) {
      client.print(commandsToSend[i] + "\n");
    }
    commandsToSend.clear();
  }
  
  // Formula from http://cuddletech.com/?p=1030
  battery = ((float)(analogRead(BATTERY_PIN)) / 4095) * 2.0 * 3.3 * 1.1;
  power = ((float)(analogRead(POWER_PIN)) / 4095) * 2.0 * 3.3 * 1.1;

  light = min(light, MAX_LIGHT);
  if (light < 0) light = 0;   // min is not defined for some reason...
  sigmaDeltaWrite(TFT_LIGHT_CHANNEL, light);

  ArduinoOTA.handle();
  server.handleClient();
  if (millis() > nextTFTUpdate) {
    nextTFTUpdate = millis() + 1000;
    img.fillSprite(TFT_BLACK);
    img.setCursor(0, 0, 2);
    img.setTextColor(TFT_WHITE);
    img.print(modes[mode]->name());
    img.println(" mode");
    modes[mode]->draw();
    img.pushSprite(0, 0);
  }
}

void addCommand(String command) {
  commandsToSend.push_back(command);
}
