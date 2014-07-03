/*
PICCOLO is a tiny Arduino-based audio visualizer...a bit like
 Spectro, but smaller, with microphone input rather than line-in.
 
 Hardware requirements:
 - Most Arduino or Arduino-compatible boards (ATmega 328P or better).
 - Adafruit Bicolor LED Matrix with I2C Backpack (ID: 902)
 - Adafruit Electret Microphone Amplifier (ID: 1063)
 - Optional: battery for portable use (else power through USB)
 Software requirements:
 - elm-chan's ffft library for Arduino
 
 Connections:
 - 3.3V to mic amp+ and Arduino AREF pin <-- important!
 - GND to mic amp-
 - Analog pin 0 to mic amp output
 - +5V, GND, SDA (or analog 4) and SCL (analog 5) to I2C Matrix backpack
 
 Written by Adafruit Industries.  Distributed under the BSD license --
 see license.txt for more information.  This paragraph must be included
 in any redistribution.
 
 ffft library is provided under its own terms -- see ffft.S for specifics.
 */

// IMPORTANT: FFT_N should be #defined as 128 in ffft.h.  This is different
// than Spectro, which requires FFT_N be 64 in that file when compiling.

#define LEDS 16
#define COPY 2

//#define SINGLE_COLOR_STRIP
//#define BIG_LEDS
//#define TOTAL_CONTROL
#define NEOPIXEL

#include <avr/pgmspace.h>
#include <ffft.h>
#include <math.h>
#include <SoftwareSerial.h> //this is a must

#ifdef SINGLE_COLOR_STRIP

#include "RGBdriver.h"

RGBdriver Driver(9, 10);

#endif

#ifdef BIG_LEDS

#include "SPI.h"
#include "Adafruit_WS2801.h"

#define DATA_PIN 11
#define CLOCK_PIN 13

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(LEDS*COPY, DATA_PIN, CLOCK_PIN);

// Create a 24 bit color value from R,G,B
#define color(r, g, b) (((uint32_t)r)<<16|((uint32_t)g)<<8|(uint32_t)b)

#endif

#ifdef TOTAL_CONTROL

#include <SPI.h>
#include <TCL.h>

#endif

#ifdef NEOPIXEL

#include <Adafruit_NeoPixel.h>

#define PIN 12

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS*COPY, PIN, NEO_GRB + NEO_KHZ800);

#define color(r,g,b) strip.Color(r,g,b)

#endif

// Microphone connects to Analog Pin 0.  Corresponding ADC channel number
// varies among boards...it's ADC0 on Uno and Mega, ADC7 on Leonardo.
// Other boards may require different settings; refer to datasheet.
#ifdef __AVR_ATmega32U4__
#define ADC_CHANNEL  13
#else
#define ADC_CHANNEL 0
#endif

#define MAX_COLS (LEDS*3)
#define AVG_COUNT 3

int16_t       capture[FFT_N];    // Audio capture buffer
complex_t     bfly_buff[FFT_N];  // FFT "butterfly" buffer
uint16_t      spectrum[FFT_N/2]; // Spectrum output buffer
volatile byte samplePos = 0;     // Buffer position counter

byte
finalValue[MAX_COLS],
colCount = 0; // Frame counter for storing past column data

int
col[MAX_COLS][AVG_COUNT],   // Column levels for the prior 10 frames
minLvlAvg[MAX_COLS], // For dynamic adjustment of low & high ends of graph,
maxLvlAvg[MAX_COLS] // pseudo rolling averages for the prior few frames.
//colDiv[MAX_COLS]    // Used when filtering FFT output to 8 columns
;

/*
These tables were arrived at through testing, modeling and trial and error,
 exposing the unit to assorted music and sounds.  But there's no One Perfect
 EQ Setting to Rule Them All, and the graph may respond better to some
 inputs than others.  The software works at making the graph interesting,
 but some columns will always be less lively than others, especially
 comparing live speech against ambient music of varying genres.
 */
PROGMEM uint8_t
// This is low-level noise that's subtracted from each FFT output column:
noise[64]={
  8,6,6,5,3,4,4,4,3,4,4,3,2,3,3,4,
  2,1,2,1,3,2,3,2,1,2,3,1,2,3,4,4,
  3,2,2,2,2,2,2,1,3,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,4}
,
// These are scaling quotients for each FFT output column, sort of a
// graphic EQ in reverse.  Most music is pretty heavy at the bass end.
eq[64]={
  255,175,218,225,220,198,147, 99, 68, 47, 33, 22, 14,  8,  4,  2,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
//,
// When filtering down to 8 columns, these tables contain indexes
// and weightings of the FFT spectrum output values to use.  Not all
// buckets are used -- the bottom-most and several at the top are
// either noisy or out of range or generally not good for a graph.
//  col0data[] = {  2,  1,  // # of spectrum bins to merge, index of first
//    111,   8 },           // Weights for each bin
//  col1data[] = {  4,  1,  // 4 bins, starting at index 1
//     19, 186,  38,   2 }, // Weights for 4 bins.  Got it now?
//  col2data[] = {  5,  2,
//     11, 156, 118,  16,   1 },
//  col3data[] = {  8,  3,
//      5,  55, 165, 164,  71,  18,   4,   1 },
//col4data[] = { 
//  11,  5,
//  3,  24,  89, 169, 178, 118,  54,  20,   6,   2,   1 }
//,
//col5data[] = { 
//  17,  7,
//  2,   9,  29,  70, 125, 172, 185, 162, 118, 74,
//  41,  21,  10,   5,   2,   1,   1 }
//,
//col6data[] = { 
//  25, 11,
//  1,   4,  11,  25,  49,  83, 121, 156, 180, 185,
//  174, 149, 118,  87,  60,  40,  25,  16,  10,   6,
//  4,   2,   1,   1,   1 }
//,
//  col7data[] = { 37, 16,
//      1,   2,   5,  10,  18,  30,  46,  67,  92, 118,
//    143, 164, 179, 185, 184, 174, 158, 139, 118,  97,
//     77,  60,  45,  34,  25,  18,  13,   9,   7,   5,
//      3,   2,   2,   1,   1,   1,   1 },
// And then this points to the start of the data for each of the columns:
//  *colData[] = {
//    col0data, col1data, col2data, col3data,
//    col4data, col5data, col6data, col7data };
//*colData[] = {
//  col4data, col5data, col7data }
;

#ifdef NEOPIXEL || BIG_LEDS

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

#endif

void setup() {
//  Serial.begin(9600);
//  Serial.println("begin");

#ifdef NEOPIXEL || BIG_LEDS
  strip.begin();
  colorWipe(color(0,0,0));
  strip.show(); // Initialize all pixels to 'off'
#endif

#ifdef TOTAL_CONTROL
  TCL.begin();
  TCL.setAll(LEDS*2,0,0,0);
#endif

  uint8_t i, j, nBins, binNum, *data;

  memset(col , 0, sizeof(col));

  for(i=0; i<MAX_COLS; i++) {
    minLvlAvg[i] = 0;
    maxLvlAvg[i] = 512;
//    data         = (uint8_t *)pgm_read_word(&colData[i]);
//    nBins        = pgm_read_byte(&data[0]) + 2;
//    binNum       = pgm_read_byte(&data[1]);
//    for(colDiv[i]=0, j=2; j<nBins; j++)
//      colDiv[i] += pgm_read_byte(&data[j]);
  }

  // Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion 
#if ADC_CHANNEL < 8
  ADMUX  = _BV(REFS0) | ADC_CHANNEL; // Channel sel, right-adj, use AREF pin
#else
  ADMUX  = _BV(REFS0) | (ADC_CHANNEL-8); // Channel sel, right-adj, use AREF pin
#endif
  ADCSRA = _BV(ADEN)  | // ADC enable
  _BV(ADSC)  | // ADC start
  _BV(ADATE) | // Auto trigger
  _BV(ADIE)  | // Interrupt enable
  _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
#if ADC_CHANNEL < 8
  ADCSRB = 0;                // Free run mode, no high MUX bit
  DIDR0  = 1 << ADC_CHANNEL; // Turn off digital input for ADC pin
#else
  ADCSRB = B100000;
  DIDR2  = 1 << (ADC_CHANNEL-8);
#endif

#ifdef TIMSK0
  TIMSK0 = 0;                // Timer0 off
#else
  TIMSK = 0;                // Timer0 off
#endif
  sei(); // Enable interrupts
}

void loop() {
  uint8_t  i, x, L, *data, nBins, binNum, weighting, c;
  uint16_t minLvl, maxLvl;
  int      level, sum;

  while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish

//  for(x=0; x<FFT_N; x++) {
//     Serial.print(capture[x]);
//     Serial.print(" ");
//  }
//  Serial.println("");
  fft_input(capture, bfly_buff);   // Samples -> complex #s
  samplePos = 0;                   // Reset sample counter
  ADCSRA |= _BV(ADIE);             // Resume sampling interrupt
  fft_execute(bfly_buff);          // Process complex data
  fft_output(bfly_buff, spectrum); // Complex -> spectrum


  // Remove noise and apply EQ levels
  for(x=0; x<FFT_N/2; x++) {
    L = pgm_read_byte(&noise[x]) + 3;
    spectrum[x] = (spectrum[x] <= L) ? 0 : (((spectrum[x] - L) * (256L - pgm_read_byte(&eq[x]))) >> 8);
  }

  // Downsample spectrum output to 8 columns:
  for(x=0; x<MAX_COLS; x++) {
    col[x][colCount] = spectrum[x+10];
    minLvl = maxLvl = col[x][0];
    for(i=1; i<AVG_COUNT; i++) { // Get range of prior 10 frames
      if(col[x][i] < minLvl)      minLvl = col[x][i];
      else if(col[x][i] > maxLvl) maxLvl = col[x][i];
    }
    // minLvl and maxLvl indicate the extents of the FFT output, used
    // for vertically scaling the output graph (so it looks interesting
    // regardless of volume level).  If they're too close together though
    // (e.g. at very low volume levels) the graph becomes super coarse
    // and 'jumpy'...so keep some minimum distance between them (this
    // also lets the graph go to zero when no sound is playing):
    if((maxLvl - minLvl) < 8) maxLvl = minLvl + 8;
    minLvlAvg[x] = (minLvlAvg[x] * 7 + minLvl) >> 3; // Dampen min/max levels
    maxLvlAvg[x] = (maxLvlAvg[x] * 7 + maxLvl) >> 3; // (fake rolling average)

    // Second fixed-point scale based on dynamic min/max levels:
    level = 255L * (col[x][colCount] - minLvlAvg[x]) /
      (long)(maxLvlAvg[x] - minLvlAvg[x]);

    // Clip output and convert to byte:
    if(level < 0L)       c = 0;
    else if(level > 255) c = 255; // Allow dot to go a couple pixels off top
    else                 c = (uint8_t)level;

    finalValue[x] = c>>2;
//    finalValue[x] = spectrum[x+10];
  }
#ifdef TOTAL_CONTROL
  TCL.sendEmptyFrame();
  for(i=0; i<COPY ;i++) {
    for(x=0; x<LEDS; x++) {
      TCL.sendColor(finalValue[x*2],finalValue[x*2+1],0);
    }
  }
  TCL.sendEmptyFrame();
#else
  for(x=0; x<LEDS; x++) {
    for(i=0; i<COPY ;i++) {
      strip.setPixelColor(x+i*LEDS, color(finalValue[x*2], finalValue[x*2+1], 0));
    }
  }
#endif
  //Serial.println("OK");
#ifdef SINGLE_COLOR_STRIP

  Driver.begin();
  Driver.SetColor(finalValue[0], finalValue[1], finalValue[2]); //Red. first node data
  Driver.end();

#else
#ifdef NEOPIXEL || BIG_LEDS
//  strip.setPixelColor(currentLed, strip.Color(finalValue[0], finalValue[1], finalValue[2]));
//  strip.show();
//  if (++currentLed >= strip.numPixels()) currentLed = 0;
//
//  colorWipe(strip.Color(finalValue[0], finalValue[1], finalValue[2]));
  strip.show();
  
#endif
#endif
  if(++colCount >= AVG_COUNT) colCount = 0;
}

ISR(ADC_vect) { // Audio-sampling interrupt
#define noiseThreshold  4
  int16_t sample = ADC; // 0-1023

  capture[samplePos] =
    ((sample > (512-noiseThreshold)) &&
    (sample < (512+noiseThreshold))) ? 0 :
  sample - 512; // Sign-convert for FFT; -512 to +511

  if(++samplePos >= FFT_N) ADCSRA &= ~_BV(ADIE); // Buffer full, interrupt off
}


