/*
 
  The circuit:
 * LCD RS pin to digital pin 3
 * LCD Enable pin to digital pin 2
 * LCD D4 pin to digital pin 6
 * LCD D5 pin to digital pin 7
 * LCD D6 pin to digital pin 8
 * LCD D7 pin to digital pin 9
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 */

// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>

#define START_HOUR        3

// 1 = grass
// 2 = drip

#define DAYS_PERIOD_R1    3
#define DAYS_PERIOD_R2    2
#define MINUTES_R1       30
#define MINUTES_R2       30

#define clockAddress 0x68

#define RELAY1   10
#define RELAY2   11

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(3, 2, 6, 7, 8, 9);

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

// Convert normal decimal numbers to binary coded decimal
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
// Assumes you're passing in valid numbers, 
// Probably need to put in checks for valid numbers.
void setDateDs1307()                
{
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

void twoDigit(byte x) {
  if (x < 10)
    lcd.print("0");
  lcd.print(x);
}

int daysFromStartOfYear(byte day, byte month) {
  switch(month) {
    case 1:
      return day;
    case 2:
      return 31+day;
    case 3:
      return 59+day;
    case 4:
      return 90+day;
    case 5:
      return 120+day;
    case 6:
      return 151+day;
    case 7:
      return 181+day;
    case 8:
      return 212+day;
    case 9:
      return 243+day;
    case 10:
      return 273+day;
    case 11:
      return 304+day;
    case 12:
      return 334+day;
  }
}

void setup() {
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
  Wire.begin();
  Serial.begin(9600);
  
  getDateDs1307();
}

int command = 0;
int aux1, aux2;

byte forceR1 = 0;
byte forceR2 = 0;

void loop() {
  if (Serial.available()) {  // Look for char in serial que and process if found
    command = Serial.read();
    Serial.print("Command: ");
    Serial.println(command);  // Echo command CHAR in ascii that was sent
    switch (command) {
      case 'T':      //If command = "T" Set Date
        setDateDs1307();
        getDateDs1307();
        Serial.println("");
        break;

      case 'R':
        aux1 = Serial.read();
        aux2 = Serial.read();
        if (aux1 == '1') {
          forceR1 = aux2 == '1' ? 1 : aux2 == '0' ? 2 : 0;
        } else if (aux1 == '2') {
          forceR2 = aux2 == '1' ? 1 : aux2 == '0' ? 2 : 0;
        }
        break;
    }
  }

  command = 0;  // reset command                  
  delay(100);

  getDateDs1307();

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  switch(dayOfWeek) {
    case 1:
      lcd.print("Sun");
      break;
    case 2:
      lcd.print("Mon");
      break;
    case 3:
      lcd.print("Tue");
      break;
    case 4:
      lcd.print("Wed");
      break;
    case 5:
      lcd.print("Thu");
      break;
    case 6:
      lcd.print("Fri");
      break;
    case 7:
      lcd.print("Sat");
      break;
    default:
      lcd.print("??");
      lcd.print(dayOfWeek);
      break;
  }
  lcd.print(" ");
  twoDigit(month);
  lcd.print("/");
  twoDigit(dayOfMonth);
  lcd.print("/");
  twoDigit(year);
  lcd.print(" ");
  twoDigit(daysFromStartOfYear(dayOfMonth, month));

  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  twoDigit(hour);
  lcd.print(":");
  twoDigit(minute);
  lcd.print(":");
  twoDigit(second);
  lcd.print(" ");
  
  if (forceR1 == 1 || (forceR1 == 0 && hour == START_HOUR && minute < MINUTES_R1 &&
      (daysFromStartOfYear(dayOfMonth, month) % DAYS_PERIOD_R1) == 0)) {
    digitalWrite(RELAY1, HIGH);
    lcd.print("R1");
  } else {
    digitalWrite(RELAY1, LOW);
    lcd.print("--");
  }
  lcd.print(" ");
  if (forceR2 == 1 || (forceR2 == 0 && hour == START_HOUR && minute < MINUTES_R2 &&
      (daysFromStartOfYear(dayOfMonth, month) % DAYS_PERIOD_R2) == 0)) {
    digitalWrite(RELAY2, HIGH);
    lcd.print("R2");
  } else {
    digitalWrite(RELAY2, LOW);
    lcd.print("--");
  }
}

