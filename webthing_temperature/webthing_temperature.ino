/*************************************************** 
 *  Temp sensor with Sparkfun ESP32 thing
 ****************************************************/

#define DEBUG
#include <DebugMacros.h>

#include <WiFi.h>

#include <ArduinoOTA.h>

#define LARGE_JSON_BUFFERS 1
#define LARGE_JSON_DOCUMENT_SIZE (10*1024)
#define ARDUINOJSON_USE_LONG_LONG 1

#include <Thing.h>
#include <WebThingAdapter.h>

#define WLAN_WAIKIKI
#include <Passwords.h>

#define NAME  "temperature"

// The built in LED
#define LED 5

// Input from TMP36
// Datasheet: http://cdn.sparkfun.com/datasheets/Sensors/Temp/TMP35_36_37.pdf
// Sparkfun page: https://www.sparkfun.com/products/10988
#define TEMPERATURE  35

// WebThings things

WebThingAdapter *adapter;

const char *deviceTypes[] = {"TemperatureSensor", nullptr};
ThingDevice device("TemperatureSensor", "Internal Temperature", deviceTypes);
ThingProperty temperatureLevel("temperature", "", NUMBER, "TemperatureProperty");
long nextTemperatureUpdate = 0;

void thingsSetup() {
  adapter = new WebThingAdapter(NAME, WiFi.localIP());

  temperatureLevel.title = "Temperature";
  temperatureLevel.unit = "degree celsius";
  device.addProperty(&temperatureLevel);

  device.description = "A web connected temperature sensor";
  adapter->addDevice(&device);
  adapter->begin();
}

void setup() {
  Serial.begin(115200);
  DL(F(NAME));

  analogReadResolution(12);
  pinMode(TEMPERATURE, INPUT);
  adcAttachPin(TEMPERATURE);
  adcStart(TEMPERATURE);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  int failCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    D(F("."));
    if (++failCount > 10) {
      ESP.restart();
    }
  }

  thingsSetup();

  DL(F("setup done"));
}

void loop() {
  delay(30); // 30ms delay is required, dont remove me!

  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (!wifiConnected) {
    DL(F("WiFi not connected!"));
    ESP.restart();
  } else {
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(NAME);
    ArduinoOTA.begin();
  }

  long temperatureRead = analogRead(TEMPERATURE);
  double tempC = ((float)temperatureRead / 4095.0f) * 95.0 + 5.0;
  if (millis() > nextTemperatureUpdate) {
    nextTemperatureUpdate = millis() + 500;
    temperatureLevel.setValue({.number = tempC});
    D(F("Teperature: "));
    D(temperatureRead);
    D(F(" "));
    DL(tempC);
  }

  adapter->update();
  ArduinoOTA.handle();
}
