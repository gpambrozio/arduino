/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#define WLAN_GUSTAVO

#include "Passwords.h"

#define BART_CHECK_PERIOD   (30 * 1000)
#define BART_MAX_CHECK      (10 * 60000)

#define ESP8266_LED 5

const char* host = "trans-anchor-110020.appspot.com";

uint8_t const sin_table[] PROGMEM={
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,
  79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,
  37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,
  10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,
  0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,
  10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,
  37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,
  79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124,
};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, 0, NEO_GRB + NEO_KHZ800);

uint32_t next_bart_check;
uint32_t max_bart_check;
uint8_t next_bart_time;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(ESP8266_LED, OUTPUT);
  digitalWrite(ESP8266_LED, HIGH);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  next_bart_check = millis();
  max_bart_check = next_bart_check + BART_MAX_CHECK;
}

void grabBartTimes() {
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/b?w=g";
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  client.flush();
  delay(1000);
  
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()){
    String line = client.readStringUntil('\n');
    // Empty line, end of headers.
    if (line == "\r") {
      next_bart_time = client.parseInt();
      Serial.print("Bart: "); Serial.println(next_bart_time); 
    }
  }
  
  Serial.println();
  Serial.println("closing connection");
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
  }
}

void loop() {
  if (max_bart_check > 0 && millis() >= next_bart_check) {
    if (millis() >= max_bart_check) {
      max_bart_check = 0;
      next_bart_time = 0;
    } else {
      next_bart_check = millis() + BART_CHECK_PERIOD;
      grabBartTimes();
    }
  }

  if (next_bart_time > 0) {
    long next_bart_time_millis = 1000 * next_bart_time;
    long cycle_position = sizeof(sin_table) * (millis() % next_bart_time_millis) / next_bart_time_millis;
    uint8_t brightness = pgm_read_byte(&sin_table[cycle_position]);
    digitalWrite(ESP8266_LED, brightness>128?HIGH:LOW);
    strip.setBrightness(brightness);
    colorWipe(strip.Color(0, 0, 255));
    strip.show();
  } else {
    digitalWrite(ESP8266_LED, HIGH);
  }
}


