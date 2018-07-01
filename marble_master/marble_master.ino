/*
*/

#define MOTOR_PIN    13
#define LED_PIN      14

//#define PRINT_DEBUG_MESSAGES 1

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
extern "C" {
  #include "user_interface.h"
}
#include <Adafruit_NeoPixel.h>


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, LED_PIN, NEO_GRB + NEO_KHZ800);

#define WLAN_GUSTAVO

#include "Passwords.h"

ESP8266WebServer server(80);

void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  Serial.begin(115200);
  Serial.println("Connecting");
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  strip.begin();
  uint16_t i=0;
  for(; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(100, 100, 100));
  }
  strip.show();

  i=0;
  while (WiFi.status() != WL_CONNECTED) {
    strip.setPixelColor(i++, 0);
    strip.show();
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin ("marble")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", []() {
    int speed = 0;
    if (server.hasArg("s")) {
      speed = server.arg("s").toInt();
    }
    analogWrite(MOTOR_PIN, speed);
    server.send (200, "text/plain", "OK");
  });
  server.on("/r", []() {
    server.send (200, "text/plain", ESP.getResetInfo());
  });
  server.on("/t", []() {
    server.send (200, "text/plain", String(millis()/1000));
  });
  server.begin();
  Serial.println ("HTTP server started");

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
}

void loop() {
  server.handleClient();
  strip.show();
}

