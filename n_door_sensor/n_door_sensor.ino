/*
  ThingSpeak ( https://www.thingspeak.com ) is a free IoT service for prototyping
  systems that collect, analyze, and react to their environments.
*/

#define ENABLE_PIN    2

//#define PRINT_DEBUG_MESSAGES 1

#include "ThingSpeak.h"
#include <ESP8266WiFi.h>

#define WLAN_GUSTAVO

#include "Passwords.h"

#define myChannelNumber  80894

WiFiClient  client;

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);

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

  ThingSpeak.begin(client);
}

void loop() {
  // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  Serial.println("Writting");
  int result = ThingSpeak.writeField(myChannelNumber, 1, (long)millis(), N_DOOR_THINGSPEAK_ID);
  Serial.print("Result: ");
  Serial.println(result);
  pinMode(ENABLE_PIN, INPUT);
  ESP.deepSleep(30000000);
}

