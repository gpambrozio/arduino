/******************************************************************
 Circuit:
 WizFiShield connected to Arduino via SPI
 
 RST: pin 2  // Output
 DRDY: pin 3  // Input
 CSB: pin 4  // output

 MOSI: pin 11  // output
 MISO: pin 12  // input
 SCK: pin 13  // out

 Created 18 Sep. 2012
 by James YS Kim  (jameskim@wiznet.co.kr, javakys@gmail.com)
 
 Modified 27 May. 2013
 by Jinbuhm Kim  (jbkim@wiznet.co.kr, jinbuhm.kim@gmail.com)

*****************************************************************/

#include "SerialLCD.h"
#include <SoftwareSerial.h> //this is a must

// WizFiShield communicates using SPI, so include the SPI library:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SPI.h>
#include <WizFi2x0.h>
#include <WizFiClient.h>
#include <TimerOne.h>

byte tmpstr[80];

#define PROG_VAR_FUNC_CAST(S, FUNC, CAST)  { \
strcpy_P((char *)tmpstr, S); \
FUNC((CAST *)tmpstr); \
}

#define PROG_FUNC_CAST(S, FUNC, CAST)  { \
static prog_char PROG_VAR[] PROGMEM = S; \
PROG_VAR_FUNC_CAST(PROG_VAR, FUNC, CAST); \
}

#define PROG_FUNC(S, FUNC)  PROG_FUNC_CAST(S, FUNC, char)

#define sensorPin A0    // select the input pin for the potentiometer

int sensorValue = 0;  // variable to store the value coming from the sensor
float temperature;

// initialize the library
SerialLCD slcd(5, 6);    //this is a must, assign soft serial pins

#define SERVER_RESET_PORT  7

#define SSID    "1820hearst"        // SSID of your AP
#define Key     "skibridger"  // Key or Passphrase
// Wi-Fi security option (NO_SECURITY, WEP_SECURITY, WPA_SECURITY, WPA2PSK_SECURITY)
#define Security        WPA2PSK_SECURITY

#define TEST_PORT    32400

WizFi2x0Class myWizFi;
TimeoutClass ConnectInterval;
uint8_t retval;
byte rcvdBuf[65];

byte currentServerAddress = 1;
boolean isServerCorrect = false;
unsigned long lastContactTime = 0;
unsigned long nextCheck = 0;
unsigned long nextSendToServer = 0;
byte serverBase[4];

///////////////////////////////
// 1msec Timer
void Timer1_ISR()
{
  myWizFi.ReplyCheckTimer.CheckIsTimeout();
}
//
//////////////////////////////

void setup() {
  Serial.begin(9600);
  Serial.println(F("Serial Init"));
  
  slcd.begin();
  slcd.noCursor();
  slcd.noBacklight();
  PROG_FUNC("Server IP: ", slcd.print);
  slcd.setCursor(0, 1);
  PROG_FUNC("Temp: ", slcd.print);
  
  pinMode(SERVER_RESET_PORT, OUTPUT);
  digitalWrite(SERVER_RESET_PORT, 0);

  // initalize WizFi2x0 module:
  myWizFi.begin();
 
  ConnectInterval.init();

  // Timer1 Initialize
  Timer1.initialize(1000); // 1msec
  Timer1.attachInterrupt(Timer1_ISR);
 
  myWizFi.SendSync();
  myWizFi.ReplyCheckTimer.TimerStart(3000);
  
  while(1)
  {
    if(myWizFi.CheckSyncReply())
    {
      myWizFi.ReplyCheckTimer.TimerStop();
      Serial.println(F("Rcvd Sync data"));
      break;
    }
    if(myWizFi.ReplyCheckTimer.GetIsTimeout())
    {
      Serial.println(F("Rcving Sync Timeout!!"));
      // Nothing to do forever;
      for(;;)
      ;
    }
  }
}

bool testConnectionWith(byte lastOctet)
{
  serverBase[3] = lastOctet;  
  
  // Socket Creation with Server IP address and Server Port num 
  WizFiClient myClient = WizFiClient(serverBase, TEST_PORT);
  retval = myClient.connect();
  boolean found = false;
  if (retval == 1)
  {
    PROG_FUNC_CAST("GET /\r\n\r\n", myClient.write, byte);
    memset(rcvdBuf, 0, 65);
    retval = 0;
    unsigned long timeout = millis() + 30000;
    while (myClient.available() && millis() < timeout)
    {
      myWizFi.RcvPacket();
      if (myClient.read(rcvdBuf))
      {
        String str = String((char*)rcvdBuf);
        if (str.indexOf("X-Plex") >= 0)
        {
          found = true;
        }
      }
    }
    
    if (myClient.available())
    {
      myClient.disconnect();
    }
  }
  return found;
}

int freeRam() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void sendToDB(const prog_char *name, const prog_char *value, int intValue)
{
  WizFiClient myClient = WizFiClient("gpa.iriscouch.com", 80);
  retval = myClient.connect();
  if (retval == 1)
  {
    PROG_FUNC_CAST("POST /sensors HTTP/1.1\nHost: gpa.iriscouch.com\nContent-Length: ", myClient.write, byte);
    
    int contentLength = 24 + strlen_P(name);
    if (value)
    {
      contentLength += 2 + strlen_P(value);
    }
    else
    {
      sprintf((char*)rcvdBuf, "%d", intValue);
      contentLength += strlen((char*)rcvdBuf);
    }
    sprintf((char*)rcvdBuf, "%d", contentLength);
    myClient.write(rcvdBuf);
    PROG_FUNC_CAST("\nContent-Type: application/json\n\n{\"device\":\"server\",\"", myClient.write, byte);
    PROG_VAR_FUNC_CAST(name, myClient.write, byte);
    PROG_FUNC_CAST("\":", myClient.write, byte);
    if (value)
    {
      PROG_FUNC_CAST("\"", myClient.write, byte);
      PROG_VAR_FUNC_CAST(value, myClient.write, byte);
      PROG_FUNC_CAST("\"", myClient.write, byte);
    }
    else
    {
      sprintf((char*)rcvdBuf, "%d", intValue);
      myClient.write(rcvdBuf);
    }
    PROG_FUNC_CAST("}\n", myClient.write, byte);
    memset(rcvdBuf, 0, 65);
    retval = 0;
    unsigned long timeout = millis() + 60000;
    while (myClient.available() && millis() < timeout)
    {
      myWizFi.RcvPacket();
      if (myClient.read(rcvdBuf))
      {
        Serial.println((char *)rcvdBuf);
      }
    }

    if (myClient.available())
    {
      Serial.println(F("DISCONNECTING"));
      myClient.disconnect();
    }
  }
}

void sendEvent(const prog_char *event)
{
  sendToDB(PSTR("event"), event, 0);
}

void sendTemperature(int temp)
{
  sendToDB(PSTR("temp"), NULL, temp);
}

void loop()
{
  sensorValue = analogRead(sensorPin);
  temperature = (float)(1023 - sensorValue) * 10000 / sensorValue; //get the resistance of the sensor;
  temperature = 1 / (log(temperature / 10000) / 3975 + 1/298.15) - 273.15;//convert to temperature via datasheet ;

  slcd.setCursor(11, 0);
  slcd.print((int)currentServerAddress, 10);
  if (!isServerCorrect) slcd.print("? ");
  else slcd.print("  ");
  slcd.print((millis() / 100) % 10, 10);
  slcd.setCursor(6, 1);
  slcd.print(int(temperature), 10);
//  slcd.print(freeRam(), 10);
  
  if (!myWizFi.IsAssociated())
  {
    PROG_FUNC(" No WiFi", slcd.print);
    ////////////////////////////////////////////////////////////////////////////
    // AP association  
    Serial.println(F("AP association attempt"));
    retval = myWizFi.associate(SSID, Key, Security, true);
    if (retval == 1) {
      Serial.println(F("AP association Success"));
      byte serverAddress[16];
      myWizFi.GetSrcIPAddr(serverAddress);
      Serial.print(F("Got IP: "));
      Serial.println((char*)serverAddress);
      String server = String((char*)serverAddress);
      int pos = 0;
      for (int i = 0; i < 3; i++)
      {
        int nextDot = server.indexOf(".", pos);
        serverBase[i] = (byte)server.substring(pos, nextDot).toInt();
        pos = nextDot + 1;
      }
      lastContactTime = millis();
      nextCheck = lastContactTime;
      sendEvent(PSTR("connected"));
    } else {
      Serial.println(F("AP association Failed"));
    }
  }
  else
  {
    slcd.print(" WiFi OK");
    if (millis() > nextCheck)
    {
      if (!isServerCorrect)
      {
        if (testConnectionWith(currentServerAddress))
        {
          Serial.print(F("FOUND: "));
          Serial.println(currentServerAddress);
          isServerCorrect = true;
          lastContactTime = millis();
        }
        else
        {
          if (++currentServerAddress > 20) currentServerAddress = 1;
        }
      }
      else 
      {
        if (testConnectionWith(currentServerAddress))
        {
          lastContactTime = millis();
          Serial.println(F("Still OK!"));
          nextCheck = millis() + 60000;
        }
        else
        {
          currentServerAddress = 1;
          isServerCorrect = false;
        }
      }
      
      if (millis() > lastContactTime + (15 * 60000))
      {
        // Might be dead....
        Serial.println(F("Resetting"));
        sendEvent(PSTR("reset"));
        digitalWrite(SERVER_RESET_PORT, 1);
        delay(7000);
        digitalWrite(SERVER_RESET_PORT, 0);
        delay(5000);
        digitalWrite(SERVER_RESET_PORT, 1);
        delay(1000);
        digitalWrite(SERVER_RESET_PORT, 0);
        lastContactTime = nextCheck = millis() + (20 * 60000);
      }
    }
    else if (millis() > nextSendToServer)
    {
      Serial.println(F("Sending to server"));
      nextSendToServer = millis() + 1800000;
      sendTemperature(int(temperature));
    }
  }
}

