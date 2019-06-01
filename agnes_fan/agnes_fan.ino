// Board: Adafruit Feather ESP8266

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <Bounce2.h>

#define MOTOR_UP   15
#define MOTOR_DOWN 14

#define LED 0

#define KEY_UP     4
#define KEY_DOWN   5

#define TOTAL_MOVE_TIME   6000

#define WLAN_AGNES

#include "Passwords.h"

ESP8266WebServer server(80);

Bounce debouncerDown = Bounce(); 
Bounce debouncerUp = Bounce(); 

int goToPosition = 0;
int currentPosition = 0;

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
  Serial.println("Starting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }

  if (MDNS.begin("fan")) {
    Serial.println("MDNS responder started");
  }

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("fan");
  ArduinoOTA.begin();

  server.on("/", []() {
    if (!server.authenticate(WLAN_SSID, WLAN_PASS)) {
      return server.requestAuthentication();
    }
    server.send(200, "text/plain", "Login OK");
  });
  server.on("/move", []() {
    if (!server.authenticate(WLAN_SSID, WLAN_PASS)) {
      return server.requestAuthentication();
    }
    int pos = parseInt(server.arg("pos").c_str());
    goToPosition = pos;
    server.send(200, "text/plain", "Move OK");
  });
  server.begin();

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser to see it working");
  digitalWrite(LED, HIGH);
}

bool goingUp = false;
bool goingDown = false;
long startMove;

void loop() {
  debouncerDown.update();
  debouncerUp.update();

  if (currentPosition != goToPosition) {
    long moveTime = (goToPosition < 0 ? 1000 : 0) + TOTAL_MOVE_TIME * abs(currentPosition - max(0, goToPosition)) / 100;
    bool goingUp = goToPosition > currentPosition;
    Serial.print("Moving ");
    Serial.print(goingUp ? "up for " : "down for ");
    Serial.println(moveTime);

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

    currentPosition = max(0, goToPosition);
    goToPosition = currentPosition;
  }
  
  bool goUp = debouncerUp.read() == LOW;
  bool goDown = debouncerDown.read() == LOW;
  if (goUp && goDown) {
    Serial.println("Can't go both ways...");
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
        currentPosition = min((long)100, currentPosition + 100 * moveTime / TOTAL_MOVE_TIME);
        goToPosition = currentPosition;
      }
      Serial.println("Going up changed");
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
        currentPosition = max((long)0, currentPosition - 100 * moveTime / TOTAL_MOVE_TIME);
        goToPosition = currentPosition;
      }
      Serial.println("Going down changed");
    }
  }

  ArduinoOTA.handle();
  server.handleClient();
}

int parseInt(const char *buffer) {
  int ret = 0;
  bool isNegative = false;
  if (*buffer == '-') {
    isNegative = true;
    buffer++;
  }
  while (*buffer >= '0' && *buffer <= '9') {
    ret = ret * 10 + (*buffer++ - '0');
  }
  return isNegative? -ret : ret;
}
