// Full_Sensor_Test.ino
//
// Test for Roomba class
// Tests reading of sensor data from a Create using the getSensors command
//
// Runs on an Arduino Mega with Create connected to Serial1
// Prints all sensor data to serial monitor if read successfully
//
//
// Last edited: 9/26/2013





#include <Roomba.h>

// Defines the Roomba instance and the HardwareSerial it connected to
Roomba roomba(&Serial1);


int x;                   // 16 bit signed variable
unsigned int u;          // 16 bit unsigned variable

void setup()
{
  Serial.begin(57600);          // baud rate of serial monitor

  roomba.start();
  roomba.safeMode();

}

void loop()
{
  uint8_t buf[52];                                        // Packet 6 returns 52 bytes
  bool ret = roomba.getSensors(6 , buf , 52);             // packetID, destination, length (bytes) 
                                                          // Note that getSensors() -returns true when [length] bytes have been read 
                                                          //                        -is blocking until [length] bytes have been read or timeout occurs
                                                          // Consult Open Interface Manual pg 17 for packet lengths
  if (ret)
  {
    Serial.println("\n\n\n\n\n\n");
    Serial.println("iRobot Create Packet 6 (Full) Sensor Data \n");
    

    Serial.print("Bumps and Wheel Drops: ");
    Serial.println(buf[0], BIN);
     Serial.print("* Wheeldrop- Caster: ");
     Serial.println(bitRead(buf[0], 4));
     Serial.print("* Wheeldrop- Left: ");
     Serial.println(bitRead(buf[0], 3));   
     Serial.print("* Wheeldrop- Right: ");
     Serial.println(bitRead(buf[0], 2));     
     Serial.print("* Bump Left: ");
     Serial.println(bitRead(buf[0], 1));
     Serial.print("* Bump Right: ");
     Serial.println(bitRead(buf[0], 0));
     
    Serial.print("Wall Sensor: ");
    Serial.println(buf[1], BIN);

    Serial.print("Cliff Left: ");
    Serial.println(buf[2], BIN);  
    Serial.print("Cliff Front Left: ");  
    Serial.println(buf[3], BIN);   
    Serial.print("Cliff Front Right: ");  
    Serial.println(buf[4], BIN);  
    Serial.print("Cliff Right: ");    
    Serial.println(buf[5], BIN);

    Serial.print("Virtual Wall: ");
    Serial.println(buf[6], BIN);

    Serial.print("Low Side Drivers and Wheel Overcurrents: ");
    Serial.println(buf[7], BIN);

    //Note the 2 unused bytes pg 18 of Open Interface Manual
    Serial.print("Infrared Byte: ");
    Serial.println(buf[10], BIN);

    Serial.print("Buttons: ");
    Serial.println(buf[11], BIN);
     Serial.print("* Play Button: ");
     Serial.println(bitRead(buf[11], 0));
     Serial.print("* Advance Button: ");
     Serial.println(bitRead(buf[11], 2));
     
    Serial.print("Distance (mm): ");              // Sum of distance traveled by both wheels divided by two
    x = BitShiftCombine(buf[12], buf[13]);        // Value sent as 16 bit signed value high byte first.
    Serial.println(x);                            // Note that if note called frequently, value is capped at min or max

    Serial.print("Angle (Degrees- CCW+): ");          
    x = BitShiftCombine(buf[14], buf[15]);        // Value sent as 16 bit signed value high byte first.
    Serial.println(x);                            // Note that if note called frequently, value is capped at min or max 

    Serial.print("Charging State: ");
    u = buf[16];
    Serial.println(u);

    Serial.print("Battery Voltage (mV): ");
    u = BitShiftCombine(buf[17], buf[18]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);      

    Serial.print("Battery Current (mA): ");
    x = BitShiftCombine(buf[19], buf[20]);      // Value sent as 16 bit signed value high byte first.
    Serial.println(x);       

    Serial.print("Battery Temperature (C): ");
    x = buf[21];
    Serial.println(x);

    Serial.print("Current Battery Charge (mAh): ");
    u = BitShiftCombine(buf[22], buf[23]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);

    Serial.print("Battery Capacity (mAh): ");
    u = BitShiftCombine(buf[24], buf[25]);      // Value sent as 16 bit signed value high byte first.
    Serial.println(u);  

    Serial.print("Wall Signal Strength (0-4095): ");
    u = BitShiftCombine(buf[26], buf[27]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);    

    Serial.print("Cliff Left Signal Strength (0-4095): ");
    u = BitShiftCombine(buf[28], buf[29]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);  

    Serial.print("Cliff Front Left Signal Strength (0-4095): ");
    u = BitShiftCombine(buf[30], buf[31]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);  

    Serial.print("Cliff Front Right Signal Strength (0-4095): ");
    u = BitShiftCombine(buf[32], buf[33]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);  

    Serial.print("Cliff Right Signal Strength (0-4095): ");
    u = BitShiftCombine(buf[34], buf[35]);      // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);  

    Serial.print("Cargo Bay Digital Inputs: ");
    Serial.println(buf[36], BIN);

    Serial.print("Cargo Bay Analog Signal (0-1023):");
    u = BitShiftCombine(buf[37], buf[38]);       // Value sent as 16 bit unsigned value high byte first.
    Serial.println(u);  

    Serial.print("Charging Sources Available:");
    Serial.println(buf[39], BIN);

    Serial.print("OI Mode: ");
    u = buf[40];
    Serial.println(u);

    Serial.print("Song Number: ");
    x = buf[41];
    Serial.println(x);

    Serial.print("Song Playing: ");
    Serial.println(buf[42], BIN);

    Serial.print("Number of Stream Packets: ");
    u = buf[43];
    Serial.println(u); 

    Serial.print("Requested Velocity (mm/s): ");
    x = BitShiftCombine(buf[44], buf[45]);      // Value sent as 16 bit signed value high byte first.
    Serial.println(x);       


    Serial.print("Requested Radius (mm):");
    x = BitShiftCombine(buf[46], buf[47]);      // Value sent as 16 bit signed value high byte first.
    Serial.println(x);       


    Serial.print("Requested Right Velocity (mm/s): ");
    x = BitShiftCombine(buf[48], buf[49]);      // Value sent as 16 bit signed value high byte first.
    Serial.println(x);       


    Serial.print("Requested Left Velocity (mm/s): ");
    x = BitShiftCombine(buf[50], buf[51]);      // Value sent as 16 bit signed value high byte first.
    Serial.println(x);       
   

    delay(5000);                      // Update every 5 seconds
    //while(1);                         // Run only once
 
  }  // if(ret)
  else Serial.println("GetSensors() Error");
  
}    // void loop()




int BitShiftCombine( unsigned char x_high, unsigned char x_low)
{


  int combined;  
  combined = x_high;              //send x_high to rightmost 8 bits
  combined = combined<<8;         //shift x_high over to leftmost 8 bits
  combined |= x_low;                 //logical OR keeps x_high intact in combined and fills in rightmost 8 bits 
  return combined;

}



