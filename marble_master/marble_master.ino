/*
*/

#define MOTOR_PIN    0

//#define PRINT_DEBUG_MESSAGES 1

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
extern "C" {
  #include "user_interface.h"
}

#define WLAN_GUSTAVO

#include "Passwords.h"

ESP8266WebServer server(80);

void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  Serial.begin(115200);

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
}

