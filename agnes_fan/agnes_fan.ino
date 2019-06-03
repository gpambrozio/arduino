// Board: Adafruit Feather ESP8266

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <Bounce2.h>
#include <vector>

#define MOTOR_UP   14
#define MOTOR_DOWN 15

#define LED 0

#define KEY_UP     5
#define KEY_DOWN   4

#define TOTAL_MOVE_TIME   9000

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

#define WLAN_AGNES

#include "Passwords.h"

ESP8266WebServer server(80);
WiFiClient client;
std::vector<String> commandsToSend;

Bounce debouncerDown = Bounce(); 
Bounce debouncerUp = Bounce(); 

long goToPosition = 0;
long currentPosition = 0;

void setup() {
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DOWN, INPUT_PULLUP);

  debouncerDown.attach(KEY_DOWN);
  debouncerDown.interval(50); // interval in ms

  debouncerUp.attach(KEY_UP);
  debouncerUp.interval(50); // interval in ms

  digitalWrite(MOTOR_UP, LOW);
  digitalWrite(MOTOR_DOWN, LOW);
  pinMode(MOTOR_UP, OUTPUT);
  pinMode(MOTOR_DOWN, OUTPUT);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.begin(115200);
  DL(F("Starting"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DL(F("WiFi Connect Failed! Rebooting..."));
    delay(1000);
    ESP.restart();
  }

  if (MDNS.begin("fan")) {
    DL(F("MDNS responder started"));
  }

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("fan");
  ArduinoOTA.begin();

  server.on("/", []() {
    String toSend = "Pos = " + currentPosition;
    server.send(200, "text/plain", toSend);
  });
  server.on("/move", []() {
    if (server.hasArg("pos")) {
      long pos = server.arg("pos").toInt();
      goToPosition = pos;
    } else if (server.hasArg("rel")) {
      long rel = server.arg("rel").toInt();
      goToPosition = currentPosition + rel;
    }
    server.send(200, "text/plain", "Move OK");
  });
  server.begin();

  D(F("Open http://"));
  D(WiFi.localIP());
  DL(F("/ in your browser to see it working"));
  digitalWrite(LED, HIGH);
}

bool goingUp = false;
bool goingDown = false;
long startMove;
long lastReported = -1;

void loop() {
  debouncerDown.update();
  debouncerUp.update();

  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (!wifiConnected) {
    DL(F("WiFi not connected!"));
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      wifiConnected = true;
    } else {
      DL(F("WiFi Connect Failed! Rebooting..."));
      delay(1000);
      ESP.restart();
    }
  }
  
  if (wifiConnected) {
    if (!client.connected()) {
      DL(F("connecting to server."));
      client.stop();
      if (client.connect(WiFi.gatewayIP(), 5000)) {
        lastReported = -1;
        commandsToSend.clear();
        DL(F("connected to server."));
        client.setTimeout(15);
        client.print("Fan\n");
      } else {
        DL(F("failed connecting to server."));
      }
    } else if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("P")) {
        goToPosition = line.substring(1).toInt();
      } else if (line.startsWith("R")) {
        goToPosition = currentPosition + line.substring(1).toInt();
      }
    } else if (!commandsToSend.empty()) {
      for (uint8_t i=0; i<commandsToSend.size(); i++) {
        client.print(commandsToSend[i] + "\n");
      }
      commandsToSend.clear();
    }
    ArduinoOTA.handle();
    server.handleClient();
  } else {
    client.stop();
  }

  goToPosition = min(100l, goToPosition);
  if (currentPosition != goToPosition) {
    long moveTime = goToPosition < 0 ? 1000 : (long)((float)abs(currentPosition - max(0l, goToPosition)) / 100 * TOTAL_MOVE_TIME);
    bool goingUp = goToPosition > currentPosition;
    D(F("Moving "));
    D(goingUp ? "up for " : "down for ");
    DL(moveTime);

    if (goingUp) {
      digitalWrite(MOTOR_UP, HIGH);
      digitalWrite(MOTOR_DOWN, LOW);
    } else {
      digitalWrite(MOTOR_UP, LOW);
      digitalWrite(MOTOR_DOWN, HIGH);
    }
    delay(moveTime);
    digitalWrite(MOTOR_UP, LOW);
    digitalWrite(MOTOR_DOWN, LOW);

    currentPosition = max(0l, goToPosition);
    goToPosition = currentPosition;
  }
  
  bool goUp = debouncerUp.read() == LOW;
  bool goDown = debouncerDown.read() == LOW;
  if (goUp && goDown) {
    DL(F("Can't go both ways..."));
  } else {
    if (goingUp != goUp) {
      goingUp = goUp;
      if (goUp) {
        startMove = millis();
        digitalWrite(MOTOR_UP, HIGH);
        digitalWrite(MOTOR_DOWN, LOW);
      } else {
        digitalWrite(MOTOR_UP, LOW);
        long moveTime = millis() - startMove;
        currentPosition = min(100l, currentPosition + 100 * moveTime / TOTAL_MOVE_TIME);
        goToPosition = currentPosition;
      }
      DL(F("Going up changed"));
    }
    if (goingDown != goDown) {
      goingDown = goDown;
      if (goDown) {
        startMove = millis();
        digitalWrite(MOTOR_DOWN, HIGH);
        digitalWrite(MOTOR_UP, LOW);
      } else {
        digitalWrite(MOTOR_DOWN, LOW);
        long moveTime = millis() - startMove;
        currentPosition = max(0l, currentPosition - 100 * moveTime / TOTAL_MOVE_TIME);
        goToPosition = currentPosition;
      }
      DL(F("Going down changed"));
    }
  }
  
  if (lastReported != currentPosition) {
    addCommand("P" + currentPosition);
    lastReported = currentPosition;
  }
}

void addCommand(String command) {
  commandsToSend.push_back(command);
}
