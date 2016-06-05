#define DEVICE_NAME   "star_switch"
#define SWITCH_PIN    0

//#define PRINT_DEBUG_MESSAGES 1

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define WLAN_GUSTAVO

#include "Passwords.h"

#define PI 3.141592653589793

WiFiClient  client;
ESP8266WebServer server(80);

typedef std::function<void(void)> ChangeHandler;

ChangeHandler handlerFunction = NULL;

bool isOn = false;

uint8_t pulsePeriod = 1;

void setup() {
  digitalWrite(SWITCH_PIN, HIGH);
  pinMode(SWITCH_PIN, OUTPUT);

  Serial.begin(115200);  

  Serial.print("Last reset reason: ");
  Serial.println(ESP.getResetInfo());
  
  Serial.println("Connecting");

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin (DEVICE_NAME)) {
    Serial.println("MDNS responder started");
  }

  server.on("/", []() {
    server.send (200, "text/plain", isOn ? "ON" : "OFF");
  });
  server.on("/on", []() {
    analogWrite(SWITCH_PIN, 1023);
    isOn = true;
    handlerFunction = NULL;
    server.send (200, "text/plain", "ON");
  });
  server.on("/off", []() {
    analogWrite(SWITCH_PIN, 0);
    isOn = false;
    handlerFunction = NULL;
    server.send (200, "text/plain", "OFF");
  });
  server.on("/pwm", []() {
    int speed = 0;
    if (server.hasArg("s")) {
      speed = constrain(server.arg("s").toInt(), 0, 1023);
    } 
    analogWrite(SWITCH_PIN, speed);
    handlerFunction = NULL;
    server.send (200, "text/plain", "OK");
  });
  server.on("/pulse", []() {
    pulsePeriod = 1;
    if (server.hasArg("p")) {
      pulsePeriod= constrain(server.arg("p").toInt(), 0, 100);
    } 
    isOn = true;
    handlerFunction = pulse;
    server.send (200, "text/plain", "OK");
  });
  server.begin();
  Serial.println ("HTTP server started");

  digitalWrite(SWITCH_PIN, LOW);
}

void loop() {
  server.handleClient();
  if (handlerFunction) {
    handlerFunction();
  }
}

void pulse() {
  // Level is between 0 and 2
  float level = 1 + sin((2 * PI) * (float)(millis()) / (float)(pulsePeriod * 1000));
  // intLevel will be betweel 0 and 1023
  int intLevel = 512 * level;
  analogWrite(SWITCH_PIN, intLevel);
}

