/*
 * Sparkfun ESP32 Thing
 */

#include <WiFiMulti.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#define LED     5

#define TX2     25
#define RX2     26

#define WLAN_AGNES

#include <Passwords.h>

#define NAME    "Parking"

#define DEBUG

#ifdef DEBUG

#define D(d)  Serial.print(d)
#define DL(d) Serial.println(d)
#define DP(args...) Serial.printf(args)
#define MARK  {Serial.print(F("Running line "));Serial.println(__LINE__);}

#else

#define D(d)  {}
#define DL(d) {}
#define DP(...) {}
#define MARK  {}

#endif

// defines variables

WiFiMulti wifiMulti;
WiFiClient client;

HardwareSerial SerialTFMini(2);

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.begin(115200); // Starts the serial communication
  DL(F(NAME));

  SerialTFMini.begin(115200, SERIAL_8N1, TX2, RX2);

  wifiMulti = WiFiMulti();
  wifiMulti.addAP(WLAN_SSID, WLAN_PASS);

  if (MDNS.begin(NAME)) {
    DL(F("MDNS responder started"));
  }

  DL(F("started"));
}

int distanceCM = 0;
int lastDistance = 0;
int strength = 0;

bool sendingDistance = false;
unsigned long nextSend = 0;

void loop() {
  bool wifiConnected = wifiMulti.run() == WL_CONNECTED;
  if (!wifiConnected) {
    DL(F("WiFi not connected!"));
  } else {
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(NAME);
    ArduinoOTA.begin();
  }

  if (wifiConnected) {
    if (!client.connected()) {
      DL(F("connecting to server."));
      client.stop();
      if (client.connect(WiFi.gatewayIP(), 5000)) {
        DL(F("connected to server."));
        client.setTimeout(15);
        client.println(NAME);
        client.flush();
        sendingDistance = false;
      } else {
        DL(F("failed connecting to server."));
      }
    } else if (client.available()) {
      String line = client.readStringUntil('\n');
      sendingDistance = (line[0] == '+');
      nextSend = 0;
    } else if (sendingDistance && millis() >= nextSend) {
      String command = "D" + String(lastDistance) + "\n";
      client.print(command);
      client.flush();
      nextSend = millis() + 500;
    }
    ArduinoOTA.handle();
  } else {
    client.stop();
    sendingDistance = false;
  }
  
  distanceCM = 0;
  getTFminiData(&distanceCM, &strength);
  
  if (distanceCM && lastDistance != distanceCM) {
    lastDistance = distanceCM;
    DL(lastDistance);
  }
}

// From https://github.com/TFmini/TFmini-Arduino#tfmini_arduino_hardware_serialpolling
void getTFminiData(int* distance, int* strength) {
  static uint8_t i = 0;
  static int rx[9];
  if (SerialTFMini.available()) {
    rx[i] = SerialTFMini.read();
    if (rx[0] != 0x59) {
      i = 0;
    } else if (i == 1 && rx[1] != 0x59) {
      i = 0;
    } else if (i == 8) {
      int checksum = 0;
      for (uint8_t j = 0; j < 8; j++) {
        checksum += rx[j];
      }
      if (rx[8] == (checksum & 0xFF)) {
        *distance = rx[2] + rx[3] * 256;
        *strength = rx[4] + rx[5] * 256;
      }
      i = 0;
    } else {
      i++;
    } 
  }  
}
