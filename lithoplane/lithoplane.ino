/***************************************************
 ****************************************************/
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>

//#define MARK  {Serial.print(millis());Serial.print(' ');Serial.print("Running line ");Serial.println(__LINE__);}
#define MARK  {}

#define LED_PIN       0

#define BUTTON_PIN    2
#define BUTTON_GROUND 14

Adafruit_NeoPixel strip = Adafruit_NeoPixel(44, LED_PIN, NEO_GRB + NEO_KHZ800);

#define LISTEN_PORT           80      // What TCP port to listen on for connections.  
                                      // The HTTP protocol uses port 80 by default.

ESP8266WebServer httpServer(LISTEN_PORT);

#define TIME_URL     F("http://trans-anchor-110020.appspot.com/t")

#define WLAN_GUSTAVO 1

#include "Passwords.h"

#define SLEEP_TIME          (8 * 60000)
#define WAKE_SECONDS        (10 * 60)
#define HOLD_TIME           1500

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

bool last_sensor_state;
long last_switch_time;
long on_time;
int number_of_taps;
bool hold;
bool hold_triggered;
int number_of_taps_triggered;

#define BOUNCE_TIME       30
#define SINGLE_TAP_TIME   200

long sleep_start;

long wake_second;

#define RAINBOW_DIVIDER     4
#define MAX_RAINBOW_INDEX   (256*5*RAINBOW_DIVIDER)

uint16_t rainbow_index;

uint32_t last_color;
uint16_t last_brightness;
bool should_rainbow;
bool should_off;

#define EEPROM_COLOR         0
#define EEPROM_BRIGHTNESS    (EEPROM_COLOR + sizeof(last_color))
#define EEPROM_RAINBOW       (EEPROM_BRIGHTNESS + sizeof(last_brightness))
#define EEPROM_OFF           (EEPROM_RAINBOW + sizeof(should_rainbow))
#define EEPROM_WAKE          (EEPROM_OFF + sizeof(should_off))
#define EEPROM_MAX           (EEPROM_WAKE + sizeof(long))

long real_second;
unsigned long real_second_start;

void setup(void)
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
  ESP.wdtDisable();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_GROUND, OUTPUT);
  digitalWrite(BUTTON_GROUND, LOW);
  
  wake_second = -1;

  EEPROM.begin(EEPROM_MAX);
  EEPROM.get(EEPROM_COLOR, last_color);
  EEPROM.get(EEPROM_BRIGHTNESS, last_brightness);
  EEPROM.get(EEPROM_RAINBOW, should_rainbow);
  EEPROM.get(EEPROM_OFF, should_off);
//  EEPROM.get(EEPROM_WAKE, wake_second);

  strip.begin();
  strip.setBrightness(10);
  colorWipe(strip.Color(255, 0, 0));
  
  // Initialise the module
  Serial.println(F("Init"));
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if (MDNS.begin("litho")) {
    Serial.println("MDNS responder started");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
   
  // Start listening for connections
  httpServer.on("/i", [](){
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/p", [](){
    resetModes();
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/b", [](){
    should_off = false;
    resetModes();
    last_brightness = httpServer.arg(0).toInt();
    updateEeprom();
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/c", [](){
    should_rainbow = false;
    should_off = false;
    resetModes();
    last_color = httpServer.arg(0).toInt();
    updateEeprom();
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/r", [](){
    should_rainbow = true;
    should_off = false;
    resetModes();
    updateEeprom();
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/w", [](){
//    wake_second = httpServer.arg(0).toInt();
    updateEeprom();
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/s", []() {
    httpServer.send(200, "text/plain", ESP.getResetInfo());
  });
  httpServer.begin();
  
  Serial.println(F("\nListening"));
  colorWipe(strip.Color(0, 0, 255));
  delay(1000);

  String payload = grabWebPage(TIME_URL);
  if (payload) {
    real_second = payload.toInt();
  } else {
    real_second = 0;
  }
  real_second_start = millis();
}

void loop(void)
{
  MARK
  ESP.wdtEnable(WDTO_8S);
  long millis_diff = millis() - real_second_start;
  if (millis_diff >= 1000) {
    real_second += millis_diff / 1000;
    real_second_start += (long)(millis_diff / 1000) * 1000;
  }
  MARK
  httpServer.handleClient();
  MARK
  bool sensor_state = digitalRead(BUTTON_PIN) == LOW;

  //first pressed
  if (sensor_state && !last_sensor_state) {
    on_time = millis();
  }
  
  //held
  if (sensor_state && last_sensor_state) {
    if ((millis() - on_time) > HOLD_TIME && !hold) {
      hold = true;
      hold_triggered = true;
    }
  }
  
  //released
  if (!sensor_state && last_sensor_state) {
    if (hold) {
      hold = false;
      number_of_taps = 0;
    } else if ((millis() - on_time) > BOUNCE_TIME) {
      number_of_taps++;
    }
    last_switch_time = millis();
  }
  
  if (!sensor_state && !last_sensor_state) {
    if ((millis() - last_switch_time) > SINGLE_TAP_TIME && number_of_taps > 0) {
      number_of_taps_triggered = number_of_taps;
      number_of_taps = 0;
    }
  }
  last_sensor_state = sensor_state;
  
  if (hold_triggered) {
    hold_triggered = false;
    should_off = true;
    resetModes();
    updateEeprom();
  } else if (number_of_taps_triggered > 0) {
    resetModes();
    if (should_off) {
      should_off = false;
      updateEeprom();
    } else {
      if (number_of_taps_triggered == 1) {
        should_rainbow = !should_rainbow;
        updateEeprom();
      } else if (number_of_taps_triggered == 2) {
        sleep_start = millis();
      }
    }
    number_of_taps_triggered = 0;
  }

  if (should_off) {
    uint8_t brightness = 0;
    if (wake_second >= 0) {
      long current_second = real_second;
      if (current_second >= wake_second) {
        should_off = false;
        wake_second = -1;
        brightness = last_brightness;
        updateEeprom();
      } else {
        long start_wake = wake_second - WAKE_SECONDS;
        if (start_wake < 0) {
          start_wake = wake_second;
          current_second = (current_second + WAKE_SECONDS) % (24 * 3600);
        }
        if (current_second > start_wake) {
          long elapsed = current_second - start_wake;
          brightness = map(elapsed, 0, WAKE_SECONDS, 0, last_brightness);
        }
      }
    }
    strip.setBrightness(brightness);
    colorWipe(strip.Color(255, 255, 255));
  } else if (sleep_start > 0) {
    long elapsed = millis() - sleep_start;
    if (elapsed >= SLEEP_TIME) {
      sleep_start = 0;
      should_off = true;
      updateEeprom();
    } else {
      uint8_t brightness = map(elapsed, 0, SLEEP_TIME, last_brightness, 0);
      strip.setBrightness(brightness);
      colorWipe(last_color);
    }
  } else if (should_rainbow) {
    strip.setBrightness(last_brightness);
    rainbowCycle(rainbow_index);
    if (++rainbow_index >= MAX_RAINBOW_INDEX) {
      rainbow_index = 0;
    }
  } else {
    strip.setBrightness(last_brightness);
    colorWipe(last_color);
  }
  MARK
}

void updateEeprom() {
  ESP.wdtFeed();
  EEPROM.put(EEPROM_COLOR, last_color);
  EEPROM.put(EEPROM_BRIGHTNESS, last_brightness);
  EEPROM.put(EEPROM_RAINBOW, should_rainbow);
  EEPROM.put(EEPROM_OFF, should_off);
  EEPROM.put(EEPROM_WAKE, wake_second);
  EEPROM.commit();
}

void resetModes() {
  sleep_start = 0;
}

String grabWebPage(const __FlashStringHelper *url) {
  HTTPClient http;
  String payload;
  
  ESP.wdtFeed();
  http.begin(url); //HTTP
  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
    }
  }
  ESP.wdtFeed();
  http.end();
  return payload;
}

long parseInt(const char *buffer) {
  long ret = 0;
  while (*buffer >= '0' && *buffer <= '9') {
    ret = ret * 10 + (*buffer++ - '0');
  }
  return ret;
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  ESP.wdtFeed();
  strip.show();
  ESP.wdtFeed();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(strip.numPixels() - i - 1, Wheel(((i * 256 / strip.numPixels()) + j / RAINBOW_DIVIDER) & 255));
  }
  ESP.wdtFeed();
  strip.show();
  ESP.wdtFeed();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
