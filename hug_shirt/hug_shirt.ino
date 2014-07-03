/*
  Analog input, analog output, serial output
 
 Reads an analog input pin, maps the result to a range from 0 to 255
 and uses the result to set the pulsewidth modulation (PWM) of an output pin.
 Also prints the results to the serial monitor.
 
 The circuit:
 * potentiometer connected to analog pin 0.
   Center pin of the potentiometer goes to the analog pin.
   side pins of the potentiometer go to +5V and ground
 * LED connected from digital pin 9 to ground
 
 created 29 Dec. 2008
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 
 */

#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define LED_PIN       2
#define ANALOG_IN     A5
#define THRESHOLD     150
#define LEVEL_THRESHOLD   300
#define RAINBOW_THRESHOLD   400

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(11, LED_PIN, NEO_GRB + NEO_KHZ800);

int count = 0;
unsigned long hideCount = 0;

#define EEPROM_COUNT    0

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  
  pinMode(ANALOG_IN, INPUT);
  digitalWrite(ANALOG_IN, HIGH);  // set pullup on analog pin

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  count = EEPROM.read(EEPROM_COUNT) + (EEPROM.read(EEPROM_COUNT+1) << 8);
  updateCount(count);
}

uint16_t rainbowPosition = 0;
uint16_t sensorValue = 0;        // value read from the pot
unsigned long debounce = 0;
boolean state = false;
boolean prevState = false;
boolean newState = false;
boolean counted = false;

void loop() {
  // read the analog in value:
  sensorValue = analogRead(ANALOG_IN);
  newState = (sensorValue < THRESHOLD);
  if (newState != prevState) {
    debounce = millis() + (state ? 1000 : 200);
    prevState = newState;
  } else if (millis() > debounce) {
    state = newState;
    if (state && !counted) {
      counted = true;
      count++;
      EEPROM.write(EEPROM_COUNT, count & 0xFF);
      EEPROM.write(EEPROM_COUNT+1, (count >> 8) & 0xFF);

      updateCount(count);      
    } else if (!state) {
      counted = false;
    }
  }
  
  if (millis() > hideCount) {
    if (sensorValue < LEVEL_THRESHOLD) {
      showBar(LEVEL_THRESHOLD - sensorValue);
    } else if (sensorValue < RAINBOW_THRESHOLD) {
      wipe();      
    } else {
      rainbowCycle(rainbowPosition++);
      rainbowPosition &= 0xff;
    }
  }
  strip.setPixelColor(0, state ? strip.Color(100,0,0) : strip.Color(0,0,0));
  strip.show();

  // print the results to the serial monitor:
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("    count = ");
  Serial.print(count);
  Serial.print("   state = ");
  Serial.println(state ? "ON" : "OFF");

  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(2);
}


void wipe() {
  for (int i=0;i<10;i++) {
    strip.setPixelColor(1+i, strip.Color(0,0,0));
  }
}

void showBar(uint16_t level) {
  float fLevel = 0.95f * ((float)level / (LEVEL_THRESHOLD - THRESHOLD));
  for (int i=0;i<5;i++) {
    int color = fLevel >= 0.2 ? 255 : (255 * 5 * fLevel);
    if (fLevel > 0.2)
      fLevel -= 0.2;
    else
      fLevel = 0.0;
    strip.setPixelColor(1+i, strip.Color(0,0,color));
    strip.setPixelColor(10-i, strip.Color(0,0,color));
  }
}

void updateCount(int aux) {
  int remainder;
  uint32_t color;
  for (int i=0;i<5;i++) {
    remainder = aux % 10;
    strip.setPixelColor(1+i, remainder >= 5 ? strip.Color(50,50,0) : strip.Color(0,0,0));
    remainder %= 5;
    switch (remainder) {
      case 0:
        color = strip.Color(0,0,0);
        break;
      case 1:
        color = strip.Color(100,0,0);
        break;
      case 2:
        color = strip.Color(0,100,0);
        break;
      case 3:
        color = strip.Color(0,0,100);
        break;
      case 4:
        color = strip.Color(50,50,0);
        break;
    }
    strip.setPixelColor(10-i, color);
    aux /= 10;
  }
  hideCount = millis() + 4000;
}


void rainbowCycle(uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels()-1; i++) {
    strip.setPixelColor(i+1, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos, 85 - WheelPos, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(85 - WheelPos, 0, WheelPos);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos, 85 - WheelPos);
  }
}

