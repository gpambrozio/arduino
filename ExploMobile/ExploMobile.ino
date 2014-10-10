/* 
*/

#include <SPI.h>
#include <Wire.h>

#include "RadioCommon.h"
#include "nRF24L01.h"
#include "RF24.h"

#define MOTOR         3
#define MOTOR_MAX     255
#define MOTOR_MIN     42

#define START_TIME    9
#define END_TIME      21

#define AUTO_OFF      5   // in minutes

#define FIRST_BUTTON  5
#define ANY_BUTTON    (FIRST_BUTTON+0)
#define BUTTON_A      (FIRST_BUTTON+1)
#define BUTTON_B      (FIRST_BUTTON+2)
#define BUTTON_C      (FIRST_BUTTON+3)
#define BUTTON_D      (FIRST_BUTTON+4)
#define BUTTONS       5

byte modes[] = {0, 47, 120, 170, 240};

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
// First = CE. Second = CS
RF24 radio(2, 10);

#define clockAddress 0x68

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
byte lastReportedMinute = 100;

byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// 4) DOW: 1 = Sun, 7 = Sat
// Assumes you're passing in valid numbers, 
// Probably need to put in checks for valid numbers.
void setDateDs1307()                
{
  while (Serial.available() < 13) {
    delay(1);
  }
  // Use of (byte) type casting and ascii math to achieve result.  
  second = (byte) ((Serial.read() - '0') * 10 + (Serial.read() - '0')); 
  minute = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  hour  = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  dayOfWeek = (byte) (Serial.read() - '0');
  dayOfMonth = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  month = (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  year= (byte) ((Serial.read() - '0') *10 +  (Serial.read() - '0'));
  Wire.beginTransmission(clockAddress);
  Wire.write(byte(0x00));
  Wire.write(decToBcd(second));  // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));    // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.write(byte(0x00));
  Wire.endTransmission();
}

// Gets the date and time from the ds1307 and prints result
void getDateDs1307() {
  // Reset the register pointer
  Wire.beginTransmission(clockAddress);
  Wire.write(byte(0x00));
  Wire.endTransmission();

  Wire.requestFrom(clockAddress, 7);

  // A few of these need masks because certain bits are control bits
  second     = bcdToDec(Wire.read() & 0x7f);
  minute     = bcdToDec(Wire.read());
  
  // Need to change this if 12 hour am/pm
  hour       = bcdToDec(Wire.read() & 0x3f);  
  dayOfWeek  = bcdToDec(Wire.read());
  dayOfMonth = bcdToDec(Wire.read());
  month      = bcdToDec(Wire.read());
  year       = bcdToDec(Wire.read());
}

void printDateToSerial() {
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.println(year, DEC);
}

void setup() {
  digitalWrite(MOTOR, LOW);
  pinMode(MOTOR, OUTPUT);

  Wire.begin();
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Starting"); 

  START_RADIO(radio, RADIO_MOBILE);

  startBananas();
}

int motorPower = MOTOR_MIN;
unsigned long mirfData;
byte inByte;
byte buttonState = 0;
long debounce = 0;
int mode = -1;
boolean bananas = false;
long bananasTargetTime;

long offTargetTime;
long rampTargetTime;
long rampStartTime;
byte rampStartMotor;
byte rampTargetMotor;
double rampMotorDelta;

boolean sleeping = false;

#define DEFAULT_RAMP    20    // Meaning this change in power per second

#define SPEED_UP    { rampTargetTime = 0; mode = -1; bananas = false; if (motorPower == 0) motorPower = MOTOR_MIN; else if (motorPower < MOTOR_MAX) motorPower++; rampTargetMotor = motorPower; Serial.println(motorPower); }
#define SPEED_DOWN  { rampTargetTime = 0; mode = -1; bananas = false; if (motorPower > MOTOR_MIN) motorPower--; else motorPower = 0; rampTargetMotor = motorPower; Serial.println(motorPower); };

long rampChange(int motor) {
  if (motor == motorPower) {
    rampTargetTime = 0;
    return millis();
  }
  rampStartMotor = motorPower;
  rampTargetMotor = motor;

  long time = (long)motor - (long)motorPower;
  // Don' use any other operation inside of abs. See doc.
  time = 1000 * abs(time) / DEFAULT_RAMP;

  rampMotorDelta = (double)((int)rampTargetMotor - (int)rampStartMotor) / (double)time;
  rampStartTime = millis();
  rampTargetTime = rampStartTime + time;
  Serial.print("New ramp ");
  Serial.print(time);
  Serial.print("ms ");
  Serial.print(rampTargetMotor);
  Serial.print("power ");
  Serial.println(rampMotorDelta * 1000);
  return rampTargetTime;
}

void bananasChange() {
  bananasTargetTime = rampChange(random(MOTOR_MIN, MOTOR_MAX + 1)) + 5000;
}

void startBananas() {
  Serial.println("Bananas");
  mode = -1;
  bananas = true;
  randomSeed(millis());
  bananasChange();
}

void loop() {
  getDateDs1307();
  if (lastReportedMinute != minute) {
    printDateToSerial();
    lastReportedMinute = minute;
  }
  
  if ((hour >= END_TIME || hour < START_TIME) && offTargetTime == 0 && motorPower != 0 && rampTargetMotor != 0) {
    offTargetTime = millis() + AUTO_OFF * 60000;
  }
  
  if (offTargetTime > 0 && millis() > offTargetTime) {
    offTargetTime = 0;
    bananas = false;
    mode = -1;
    rampChange(0);
    sleeping = true;
  }
  
  if (sleeping && hour >= START_TIME) {
    sleeping = false;
    startBananas();
  }
  
  if (bananas && millis() > bananasTargetTime) {
    bananasChange();
  }
  
  if (rampTargetTime > 0 && rampTargetMotor != motorPower) {
    long deltaTime = millis() - rampStartTime;
    byte newMotorPower = rampStartMotor + (byte)(deltaTime * rampMotorDelta);
    if (newMotorPower != motorPower) {
      motorPower = newMotorPower;
      Serial.println(motorPower);
    }
  }
  analogWrite(MOTOR, motorPower);
  
  byte newButtonState = 0;
  for (byte i=0; i<BUTTONS;i++) {
    newButtonState <<= 1;
    if (digitalRead(FIRST_BUTTON+i)) {
      newButtonState |= 1;
    }
  }
  
  byte change = newButtonState ^ buttonState; 
  if (change) {
    debounce = millis();
    
    // Button A
    if ((change & B1000) && (newButtonState & B1000)) {
      SPEED_DOWN
    }

    // Button B
    if ((change & B0100) && (newButtonState & B0100)) {
      SPEED_UP
    }
    
    // Button C
    if ((change & B0010) && (newButtonState & B0010)) {
      startBananas();
    }
    
    // Button D
    if ((change & B0001) && (newButtonState & B0001)) {
      bananas = false;
      if (mode == -1) {
        for (byte i=0;i<sizeof(modes)/sizeof(byte);i++) {
          if (motorPower < modes[i]) {
            mode = i;
            break;
          }
        }
      } else {
        if (++mode >= sizeof(modes)/sizeof(byte)) mode = 0;
      }
      rampChange(modes[mode]);
    }
    
    buttonState = newButtonState;
  } else if (buttonState & B1100) {   // Only repeat for button A and B
    if (millis() - debounce > 50) {
      buttonState = 0;
    }
  }

  if (Serial.available()) {  // Look for char in serial que and process if found
    int command = Serial.read();
    switch (command) {
      case 'T':      //If command = "T" Set Date
        setDateDs1307();
        Serial.println("time");
        getDateDs1307();
        printDateToSerial();
        break;

      case '+':
        Serial.println("Received +");
        SPEED_UP
        break;
        
      case '-':
        Serial.println("Received -");
        SPEED_DOWN
        break;
        
      case 'B':
        startBananas();
        break;

      default:
        Serial.print("Received command ");
        Serial.println(command);
    }
  }

  if (radio.available()) {
    radio.read((byte *)&mirfData, sizeof(unsigned long));
    inByte = mirfData & 0xFF;
    switch (inByte) {
      case '+':
        Serial.println("Received +");
        SPEED_UP
        break;
        
      case '-':
        Serial.println("Received -");
        SPEED_DOWN
        break;
        
      case 'T':
        mode = -1;
        bananas = false;
        rampChange(constrain((mirfData >> 8) & 0xFF, MOTOR_MIN, MOTOR_MAX));
        break;
        
      case 'B':
        startBananas();
        break;
        
      default:
        Serial.print("Received data ");
        Serial.println(mirfData);
    }
  }
}

