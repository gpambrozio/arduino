/*************************************************** 
 *  Agnes' keypad using adafruit's feather ESP8266
 ****************************************************/

#include <Wire.h>
#include "Adafruit_Trellis.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define WLAN_AGNES

#include <Passwords.h>

ESP8266WebServer server(80);
HTTPClient http;

Adafruit_Trellis matrix0 = Adafruit_Trellis();

// Just one
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define numKeys (NUMTRELLIS * 16)

// Connect Trellis Vin to 5V and Ground to ground.
#define INTPIN 2
// Connect I2C SDA pin to your Arduino SDA line
// Connect I2C SCL pin to your Arduino SCL line
// All Trellises share the SDA, SCL and INT pin! 
// Even 8 tiles use only 3 wires max

// TTF Config
// ==========
// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     15
#define TFT_RST    0   // you can also connect this to the Arduino reset
                       // in which case, set this #define pin to -1!
#define TFT_DC     16
#define TFT_LIGHT  12

// For 1.44" and 1.8" TFT with ST7735 use
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);


void setup() {
  Serial.begin(115200);
  Serial.println("Keypad");

  // INT pin requires a pullup
  pinMode(INTPIN, INPUT_PULLUP);
  digitalWrite(INTPIN, HIGH);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Rebooting...");
    delay(1000);
    ESP.restart();
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
  analogWrite(TFT_LIGHT, 1023);

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

  tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
}

long nextTFTUpdate = 0;
int light = 1023;

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

        if (i == 3) {
          light = min(1023, light + 128);
          analogWrite(TFT_LIGHT, light);
        } else if (i == 15) {
          light = max(0, light - 128);
          analogWrite(TFT_LIGHT, light);
        } else {
          if (i % 2 == 0) {
            http.begin("http://agnespanel.local:8080/text//0");
          } else {
            http.begin("http://agnespanel.local:8080/image/thanks/1");
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
  tft.setTextWrap(false);
  tft.startWrite();
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  tft.print(millis() / 1000);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print(" seconds.");
  tft.endWrite();
}
