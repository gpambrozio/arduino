/******************************************************************
 WizFiShield UDP Server Teset Example
 
 Circuit:
 WizFiShield connected to Arduino via SPI
 
 RST: pin 2  // Output
 DRDY: pin 3  // Input
 CSB: pin 4  // output

 MOSI: pin 11  // output
 MISO: pin 12  // input
 SCK: pin 13  // out
 
 Created 27 Sep. 2012
 by James YS Kim  (jameskim@wiznet.co.kr, javakys@gmail.com)
 
 Modified 27 May. 2013
 by Jinbuhm Kim  (jbkim@wiznet.co.kr, jinbuhm.kim@gmail.com)

*****************************************************************/
// WizFiShield communicates using SPI, so include the SPI library:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SPI.h>
#include <WizFi2x0.h>
#include <WizFiUDP.h>
#include <TimerOne.h>

#define SSID    ""        // SSID of your AP
#define Key     ""  // Key or Passphrase
// Wi-Fi security option (NO_SECURITY, WEP_SECURITY, WPA_SECURITY, WPA2PSK_SECURITY)
//#define Security        WPA_SECURITY

unsigned int SrcPort = 50000;

WizFi2x0Class myWizFi;
WizFiUDP myUDP;
TimeoutClass ConnectInterval;
boolean Wifi_setup = false;
///////////////////////////////
// 1msec Timer
void Timer1_ISR()
{
  myWizFi.ReplyCheckTimer.CheckIsTimeout();
}
//
//////////////////////////////

void setup() {
  byte retval;
  
  Serial.begin(9600);
  Serial.println("\r\nSerial Init");
  
  // initalize WizFiShield
  myWizFi.begin();
  myUDP = WizFiUDP((uint8_t *)NULL, 0, SrcPort);
 
  ConnectInterval.init();

  // Timer1 Initialize
  Timer1.initialize(1000); // 1msec
  Timer1.attachInterrupt(Timer1_ISR);
 
  myWizFi.SendSync();
  myWizFi.ReplyCheckTimer.TimerStart(3000);
  
  Serial.println("Send Sync data");
  while(1)
  {
    if(myWizFi.CheckSyncReply())
    {
      myWizFi.ReplyCheckTimer.TimerStop();
      Serial.println("Rcvd Sync data");
      break;
    }
    if(myWizFi.ReplyCheckTimer.GetIsTimeout())
    {
      Serial.println("Rcving Sync Timeout!!");
      // Nothing to do forever;
      for(;;)
      ;
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  // AP association  
  while(1)
  {
    byte tmpstr[32];
    
    retval = myWizFi.associate(SSID, Key, Security, true);
    
    if(retval == 1){
      myWizFi.GetSrcIPAddr(tmpstr);
      Serial.println("AP association Success");
      Serial.print("MY IPAddress: ");
      Serial.println((char *)tmpstr);
      Wifi_setup = true;
      break;
    }else{
      Serial.println("AP association Failed");
    }
  }
  
  if(myUDP.open())
    Serial.println("UDP socket created successfully");
  else
    Serial.println("UDP socket creation failed");
  
}

void loop()
{
  byte rcvdBuf[129];
  byte tmpIP[16];
  uint16_t tmpPort;
 
  memset(rcvdBuf, 0, 129);
  
  if(Wifi_setup)
  {  
    myWizFi.RcvPacket();
    if(myUDP.available())
    {
      if(myUDP.read(rcvdBuf))
      {
           Serial.print("CID[");
           Serial.print((char)myUDP.GetCID());
           Serial.print("]");
           Serial.println((char *)rcvdBuf);
           myUDP.GetCurrentDestInfo(tmpIP, &tmpPort);
           Serial.println((char *)tmpIP);
           sprintf((char *)tmpIP, "Portnum: %u", tmpPort);
           Serial.println((char *)tmpIP);
           myUDP.write(rcvdBuf);
           
           // Send data to another IP address. Specify the other UDP peer's IP & port
           myUDP.SetCurrentDestInfo((byte *)"192.168.123.170", 5500);
           myUDP.write(rcvdBuf);
      }
    }
  }
}

