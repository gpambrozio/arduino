/*--------------------------------------------------------------------------
  GUGGENHAT: a Bluefruit LE-enabled wearable NeoPixel marquee.

  Requires:
  - Arduino-compatible microcontroller board.  Most mainstream ATmega-based
    boards (Uno, Micro, etc.) should work; cutting-edge boards (Teensy 3,
    Arduino Due, Netduino) are untested and likely not compatible.
  - Adafruit Bluefruit LE nRF8001 breakout: www.adafruit.com/products/1697
  - 4 Meters 60 NeoPixel LED strip: www.adafruit.com/product/1461
  - 3xAA alkaline cells, 4xAA NiMH or a beefy (e.g. 1200 mAh) LiPo battery.
  - Late-model Android or iOS phone or tablet running nRF UART or
    Bluefruit LE Connect app.
  - BLE_UART, NeoPixel, NeoMatrix and GFX libraries for Arduino.

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  MIT license.  All text above must be included in any redistribution.
  --------------------------------------------------------------------------*/

#include <SPI.h>
#include <Adafruit_BLE_UART.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <EEPROM.h>
#include <CapacitiveSensor.h>

#define NEO_PIN     3   // Arduino pin to NeoPixel data input
#define MIC_PIN     A0  // Microphone is attached to this analog pin

// CLK, MISO, MOSI connect to hardware SPI.  Other pins are configrable:
#define ADAFRUITBLE_REQ  4
#define ADAFRUITBLE_RST  5
#define ADAFRUITBLE_RDY  2 // Must be an interrupt pin

#define CAP1_IN      11
#define CAP1_SENSOR  12
#define CAP2_IN      10
#define CAP2_SENSOR   9

#define EXTRA_GND     8
#define MOTOR         6
#define PRESSURE     A1

#define LED          13     // Onboard LED (not NeoPixel) pin

#define MARK  {Serial.print("Running line ");Serial.println(__LINE__);}
//#define MARK  {}

// SHARED MEMORY STUFF ----------------------------------------------------------

#define SHARED_BUFFER_SIZE  14
int sharedBuffer[SHARED_BUFFER_SIZE];

uint8_t       ui81, ui82;
int           i1, i2, i3, i4;
unsigned long ul1;
uint16_t      ui161, ui162;


// CAPACITIVE SENSOR STUFF ----------------------------------------------------
CapacitiveSensor  cs1 = CapacitiveSensor(CAP1_IN, CAP1_SENSOR);
CapacitiveSensor  cs2 = CapacitiveSensor(CAP2_IN, CAP2_SENSOR);

// HUG COUNTER STUFF ----------------------------------------------------------

#define EEPROM_COUNT    0


// NEOPIXEL STUFF ----------------------------------------------------------

// 4 meters of NeoPixel strip is coiled around a top hat; the result is
// not a perfect grid.  My large-ish 61cm circumference hat accommodates
// 37 pixels around...a 240 pixel reel isn't quite enough for 7 rows all
// around, so there's 7 rows at the front, 6 at the back; a smaller hat
// will fare better.
#define NEO_WIDTH  37 // Hat circumference in pixels
#define NEO_HEIGHT  7 // Number of pixel rows (round up if not equal)
#define NEO_OFFSET  (((NEO_WIDTH * NEO_HEIGHT) - 240) / 2)

// Pixel strip must be coiled counterclockwise, top to bottom, due to
// custom remap function (not a regular grid).
Adafruit_NeoMatrix matrix(NEO_WIDTH, NEO_HEIGHT, NEO_PIN,
  NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB         + NEO_KHZ800);

#define MSG_MAX  21
#define msg ((char*)sharedBuffer)
#define msgLen (ui81)
#define msgX (i1)
#define prevFrameTime (ul1)

#define FPS 10                                // Scrolling speed

// MUSIC STUFF  ---------------------------------------------------------
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   12  // Length of buffer for dynamic level adjustment
#define TOP       (NEO_HEIGHT + 2) // Allow dot to go slightly off scale

#define vol ((int*)sharedBuffer)

#define volCount (ui81)    // Frame counter for storing past volume data
#define currentColumn (ui82)

#define lvl (i1)          // Current "dampened" audio level
#define minLvlAvg (i2)    // For dynamic adjustment of graph low & high
#define maxLvlAvg (i3)
#define height (i4)

#define minLvl (ui161)
#define maxLvl (ui162)

// COUNT STUFF  ----------------------------------------------------------
#define count (i1)

// BLUEFRUIT LE STUFF-------------------------------------------------------


Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(
  ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);
aci_evt_opcode_t  prevState  = ACI_EVT_DISCONNECTED;

// STATUS LED STUFF --------------------------------------------------------

// The Arduino's onboard LED indicates BTLE status.  Fast flash = waiting
// for connection, slow flash = connected, off = disconnected.
int           LEDperiod   = 0;   // Time (milliseconds) between LED toggles
boolean       LEDstate    = LOW; // LED flashing state HIGH/LOW
unsigned long prevLEDtime = 0L;  // For LED timing

// UTILITY FUNCTIONS -------------------------------------------------------

// Because the NeoPixel strip is coiled and not a uniform grid, a special
// remapping function is used for the NeoMatrix library.  Given an X and Y
// grid position, this returns the corresponding strip pixel number.
// Any off-strip pixels are automatically clipped by the NeoPixel library.
uint16_t remapXY(uint16_t x, uint16_t y) {
  return y * NEO_WIDTH + x - NEO_OFFSET;
}

// Given hexadecimal character [0-9,a-f], return decimal value (0 if invalid)
uint8_t unhex(char c) {
  return ((c >= '0') && (c <= '9')) ?      c - '0' :
         ((c >= 'a') && (c <= 'f')) ? 10 + c - 'a' :
         ((c >= 'A') && (c <= 'F')) ? 10 + c - 'A' : 0;
}

// Read from BTLE into buffer, up to maxlen chars (remainder discarded).
// Does NOT append trailing NUL.  Returns number of bytes stored.
uint8_t readStr(char dest[], uint8_t maxlen) {
  int     c;
  uint8_t len = 0;
  while((c = BTLEserial.read()) >= 0) {
    if(len < maxlen) dest[len++] = c;
  }
  return len;
}

// MEAT, POTATOES ----------------------------------------------------------

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  matrix.begin();
  matrix.setRemapFunction(remapXY);
  matrix.setTextWrap(false);   // Allow scrolling off left
  matrix.setTextColor(matrix.Color(0,0,0xFF)); // Blue by default
  matrix.setBrightness(31);    // Batteries have limited sauce
  matrix.fillScreen(0);

  memset(sharedBuffer, 0, SHARED_BUFFER_SIZE);

  BTLEserial.begin();

  pinMode(EXTRA_GND, OUTPUT);
  digitalWrite(EXTRA_GND, LOW);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);
  
  analogReference(EXTERNAL);
}

typedef enum {
  MODE_SOUND = 0,
  MODE_TEXT,
  MODE_COUNTER,
  MODE_LAST,
} modes;

byte prevMode = 0xff;
byte mode = MODE_SOUND;

uint8_t  i, state1, count1;
unsigned long t, motor_end;

void loop() {
  t = millis(); // Current elapsed time, milliseconds.
  // millis() comparisons are used rather than delay() so that animation
  // speed is consistent regardless of message length & other factors.

  BTLEserial.pollACI(); // Handle BTLE operations
  aci_evt_opcode_t state = BTLEserial.getState();

  if(state != prevState) { // BTLE state change?
    switch(state) {        // Change LED flashing to show state
     case ACI_EVT_DEVICE_STARTED: LEDperiod = 1000L / 10; break;
     case ACI_EVT_CONNECTED:      LEDperiod = 1000L / 2;  break;
     case ACI_EVT_DISCONNECTED:   LEDperiod = 0L;         break;
    }
    prevState   = state;
    prevLEDtime = t;
    LEDstate    = LOW; // Any state change resets LED
    digitalWrite(LED, LEDstate);
  }

  if(LEDperiod && ((t - prevLEDtime) >= LEDperiod)) { // Handle LED flash
    prevLEDtime = t;
    LEDstate    = !LEDstate;
    digitalWrite(LED, LEDstate);
  }

  // If connected, check for input from BTLE...
  if((state == ACI_EVT_CONNECTED) && BTLEserial.available()) {
    if(BTLEserial.peek() == '#') { // Color commands start with '#'
      char color[7];
      switch(readStr(color, sizeof(color))) {
       case 4:                  // #RGB    4/4/4 RGB
        matrix.setTextColor(matrix.Color(
          unhex(color[1]) * 17, // Expand to 8/8/8
          unhex(color[2]) * 17,
          unhex(color[3]) * 17));
        break;
       case 5:                  // #XXXX   5/6/5 RGB
        matrix.setTextColor(
          (unhex(color[1]) << 12) +
          (unhex(color[2]) <<  8) +
          (unhex(color[3]) <<  4) +
           unhex(color[4]));
        break;
       case 7:                  // #RRGGBB 8/8/8 RGB
        matrix.setTextColor(matrix.Color(
          (unhex(color[1]) << 4) + unhex(color[2]),
          (unhex(color[3]) << 4) + unhex(color[4]),
          (unhex(color[5]) << 4) + unhex(color[6])));
        break;
      }
    } else { // Not color, must be message string
      msgLen      = readStr(msg, MSG_MAX-1);
      msg[msgLen] = 0;
      msgX        = matrix.width(); // Reset scrolling
    }
  }

  i = cs1.capacitiveSensorRaw(30);

  count1 = ((count1 * 7) + i) >> 3;    // "Dampened" reading
  Serial.print(mode);
  Serial.print(' ');
  Serial.print(count1);
  Serial.print(' ');
  Serial.println(i);
  if (count1 >= 120) {
    if (state1 == 0) {
      if (++mode >= MODE_LAST) mode = 0;
      motor_end = t + (mode == 0 ? 500 : 200);
      state1 = 1;
    }
  } else if (count1 < 80) {
    state1 = 0;
  }
  
  digitalWrite(MOTOR, motor_end > t ? HIGH : LOW);
  
  if (prevMode != mode) {
    if (mode == MODE_TEXT) {
      msg[0] = 'H';
      msg[1] = 'U';
      msg[2] = 'G';
      msg[3] = ' ';
      msg[4] = 'M';
      msg[5] = 'E';
      msg[6] = '!';
      msg[7] = 0;
      msgLen = 7;
      msgX = matrix.width(); // Start off right edge
      prevFrameTime = 0L;             // For animation timing
    } else if (mode == MODE_SOUND) {
      memset(sharedBuffer, 0, SHARED_BUFFER_SIZE);
      volCount  = 0;
      lvl       = 10;
      minLvlAvg = 0;
      maxLvlAvg = 512;
    } else if (mode == MODE_COUNTER) {
      count = EEPROM.read(EEPROM_COUNT) + (EEPROM.read(EEPROM_COUNT+1) << 8);
    }
    
    prevMode = mode;
  }

  if (mode == MODE_TEXT) {
    if((t - prevFrameTime) >= (1000L / FPS)) { // Handle scrolling
      matrix.fillScreen(0);
      matrix.setCursor(msgX, 0);
      matrix.print(msg);
      matrix.show();
      if(--msgX < (msgLen * -6)) msgX = matrix.width(); // We must repeat!
      prevFrameTime = t;
    }
  } else if (mode == MODE_SOUND) {
    height   = analogRead(MIC_PIN);                        // Raw reading from mic
    height   = abs(height - 512 - DC_OFFSET); // Center on zero
    height   = (height <= NOISE) ? 0 : (height - NOISE);             // Remove noise/hum
    lvl = ((lvl * 7) + height) >> 3;    // "Dampened" reading (else looks twitchy)

    vol[volCount] = height;                 // Save sample for dynamic leveling
    if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
   
    // Calculate bar height based on dynamic min/max levels (fixed point):
    height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

    if(height < 0L)       height = 0;      // Clip output
    else if(height > TOP) height = TOP;

    // Color pixels based on rainbow gradient
    matrix.drawFastVLine(12+currentColumn,0,TOP-height,0);
    matrix.drawFastVLine(12+currentColumn,TOP-height,height,Wheel(map(height,0,TOP,30,150)));
    matrix.show();
    if (++currentColumn >= 10) currentColumn = 0;

    // Get volume range of prior frames
    minLvl = maxLvl = vol[0];

    for(i=1; i<SAMPLES; i++) {
      if(vol[i] < minLvl)      minLvl = vol[i];
      else if(vol[i] > maxLvl) maxLvl = vol[i];
    }

    // minLvl and maxLvl indicate the volume range over prior frames, used
    // for vertically scaling the output graph (so it looks interesting
    // regardless of volume level).  If they're too close together though
    // (e.g. at very low volume levels) the graph becomes super coarse
    // and 'jumpy'...so keep some minimum distance between them (this
    // also lets the graph go to zero when no sound is playing):
    if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
    minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
    maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)

  } else if (mode == MODE_COUNTER) {
    sprintf(msg, "%d", count);
    msgLen = strlen(msg);
    msgLen = (NEO_WIDTH - 5 * msgLen) >> 1;
    matrix.fillScreen(0);
    matrix.setCursor(msgLen - 1, 0);
    matrix.print(msg);
    matrix.show();
  }
  
  // For some reason there needs to be a delay here.
  delay(1);
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

