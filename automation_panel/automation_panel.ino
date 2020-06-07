/*************************************************** 
 *  Agnes' keypad using adafruit's feather ESP32
 ****************************************************/

#include "debug.h"

#include <WiFi.h>

#include <ArduinoOTA.h>

#define LARGE_JSON_BUFFERS 1
#define ARDUINOJSON_USE_LONG_LONG 1

#include <Thing.h>
#include <WebThingAdapter.h>

// IMPORTANT: Check if libraries/TFT_eSPI/User_Setup.h didn't change in latest library update

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Trellis.h>

#include "globals.h"

#define WLAN_WAIKIKI

#include <Passwords.h>

#define NAME  "panel"

// The built in LED
#define LED 13

#define BATTERY_PIN   A13
#define POWER_PIN     A2

#define TFT_LIGHT  27
#define TFT_LIGHT_CHANNEL 0

Adafruit_Trellis matrix0 = Adafruit_Trellis();

// Just one
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);

// For 1.44" and 1.8" TFT with ST7735 use
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

// WebThings things

WebThingAdapter *adapter;

const char *panelTypes[] = {"OnOffSwitch", "Light", "PushButton", nullptr};

ThingDevice panel(NAME, "Control Panel", panelTypes);
ThingProperty ledOn("on", "", BOOLEAN, "OnOffProperty");

ThingProperty *buttons[NUM_KEYS];

void thingsSetup() {
  adapter = new WebThingAdapter(NAME, WiFi.localIP());

  panel.description = "A web connected control panel";
  panel.addProperty(&ledOn);

  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    String sensor = "Button " + String(i + 1);
    buttons[i] = new ThingProperty(sensor.c_str(), sensor.c_str(), BOOLEAN, "PushedProperty");
    buttons[i]->title = sensor;
    panel.addProperty(buttons[i]);
  }
  
  adapter->addDevice(&panel);
  adapter->begin();

  // set initial values
  ThingPropertyValue initialOn = {.boolean = true};
  ledOn.setValue(initialOn);
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

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  int failCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    D(F("."));
    if (++failCount > 10) {
      ESP.restart();
    }
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
  thingsSetup();

  DL(F("setup done"));
}

long nextTFTUpdate = 0;

float battery;
float power;

#define USING_BATTERY  (power < 4.0)

#define MAX_LIGHT_POWER 255
#define MAX_LIGHT_BATTERY 96
#define MAX_LIGHT (USING_BATTERY ? MAX_LIGHT_BATTERY : MAX_LIGHT_POWER)

bool leds[NUM_KEYS];
bool light = true;
bool needTrellisWrite = false;

int8_t keyPressed;
int8_t keyReleased;

void loop() {
  delay(30); // 30ms delay is required, dont remove me!

  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (!wifiConnected) {
    DL(F("WiFi not connected!"));
    ESP.restart();
  } else {
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(NAME);
    ArduinoOTA.begin();
  }

  // Formula from http://cuddletech.com/?p=1030
  battery = ((float)(analogRead(BATTERY_PIN)) / 4095) * 2.0 * 3.3 * 1.1;
  power = ((float)(analogRead(POWER_PIN)) / 4095) * 2.0 * 3.3 * 1.1;

  sigmaDeltaWrite(TFT_LIGHT_CHANNEL, light ? MAX_LIGHT : 0);

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
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
      if (justPressed(i)) {
        ThingPropertyValue value = {.boolean = true};
        buttons[i]->setValue(value);
      } else if (justReleased(i)) {
        ThingPropertyValue value = {.boolean = false};
        buttons[i]->setValue(value);
      }
    }
    if (justPressed(15)) {
      switchLight();
    }
  }

  adapter->update();
  ArduinoOTA.handle();

  bool on = ledOn.getValue().boolean;
  if (on != light) {
    switchLight();
  }

  
  if (needTrellisWrite) {
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
      if (leds[i] && light) {
        trellis.setLED(i);
      } else {
        trellis.clrLED(i);
      }
    }
    // tell the trellis to set the LEDs we requested
    trellis.writeDisplay();
    needTrellisWrite = false;
  }

  if (millis() > nextTFTUpdate) {
    nextTFTUpdate = millis() + 1000;
    img.fillSprite(TFT_BLACK);
    img.setCursor(0, 0, 2);
    img.setTextColor(TFT_WHITE);
    draw();
    img.pushSprite(0, 0);
  }
}

void switchLight() {
  light = !light;
  needTrellisWrite = true;

  ThingPropertyValue value = {.boolean = light};
  ledOn.setValue(value);
}

void draw() {
  long runningSeconds = millis() / 1000;
  img.setTextFont(1);
  img.setTextColor(TFT_WHITE);
  img.println("Sketch running time:");
  img.setTextColor(TFT_MAGENTA, TFT_BLACK);
  long days = runningSeconds / (24 * 60 * 60);
  if (days > 0) {
    img.printf("%ldd, ", days);
    runningSeconds %= 24 * 60 * 60;
  }
  img.printf("%02ld:", runningSeconds / (60 * 60));
  runningSeconds %= 60 * 60;
  img.printf("%02ld.%02ld\n", runningSeconds / 60, runningSeconds % 60);
  img.setTextColor(TFT_WHITE, TFT_BLACK);

  img.printf("Battery: %.2fV\n", battery);
  img.printf("Power: %.2fV\n", power);

  if (WiFi.isConnected()) {
    img.print("IP: ");
    img.println(WiFi.localIP());
  } else {
    img.println("Disconnected!!");
  }
}

void setLED(uint8_t n, bool onOff) {
  if (n >= NUM_KEYS) return;
  leds[n] = onOff;
  needTrellisWrite = true;
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
