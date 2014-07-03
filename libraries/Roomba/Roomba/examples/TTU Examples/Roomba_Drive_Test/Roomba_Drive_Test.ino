//
//
// Test for Roomba class
// Example of Create Drive commands
//
// Create will cycle throught the following motions
// -Drive straight
// -Spin CounterClockwise
// -Spin Clockwise
// -Stop
// -Turn Left
// -Turn Right
// -Drive Straight
// -Spin Clockwise
// -Spin CounterClockwise
// -Stop
//
// Runs on an Arduino Mega with Create connected to Serial1
//
// Last editied: 10/1/2013


#include <Roomba.h>

// Defines the Roomba instance and the HardwareSerial it connected to
Roomba roomba(&Serial1);

void setup()
{
//    Serial.begin(9600);
     
    roomba.start();
    roomba.safeMode();
}

void loop()
{
  // Serial.println("driveDirect");
  delay(1000);
  roomba.driveDirect(300 , 300);   // Often the first call after restart is not registered. This is an ongoing problem with no known fix.
  roomba.driveDirect(300 , 300);   // Left/Right Wheel velocity (mm/s)
  delay(1000);
  roomba.driveDirect(-300 , 300);  // Left/Right Wheel velocity (mm/s)
  delay(1000);
  roomba.driveDirect(300, -300);   // Left/Right Wheel velocity (mm/s)
  delay(1000);  
  roomba.driveDirect(0, 0);        // Left/Right Wheel velocity (mm/s)
  delay(2000);   

  // Serial.println("drive");  
  roomba.drive(300, 300);                   // Velocity (mm/s), Radius (mm)
  delay(1000);
  roomba.drive(300, -300);
  delay(1000);
  roomba.drive(300, roomba.DriveStraight);                  //DriveStraight is a special case. See Public Types in Roomba Class Reference
  delay(1000);
  roomba.drive(300, roomba.DriveInPlaceClockwise);
  delay(1000);
  roomba.drive(300, roomba.DriveInPlaceCounterClockwise);
  delay(1000);  
  roomba.drive(0, 0);
  delay(1000);
}
