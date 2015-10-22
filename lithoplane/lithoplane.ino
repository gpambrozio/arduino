/***************************************************
  Adafruit CC3000 Breakout/Shield Simple HTTP Server
    
  This is a simple implementation of a bare bones
  HTTP server that can respond to very simple requests.
  Note that this server is not meant to handle high
  load, concurrent connections, SSL, etc.  A 16mhz Arduino 
  with 2K of memory can only handle so much complexity!  
  This server example is best for very simple status messages
  or REST APIs.

  See the CC3000 tutorial on Adafruit's learning system
  for more information on setting up and using the
  CC3000:
    http://learn.adafruit.com/adafruit-cc3000-wifi  
    
  Requirements:
  
  This sketch requires the Adafruit CC3000 library.  You can
  download the library from:
    https://github.com/adafruit/Adafruit_CC3000_Library
  
  For information on installing libraries in the Arduino IDE
  see this page:
    http://arduino.cc/en/Guide/Libraries
  
  Usage:
    
  Update the SSID and, if necessary, the CC3000 hardware pin 
  information below, then run the sketch and check the 
  output of the serial port.  After connecting to the 
  wireless network successfully the sketch will output 
  the IP address of the server and start listening for 
  connections.  Once listening for connections, connect
  to the server IP from a web browser.  For example if your
  server is listening on IP 192.168.1.130 you would access
  http://192.168.1.130/ from your web browser.
  
  Created by Tony DiCola and adapted from HTTP server code created by Eric Friedrich.
  
  This code was adapted from Adafruit CC3000 library example 
  code which has the following license:
  
  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution      
 ****************************************************/
#include <Adafruit_CC3000.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <RTC.h>
#include <EEPROM.h>
#include <CapacitiveSensor.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  9
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

#define LED_PIN       7

#define CAP1_IN       4
#define CAP1_SENSOR   5

CapacitiveSensor  cs1 = CapacitiveSensor(CAP1_IN, CAP1_SENSOR);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(44, LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "myNetwork"   // cannot be longer than 32 characters!
#define WLAN_PASS       "myPassword"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           80      // What TCP port to listen on for connections.  
                                      // The HTTP protocol uses port 80 by default.

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              64      // Maximum length of the HTTP request path that can be parsed.
                                      // There isn't much memory available so keep this short!

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
                                                          // Since only the first line is parsed this
                                                          // needs to be as large as the maximum action
                                                          // and path plus a little for whitespace and
                                                          // HTTP version.

#define TIMEOUT_MS            500    // Amount of time in milliseconds to wait for
                                     // an incoming request to finish.  Don't set this
                                     // too high or your server could be slow to respond.

#define IDLE_TIMEOUT_MS  1000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "trans-anchor-110020.appspot.com"

#define BART_URL     F("/bart?s=16th&d=n&t=6")

#define BART_CHECK_PERIOD   (30 * 1000)
#define BART_MAX_CHECK      (10 * 60000)

uint8_t const sin_table[] PROGMEM={
  64,66,67,69,70,72,73,75,76,78,80,81,83,84,86,87,
  88,90,91,93,94,96,97,98,100,101,102,103,105,106,107,108,
  109,110,111,112,113,114,115,116,117,118,119,120,120,121,122,123,
  123,124,124,125,125,126,126,126,127,127,127,128,128,128,128,128,
  128,128,128,128,128,128,127,127,127,126,126,126,125,125,124,124,
  123,123,122,121,120,120,119,118,117,116,115,114,113,112,111,110,
  109,108,107,106,105,103,102,101,100,98,97,96,94,93,91,90,
  88,87,86,84,83,81,80,78,76,75,73,72,70,69,67,66,
  64,62,61,59,58,56,55,53,52,50,48,47,45,44,42,41,
  40,38,37,35,34,32,31,30,28,27,26,25,23,22,21,20,
  19,18,17,16,15,14,13,12,11,10,9,8,8,7,6,5,
  5,4,4,3,3,2,2,2,1,1,1,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,2,2,2,3,3,4,4,
  5,5,6,7,8,8,9,10,11,12,13,14,15,16,17,18,
  19,20,21,22,23,25,26,27,28,30,31,32,34,35,37,38,
  40,41,42,44,45,47,48,50,52,53,55,56,58,59,61,62,
};

Adafruit_CC3000_Server httpServer(LISTEN_PORT);
uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char *action;
char *path;

bool last_sensor_state;
long last_switch_time;
long on_time;
int number_of_taps;
bool hold;
bool hold_triggered;
int number_of_taps_triggered;

#define SENSOR_THRESHOLD  60

#define BOUNCE_TIME       20
#define SINGLE_TAP_TIME   200
#define HOLD_TIME         1500

uint32_t next_bart_check;
uint32_t max_bart_check;
long next_bart_time;

uint16_t rainbow_index;

#define MAX_RAINBOW_INDEX   (256*5)

uint32_t last_color;
uint16_t last_brightness;
bool should_rainbow;
bool should_off;

#define EEPROM_COLOR         0
#define EEPROM_BRIGHTNESS    (EEPROM_COLOR + sizeof(last_color))
#define EEPROM_RAINBOW       (EEPROM_BRIGHTNESS + sizeof(last_brightness))

void setup(void)
{
  Serial.begin(115200);
  
  RTCStart();
  RTCGetDateDs1307();
  RTCPrintDateToSerial();

  EEPROM.get(EEPROM_COLOR, last_color);
  EEPROM.get(EEPROM_BRIGHTNESS, last_brightness);
  EEPROM.get(EEPROM_RAINBOW, should_rainbow);
  strip.begin();
  strip.setBrightness(last_brightness);
  colorWipe(last_color);
  strip.show(); 

  Serial.print(F("RAM:")); Serial.println(getFreeRam(), DEC);
  
  // Initialise the module
  Serial.println(F("Init"));
  if (!cc3000.begin()) {
    Serial.println(F("ERR1"));
    while(1);
  }
  
  Serial.print(F("Conn ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed"));
    while(1);
  }
   
  Serial.println(F("DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(1000); // ToDo: Insert a DHCP timeout!
  }  

  // Display the IP address DNS, Gateway, etc.
  while (!displayConnectionDetails()) {
    delay(1000);
  }

  // Start listening for connections
  httpServer.begin();
  
  Serial.println(F("\nListening"));
}

void loop(void)
{
  long cap_sensor =  cs1.capacitiveSensor(20);
  bool sensor_state = cap_sensor > SENSOR_THRESHOLD;

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
  } else if (number_of_taps_triggered > 0) {
    switch (number_of_taps_triggered) {
      case 1:
        if (should_off) {
          should_off = false;
        } else {
          startBartCheck();
        }
        break;
        
      case 2:
        should_rainbow = !should_rainbow;
        EEPROM.put(EEPROM_RAINBOW, should_rainbow);
        break;
    }
    number_of_taps_triggered = 0;
  }

  if (Serial.available()) {  // Look for char in serial que and process if found
    int command = Serial.read();
    Serial.print(F("cmd:"));
    Serial.println(command);  // Echo command CHAR in ascii that was sent
    switch (command) {
      case 'T':      //If command = "T" Set Date
        RTCSetDateDs1307();
        RTCGetDateDs1307();
        RTCPrintDateToSerial();
        Serial.println("");
        break;
        
      case 't':
        RTCGetDateDs1307();
        RTCPrintDateToSerial();
        Serial.println("");
        break;
        
      case 'm':
        Serial.print(F("RAM:")); Serial.println(getFreeRam(), DEC);
        break;
        
      case 'b':
        startBartCheck();
        break;
    }
  }
  
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
    strip.setBrightness(brightness/2);
    colorWipe(last_color);
    strip.show();
  } else if (should_off) {
    strip.setBrightness(0);
    colorWipe(0);
    strip.show();
  } else if (should_rainbow) {
    strip.setBrightness(last_brightness);
    rainbowCycle(rainbow_index);
    if (++rainbow_index >= MAX_RAINBOW_INDEX) {
      rainbow_index = 0;
    }
  } else {
    strip.setBrightness(last_brightness);
    colorWipe(last_color);
    strip.show();
  }

  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println(F("Client connected."));
    // Process this request until it completes or times out.
    // Note that this is explicitly limited to handling one request at a time!

    // Clear the incoming data buffer and point to the beginning of it.
    bufindex = 0;
    
    // Set a timeout for reading all the incoming data.
    unsigned long endtime = millis() + TIMEOUT_MS;
    
    // Read all the incoming data until it can be parsed or the timeout expires.
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      buffer[bufindex] = 0;
      parsed = parseRequest(buffer, bufindex, &action, &path);
    }

    // Handle the request if it was parsed.
    if (parsed) {
      Serial.println(F("Action:"));
      Serial.println(action); 
      Serial.println(path);
      // Check the action to see if it was a GET request.
      if (strcmp(action, "GET") == 0) {
        // Respond with the path that was accessed.
        // First send the success response code.
        client.fastrprintln(F("HTTP/1.1 200 OK"));
        // Then send a few headers to identify the type of data returned and that
        // the connection will not be held open.
        client.fastrprintln(F("Content-Type: text/plain"));
        client.fastrprintln(F("Connection: close"));
        // Send an empty line to signal start of body.
        client.fastrprintln(F(""));

        switch(path[0]) {
          case 'i':    // identify yourself!
            client.fastrprintln(F("hi"));
            break;

          case 't':    // Set time
            second = (byte) ((path[1] - '0') * 10 + (path[2] - '0')); 
            minute = (byte) ((path[3] - '0') *10 +  (path[4] - '0'));
            hour  = (byte) ((path[5] - '0') *10 +  (path[6] - '0'));
            dayOfWeek = (byte) (path[7] - '0');
            dayOfMonth = (byte) ((path[8] - '0') *10 +  (path[9] - '0'));
            month = (byte) ((path[10] - '0') *10 +  (path[11] - '0'));
            year= (byte) ((path[12] - '0') *10 +  (path[13] - '0'));
            RTCWriteDateToDs1307();
            client.fastrprintln(F("Date set"));
            break;
            
          case 'b':    // Brightness
            last_brightness = parseInt(path+1);
            EEPROM.put(EEPROM_BRIGHTNESS, last_brightness);
            client.fastrprintln(F("Brightness set"));
            break;
            
          case 'c':    // Color
            should_rainbow = false;
            last_color = parseInt(path+1);
            EEPROM.put(EEPROM_COLOR, last_color);
            EEPROM.put(EEPROM_RAINBOW, should_rainbow);
            break;
            
          case 'r':    // Rainbow
            should_rainbow = true;
            EEPROM.put(EEPROM_RAINBOW, should_rainbow);
            break;
        }
        
        // Now send the response data.
        client.fastrprintln(F("OK"));
      }
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(100);

    // Close the connection when done.
    client.close();
  }
}

void startBartCheck() {
  next_bart_check = millis();
  max_bart_check = next_bart_check + BART_MAX_CHECK;
}

void grabBartTimes() {
  if (grabWebPage(WEBSITE, BART_URL)) {
    next_bart_time = parseInt((char*)buffer);
    Serial.print(F("Next bart: ")); Serial.println(next_bart_time);
  }
}

bool grabWebPage(const char *site, const __FlashStringHelper *url) {
  uint32_t ip = 0;
  // Try looking up the website's IP address
  Serial.print(site); Serial.print(F(" -> "));
  while (ip == 0) {
    if (!cc3000.getHostByName(site, &ip)) {
      Serial.println(F("Couldn't resolve!"));
      return false;
    }
    delay(500);
  }

  cc3000.printIPdotsRev(ip);
  
  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(url);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(site); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    return false;
  }

  bufindex = 0;
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  bool receivingBody = false;
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available() && (bufindex < BUFFER_SIZE)) {
      char c = www.read();
      Serial.print(c);
      if (receivingBody || c == '\r' || c == '\n') {
        buffer[bufindex++] = c;
      } else {
        bufindex = 0;
      }
      if ((c == '\r' || c == '\n') && bufindex == 4) {
        bufindex = 0;
        receivingBody = true;
      }
      lastRead = millis();
    }
  }
  buffer[bufindex] = 0;
  www.close();
  return true;
}

int parseInt(const char *buffer) {
  int ret = 0;
  while (*buffer >= '0' && *buffer <= '9') {
    ret = ret * 10 + (*buffer++ - '0');
  }
  return ret;
}

// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool parseRequest(uint8_t* buf, int bufSize, char** action, char** path) {
  // Check if the request ends with \r\n to signal end of first line.
  if (bufSize < 2)
    return false;
  if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    parseFirstLine((char*)buf, action, path);
    return true;
  }
  return false;
}

// Parse the action and path from the first line of an HTTP request.
void parseFirstLine(char* line, char** action, char** path) {
  // Parse first word up to whitespace as action.
  char* lineaction = strtok(line, " ");
  if (lineaction != NULL)
    *action = lineaction;
  // Parse second word up to whitespace as path.
  char* linepath = strtok(NULL, " ");
  if (linepath != NULL)
    *path = linepath + 1; // Remove slash
}

// Tries to read the IP address and other connection details
bool displayConnectionDetails(void) {
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
    Serial.println(F("No IP"));
    return false;
  } else {
    Serial.print(F("IP: ")); cc3000.printIPdotsRev(ipAddress);
    return true;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
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

