/*

Test for Roomba class
Tests reading of sensor data from a Create using the stream() command
Drives forward, turns left, right, stops, and repeats

Runs on an Arduino Mega with Create connected to Serial1

Last edited: 10/1/2013
*/


#include <Roomba.h>

// Defines the Roomba instance and the HardwareSerial it connected to
Roomba roomba(&Serial1);

void setup()
{
  //    Serial.begin(9600);      // For debugging
  
  roomba.start();
  roomba.safeMode();

}

void loop()
{
  uint8_t buf[52];                                        // Packet 6 returns 52 bytes
  bool ret = roomba.getSensors(6 , buf , 52);             // packetID, destination, length (bytes) 

  if (ret)   // Only drive if we are getting valid sensor readings
  {
    //Serial.println(buf[0], BIN);

    // Note that all these functions are blocking. The sensor reading will not be updated until they finish
    if(bitRead(buf[0], 2) == 1 || bitRead(buf[0], 3) == 1 || bitRead(buf[0], 4) == 1)   // Any of the wheels drop
    {                                                                 // Note that safe mode exibits this functionality anyway
      roomba.driveDirect(0, 0);
      delay(3);
    }   
    
    else if(bitRead(buf[0], 0) == 1 && bitRead(buf[0], 1) == 1)   {    //Bump Both
      roomba.driveDirect(-300, -300);
      delay(500);
      roomba.driveDirect(-300 , 300);   // Left/Right Wheel velocity (mm/s)
      delay(1000);
    }
    else if(bitRead(buf[0], 0) == 1)   {    //Bump Right
      roomba.driveDirect(-300, -300);
      delay(500);
      roomba.driveDirect(-300 , 300);   // Left/Right Wheel velocity (mm/s)
      delay(250);
    }    
    else if(bitRead(buf[0], 1) == 1)   {    //Bump Left
      roomba.driveDirect(-300, -300);
      delay(500);
      roomba.driveDirect(300 , -300);   // Left/Right Wheel velocity (mm/s)
      delay(250);
    }      
    else
    {
      roomba.driveDirect(300 , 300);   // Left/Right Wheel velocity (mm/s)
    }

  } 
  else                //if sensor readings are invalid, don't move.
  {
    roomba.driveDirect(0, 0);        // Left/Right Wheel velocity (mm/s)  
  }

}




