
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
#include "utility/debug.h"
#include "utility/socket.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  9
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

#define LED_PIN   7

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

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "www.adafruit.com"
#define WEBPAGE      F("/testwifi/index.html")

Adafruit_CC3000_Server httpServer(LISTEN_PORT);
uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char *action;
char *path;

uint32_t ip;

bool should_grab_bart_times;
uint16_t rainbow_index;

#define MAX_RAINBOW_INDEX   (256*5)

uint32_t last_color;
uint16_t last_brightness;
bool should_rainbow;

#define EEPROM_COLOR         0
#define EEPROM_BRIGHTNESS    (EEPROM_COLOR + sizeof(last_color))
#define EEPROM_RAINBOW       (EEPROM_BRIGHTNESS + sizeof(last_brightness))

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

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

  Serial.print(F("Free RAM: ")); Serial.println(getFreeRam(), DEC);
  
  // Initialise the module
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  // Display the IP address DNS, Gateway, etc.
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  // Start listening for connections
  httpServer.begin();
  
  Serial.println(F("Listening for connections..."));
}

void loop(void)
{
  if (Serial.available()) {  // Look for char in serial que and process if found
    int command = Serial.read();
    Serial.print(F("Command: "));
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
      case 'M':
        Serial.print(F("Free RAM: ")); Serial.println(getFreeRam(), DEC);
        break;
        
      case 'W':
        grabBartTimes();
        break;
    }
  }
  
  if (should_grab_bart_times) {
    should_grab_bart_times = false;
    grabBartTimes();
  }
  
  if (should_rainbow) {
    rainbowCycle(rainbow_index);
    if (++rainbow_index >= MAX_RAINBOW_INDEX) {
      rainbow_index = 0;
    }
  }

  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println(F("Client connected."));
    // Process this request until it completes or times out.
    // Note that this is explicitly limited to handling one request at a time!

    // Clear the incoming data buffer and point to the beginning of it.
    bufindex = 0;
    memset(&buffer, 0, sizeof(buffer));
    
    // Set a timeout for reading all the incoming data.
    unsigned long endtime = millis() + TIMEOUT_MS;
    
    // Read all the incoming data until it can be parsed or the timeout expires.
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      parsed = parseRequest(buffer, bufindex, &action, &path);
    }

    // Handle the request if it was parsed.
    if (parsed) {
      Serial.println(F("Processing request"));
      Serial.print(F("Action: ")); Serial.println(action);
      Serial.print(F("Path: ")); Serial.println(path);
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
            client.fastrprintln(F("lithoplane here"));
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
            last_brightness = String(path+1).toInt();
            EEPROM.put(EEPROM_BRIGHTNESS, last_brightness);
            strip.setBrightness(last_brightness);
            colorWipe(last_color);
            strip.show();
            client.fastrprintln(F("Brightness set"));
            break;
            
          case 'c':    // Color
            should_rainbow = false;
            last_color = String(path+1).toInt();
            EEPROM.put(EEPROM_COLOR, last_color);
            EEPROM.put(EEPROM_RAINBOW, should_rainbow);
            colorWipe(last_color);
            strip.show(); 
            break;
            
          case 'r':    // Rainbow
            should_rainbow = true;
            EEPROM.put(EEPROM_RAINBOW, should_rainbow);
            break;
            
          case 'B':    // Bart
            // Can' grab bart times cause WiFi in use now. Do it later.
            should_grab_bart_times = true;
            break;
        }
        
        // Now send the response data.
        client.fastrprintln(F("OK"));
      }
      else {
        // Unsupported action, respond with an HTTP 405 method not allowed error.
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F(""));
      }
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(100);

    // Close the connection when done.
    Serial.println(F("Client disconnected"));
    client.close();
  }
}

void grabBartTimes() {
  ip = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
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
    www.fastrprint(WEBPAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }

  bufindex = 0;
  memset(&buffer, 0, sizeof(buffer));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      buffer[bufindex++] = c;
      Serial.print(c);
      lastRead = millis();
    }
  }
  buffer[bufindex] = 0;
  www.close();
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
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  } else {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.println();
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

