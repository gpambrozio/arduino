/*************************************************** 
 *  Agnes' keypad using adafruit's feather ESP32
 ****************************************************/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#include <Wire.h>
#include "Adafruit_Trellis.h"

#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library

#define WLAN_AGNES

#include <Passwords.h>

WiFiMulti wifiMulti;
WebServer server(80);

Adafruit_Trellis matrix0 = Adafruit_Trellis();

// Just one
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define numKeys (NUMTRELLIS * 16)

// Connect Trellis Vin to 5V and Ground to ground.
#define INTPIN 14

#define TFT_LIGHT  13
#define TFT_LIGHT_CHANNEL 0

// For 1.44" and 1.8" TFT with ST7735 use
TFT_eSPI tft = TFT_eSPI();


void setup() {

  Serial.begin(115200);
  Serial.println("Keypad");

  // INT pin requires a pullup
  pinMode(INTPIN, INPUT_PULLUP);
  digitalWrite(INTPIN, HIGH);

  wifiMulti.addAP(WLAN_SSID, WLAN_PASS);

  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Retrying...");
    delay(1000);
  }
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
    if (!server.authenticate(WLAN_SSID, WLAN_PASS)) {
      return server.requestAuthentication();
    }
    server.send(200, "text/plain", "Login OK");
  });
  server.begin();

  // begin() with the addresses of each panel in order
  // I find it easiest if the addresses are in order
  trellis.begin(0x70);  // only one

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
    delay(50);
  }
  // then turn them off
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.clrLED(i);
    trellis.writeDisplay();
    delay(50);
  }

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  Serial.println("setup done");
}

HTTPClient http;
long nextTFTUpdate = 0;
int light = 255;

void loop() {
  delay(30); // 30ms delay is required, dont remove me!

  // If a button was just pressed or released...
  if (trellis.readSwitches()) {
    // go through every button
    for (uint8_t i=0; i<numKeys; i++) {
      // if it was pressed...
      if (trellis.justPressed(i)) {
        Serial.printf("v%d\n", i);
        // Alternate the LED
        if (trellis.isLED(i)) {
          trellis.clrLED(i);
        } else {
          trellis.setLED(i);
        }

        if (i == 15) {
          light += 32;
          if (light > 255) light = 255;
          sigmaDeltaWrite(TFT_LIGHT_CHANNEL, light);
        } else if (i == 14) {
          light -= 32;
          if (light < 0) light = 0;
          sigmaDeltaWrite(TFT_LIGHT_CHANNEL, light);
        } else {
          if (i % 2 == 0) {
            http.begin("agnespanel", 8080, "/text//0");
          } else {
            http.begin("agnespanel", 8080, "/image/thanks/1");
          }
          
          int httpCode = http.GET();
          if (httpCode > 0) {
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
          } else {
            Serial.print("[HTTP] GET... failed, error: "); Serial.println(http.errorToString(httpCode).c_str());
          }
  
          http.end();
        }
      }
    }
    // tell the trellis to set the LEDs we requested
    trellis.writeDisplay();
  }
  ArduinoOTA.handle();
  server.handleClient();
  if (millis() > nextTFTUpdate) {
    nextTFTUpdate = millis() + 1000;
    tftPrintTest();
  }
}

void tftPrintTest() {
  tft.setCursor(0, 0, 1);
  tft.setTextColor(TFT_WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.print(millis() / 1000);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(" seconds.");
}
