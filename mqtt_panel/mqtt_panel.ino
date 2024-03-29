/*************************************************** 
 *  Agnes' keypad using adafruit's feather ESP32
 ****************************************************/

#define DEBUG
#include <DebugMacros.h>

#include <WiFi.h>

#include <ArduinoOTA.h>

// IMPORTANT: Check if libraries/TFT_eSPI/User_Setup.h didn't change in latest library update

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Trellis.h>

#include "globals.h"

#define WLAN_WAIKIKI
#include <Passwords.h>

#define NAME  "controller"

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

#include <PubSubClient.h>

String statusTopic("homeassistant/status");

String discoveryPrefix("homeassistant/");

String componentButtons("binary_sensor/");
String buttonsPrefix = discoveryPrefix + componentButtons + NAME + "/button";

String componentLights("light/");
String lightsPrefix = discoveryPrefix + componentLights + NAME + "/light";

WiFiClient wclient;
PubSubClient client(wclient); // Setup MQTT client

void sendButttonsConfiguration() {
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    String topic = buttonsPrefix + String(i+1);
    String json = "{\"~\":\"" + topic + "\",\"stat_t\":\"~/state\",\"name\":\"Controller Button " + String(i+1) + "\"}";
    client.publish((topic + "/config").c_str(), json.c_str());
  }
}

void sendLightsConfiguration() {
  String topic = lightsPrefix + "0";
  String json = "{\"~\":\"" + topic + "\",\"cmd_t\":\"~/set\",\"stat_t\":\"~/state\",\"name\":\"Controller Backlight\"}";
  client.publish((topic + "/config").c_str(), json.c_str());
  client.subscribe((topic + "/set").c_str());

  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    String topic = lightsPrefix + String(i+1);
    String json = "{\"~\":\"" + topic + "\",\"cmd_t\":\"~/set\",\"stat_t\":\"~/state\",\"name\":\"Controller LED " + String(i+1) + "\"}";
    client.publish((topic + "/config").c_str(), json.c_str());
    client.subscribe((topic + "/set").c_str());
  }
}

void sendConfiguration() {
  sendButttonsConfiguration();
  sendLightsConfiguration();
}

// Reconnect to client
void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    D(F("Attempting MQTT connection... "));
    // Attempt to connect
    if (client.connect(NAME, HA_USER, HA_PASS)) {
      client.subscribe(statusTopic.c_str());
      sendConfiguration();
      DL(F("connected"));
    } else {
      DL(F(" failed. Will try again soon"));
    }
  }
}

// Handle incomming messages from the broker
void callback(char* t, byte* p, unsigned int length) {
  String payload;
  String topic(t);

  for (int i = 0; i < length; i++) {
    payload += (char)p[i];
  }
  
  D(F("Message arrived ["));
  D(topic);
  D(F("] "));
  DL(payload);

  if (topic.startsWith(lightsPrefix)) {
    long led = topic.substring(lightsPrefix.length()).toInt();
    D(F("LED ")); DL(led);
    if (led == 0) {
      switchLight(payload == "ON");
    } else if (led >= 1 && led <= NUM_KEYS) {
      setLED(led - 1, (payload == "ON"));
    }
    String stateTopic = lightsPrefix + String(led) + "/state";
    client.publish(stateTopic.c_str(), payload.c_str());
  } else if (topic == statusTopic && payload == "online") {
    sendConfiguration();
  }
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

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(NAME);
  ArduinoOTA.begin();

  client.setServer(IPAddress(192, 168, 1, 162), 1883);
  client.setCallback(callback);// Initialize the callback routine

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

  DL(F("setup done"));
}

long nextTFTUpdate = 0;
long nextBatteryUpdate = 0;

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
  }
  
  reconnect();
  client.loop();

  // Formula from http://cuddletech.com/?p=1030
  battery = ((float)(analogRead(BATTERY_PIN)) / 4095) * 2.0 * 3.3 * 1.1;
  power = ((float)(analogRead(POWER_PIN)) / 4095) * 2.0 * 3.3 * 1.1;

  if (millis() > nextBatteryUpdate) {
    nextBatteryUpdate = millis() + 5000;
//    batteryLevel.setValue({.number = battery});
//    powerLevel.setValue({.number = power});
  }

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
      if (justPressed(i) || justReleased(i)) {
        String topic = buttonsPrefix + String(i+1) + "/state";
        client.publish(topic.c_str(), justPressed(i) ? "ON" : "OFF");
        D(F("Published to ")); DL(topic);
      }
    }
  }

  ArduinoOTA.handle();

//  bool on = ledOn.getValue().boolean;
//  if (on != light) {
//    switchLight();
//  }

  
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
  switchLight(!light);
}

void switchLight(bool turnOn) {
  light = turnOn;
  needTrellisWrite = true;
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

  img.print("MQTT ");
  if (!client.connected()) {
    img.print("dis");
  }
  img.println("connected!!");
}

void setLED(uint8_t n, bool onOff) {
  if (n >= NUM_KEYS) return;
  if (leds[n] != onOff) {
    leds[n] = onOff;
    needTrellisWrite = true;
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
