/*******************************
 * Sparkfun ESP32 Thing
 *******************************/

#define PIN_UP   12
#define PIN_DN   13

#define SWITCH_UP  17
#define SWITCH_DN  16

#define TOTAL_MOVE_TIME   (9500.0f)

#define LED     5

#include <WiFi.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Bounce2.h>
#include <vector>

#define WLAN_AGNES

#include <Passwords.h>

#define NAME    "Couch"

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

WiFiClient client;

long goToPosition = 0;
long currentPosition = 0;

Bounce debouncerDn = Bounce(); 
Bounce debouncerUp = Bounce(); 

std::vector<String> commandsToSend;

void setup() {
  pinMode(SWITCH_UP, INPUT_PULLUP);
  pinMode(SWITCH_DN, INPUT_PULLUP);
  
  debouncerDn.attach(SWITCH_DN);
  debouncerDn.interval(50); // interval in ms

  debouncerUp.attach(SWITCH_UP);
  debouncerUp.interval(50); // interval in ms

  pinMode(PIN_UP, OUTPUT);
  pinMode(PIN_DN, OUTPUT);
  digitalWrite(PIN_UP, HIGH);
  digitalWrite(PIN_DN, HIGH);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.begin(115200); // Starts the serial communication
  DL(F(NAME));

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  int failCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++failCount > 10) {
      ESP.restart();
    }
  }

  if (MDNS.begin(NAME)) {
    DL(F("MDNS responder started"));
  }

  DL(F("started"));
}

bool goingUp = false;
bool goingDown = false;
long startMove;
long lastReported = -1;

void loop() {
  debouncerDn.update();
  debouncerUp.update();

  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (!wifiConnected) {
    DL(F("WiFi not connected!"));
    ESP.restart();
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
        commandsToSend.clear();
        client.setTimeout(15);
        client.println(NAME);
        client.flush();
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
  } else {
    client.stop();
  }
  
  goToPosition = min(100l, goToPosition);
  if (currentPosition != goToPosition) {
    long moveTime = goToPosition < 0 ? 1000 : (long)((float)(abs(currentPosition - max(0l, goToPosition))) / 100.0f * TOTAL_MOVE_TIME);
    moveTime += (goToPosition == 0 || goToPosition == 100) ? 1000 : 0;
    bool goingUp = goToPosition > currentPosition;
    D(F("Moving "));
    D(goingUp ? "up for " : "down for ");
    DL(moveTime);

    if (goingUp) {
      digitalWrite(PIN_UP, LOW);
      digitalWrite(PIN_DN, HIGH);
    } else {
      digitalWrite(PIN_UP, HIGH);
      digitalWrite(PIN_DN, LOW);
    }
    delay(moveTime);
    digitalWrite(PIN_UP, HIGH);
    digitalWrite(PIN_DN, HIGH);

    currentPosition = max(0l, goToPosition);
    goToPosition = currentPosition;
  }
  
  bool goUp = debouncerUp.read() == LOW;
  bool goDown = debouncerDn.read() == LOW;
  if (goUp && goDown) {
    DL(F("Can't go both ways..."));
  } else {
    if (goingUp != goUp) {
      goingUp = goUp;
      if (goUp) {
        startMove = millis();
        digitalWrite(PIN_UP, LOW);
        digitalWrite(PIN_DN, HIGH);
      } else {
        digitalWrite(PIN_UP, HIGH);
        long moveTime = millis() - startMove;
        currentPosition = min(100l, (long)(currentPosition + 100 * moveTime / TOTAL_MOVE_TIME));
        goToPosition = currentPosition;
      }
      DL(F("Going up changed"));
    }
    if (goingDown != goDown) {
      goingDown = goDown;
      if (goDown) {
        startMove = millis();
        digitalWrite(PIN_DN, LOW);
        digitalWrite(PIN_UP, HIGH);
      } else {
        digitalWrite(PIN_DN, HIGH);
        long moveTime = millis() - startMove;
        currentPosition = max(0l, (long)(currentPosition - 100 * moveTime / TOTAL_MOVE_TIME));
        goToPosition = currentPosition;
      }
      DL(F("Going down changed"));
    }
  }
  
  if (lastReported != currentPosition) {
    addCommand("P:" + String(currentPosition));
    lastReported = currentPosition;
  }
}

void addCommand(String command) {
  commandsToSend.push_back(command);
}
