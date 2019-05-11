/*************************************************** 
 *  Agnes' keypad using adafruit's feather ESP32
 ****************************************************/

#include <Arduino.h>

#include <WiFiMulti.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>

// IMPORTANT: Check if libraries/TFT_eSPI/User_Setup.h didn't change in latest library update

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

#define NAME  "keypad"

// The built in LED
#define LED 13

#define BATTERY_PIN   A13
#define POWER_PIN     A2

#define TFT_LIGHT  27
#define TFT_LIGHT_CHANNEL 0

WiFiMulti wifiMulti;

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
byte mode = 1;

void recreateWifi() {
  wifiMulti = WiFiMulti();
  wifiMulti.addAP(WLAN_SSID, WLAN_PASS);
}

void setup() {

  Serial.begin(115200);
  DL(F(NAME));

  pinMode(BATTERY_PIN, INPUT);
  pinMode(POWER_PIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Wire.begin();
  Wire.setClock(100000);

  // begin() with the addresses of each panel in order
  trellis.begin(0x70);  // only one
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
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
  img.printf("Starting...");
  img.pushSprite(0, 0);

  recreateWifi();

  if (MDNS.begin(NAME)) {
    DL(F("MDNS responder started"));
  }

  pinMode(TFT_LIGHT, OUTPUT);

  //setup channel 0 with frequency 312500 Hz
  sigmaDeltaSetup(TFT_LIGHT_CHANNEL, 8096);
  //attach TFT_LIGHT to channel 0
  sigmaDeltaAttachPin(TFT_LIGHT, 0);
  //initialize channel 0 to off
  sigmaDeltaWrite(TFT_LIGHT_CHANNEL, 255);

  // light up all the LEDs in order
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();
    delay(30);
  }
  // then turn them off
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();
    delay(30);
  }

  for (uint8_t i = 0; i < NUMBER_OF_MODES; i++) {
    modes[i]->init();
  }
  modes[mode]->isActive = true;
  modes[mode]->setup();
  
  DL(F("setup done"));
}

WiFiClient client;

long nextTFTUpdate = 0;

float battery;
float power;

std::vector<String> commandsToSend;

#define USING_BATTERY  (power < 4.0)

#define LIGHT_CHANGE 32
#define MAX_LIGHT_POWER 255
#define MAX_LIGHT_BATTERY 96
#define MAX_LIGHT (USING_BATTERY ? MAX_LIGHT_BATTERY : MAX_LIGHT_POWER)

int light = MAX_LIGHT_POWER;
bool needTrellisWrite = false;

int8_t keyPressed;
int8_t keyReleased;

void loop() {
  delay(30); // 30ms delay is required, dont remove me!

  bool wifiConnected = wifiMulti.run() == WL_CONNECTED;
  if (!wifiConnected) {
    DL(F("WiFi not connected!"));
  } else {
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(NAME);
    ArduinoOTA.begin();
  }

  // Formula from http://cuddletech.com/?p=1030
  battery = ((float)(analogRead(BATTERY_PIN)) / 4095) * 2.0 * 3.3 * 1.1;
  power = ((float)(analogRead(POWER_PIN)) / 4095) * 2.0 * 3.3 * 1.1;

  light = min(light, MAX_LIGHT);
  if (light < 0) light = 0;   // min is not defined for some reason...
  sigmaDeltaWrite(TFT_LIGHT_CHANNEL, light);

  bool hasSwitchChanges = false;
  keyReleased = -1;

  // If a button was just pressed or released...
  if (trellis.readSwitches()) {
    uint8_t presses = 0;
    uint8_t pressed = 0;
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
      if (trellis.justPressed(i)) {
        presses++;
        pressed = i;
      }
    }

    if (presses == 1 && keyPressed == -1) {
      keyPressed = pressed;
      hasSwitchChanges = true;
    } else if (presses == 0 && keyPressed >= 0 && !trellis.isKeyPressed(keyPressed)) {
      hasSwitchChanges = true;
      keyReleased = keyPressed;
      keyPressed = -1;
    }
  }

  if (hasSwitchChanges) {
    if (justPressed(12)) {
      if (mode == 0) changeMode(NUMBER_OF_MODES - 1);
      else changeMode(mode-1);
    }
    if (justPressed(13)) {
      if (mode >= NUMBER_OF_MODES-1) changeMode(0);
      else changeMode(mode+1);
    }
    if (justPressed(14)) {
      light -= LIGHT_CHANGE;
    }
    if (justPressed(15)) {
      light += LIGHT_CHANGE;
    }
    modes[mode]->checkKeys();
  }

  if (wifiConnected) {
    if (!client.connected()) {
      DL(F("connecting to server."));
      client.stop();
      if (client.connect(WiFi.gatewayIP(), 5000)) {
        DL(F("connected to server."));
        client.setTimeout(15);
        client.print("Keypad\n");
      } else {
        DL(F("failed connecting to server."));
      }
    } else if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("M")) {
        char requiredMode = line.charAt(1);
        
        for (uint8_t i=0; i<NUMBER_OF_MODES; i++) {
          if (requiredMode == modes[i]->identifier()) {
            changeMode(i);
            break;
          }
        }
      } else {
        for (uint8_t i=0; i<NUMBER_OF_MODES; i++) {
          modes[i]->checkCommand(line);
        }
      }
    } else if (!commandsToSend.empty()) {
      for (uint8_t i=0; i<commandsToSend.size(); i++) {
        client.print(commandsToSend[i] + "\n");
      }
      commandsToSend.clear();
    }
    ArduinoOTA.handle();
  } else {
    client.stop();
  }
  
  if (needTrellisWrite) {
    // tell the trellis to set the LEDs we requested
    trellis.writeDisplay();
    needTrellisWrite = false;
  }

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

void changeMode(byte newMode) {
  if (mode != newMode) {
    scheduleScreenRefresh();
    setKeysBrightness(15);
    modes[mode]->tearDown();
    modes[mode]->isActive = false;
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
      setLED(i, false);
    }
    mode = newMode;
    modes[mode]->isActive = true;
    modes[mode]->setup();
  }
}

void setLED(uint8_t n, bool onOff) {
  needTrellisWrite = true;
  if (onOff) {
    trellis.setLED(n);
  } else {
    trellis.clrLED(n);
  }
}

bool justPressed(uint8_t n) {
  return keyPressed == n;
}

bool justReleased(uint8_t n) {
  return keyReleased == n;
}

void setKeysBrightness(uint8_t brightness) {
  needTrellisWrite = true;
  trellis.setBrightness(brightness);
}

inline void scheduleScreenRefresh() {
  nextTFTUpdate = 0;
}

void addCommand(String command) {
  commandsToSend.push_back(command);
}
