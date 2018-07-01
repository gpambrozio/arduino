/*
PICCOLO is a tiny Arduino-based audio visualizer.

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

// IMPORTANT: FFT_N should be #defined as 128 in ffft.h.

#include <math.h>
#include <Wire.h>

#define ADC_CHANNEL 4

volatile byte samplePos = 0;     // Buffer position counter
uint32_t sampleSum = 0;
uint32_t runningSum = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Cane controller"); Serial.println("");

  // Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion 
  ADMUX  = _BV(REFS0) | ADC_CHANNEL; // Channel sel, right-adj, use AVcc pin
  ADCSRA = _BV(ADEN)  | // ADC enable
           _BV(ADSC)  | // ADC start
           _BV(ADATE) | // Auto trigger
           _BV(ADIE)  | // Interrupt enable
           _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
  ADCSRB = 0;                // Free run mode, no high MUX bit
  DIDR0  = 1 << ADC_CHANNEL; // Turn off digital input for ADC pin

  sei(); // Enable interrupts
}

void loop() {

  while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish

  uint32_t thisSum = sampleSum >> 7;
  sampleSum = 0;
  samplePos = 0;                   // Reset sample counter
  ADCSRA |= _BV(ADIE);             // Resume sampling interrupt

  runningSum = ((runningSum * 7) + thisSum) >> 3;

  Serial.print("Avg "); Serial.println(runningSum);
}

ISR(ADC_vect) { // Audio-sampling interrupt
  int16_t sample = ADC; // 0-1023
  sampleSum += sample;
  if(++samplePos >= 128) ADCSRA &= ~_BV(ADIE); // Buffer full, interrupt off
}

