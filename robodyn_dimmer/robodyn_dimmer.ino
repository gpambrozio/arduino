#include "hw_timer.h"
const byte zcPin = 13;
const byte pwmPin = 12;

byte fade = 1;
byte state = 1;
byte tarBrightness = 255;
byte curBrightness = 0;
byte zcState = 0; // 0 = ready; 1 = processing;

void ICACHE_RAM_ATTR dimTimerISR();
void ICACHE_RAM_ATTR zcDetectISR();

void setup() {
  Serial.begin(115200);   
  pinMode(zcPin, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(zcPin, zcDetectISR, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  hw_timer_init(NMI_SOURCE, 0);
  hw_timer_set_func(dimTimerISR);
}

void loop() {
  // put your main code here, to run repeatedly:
    if (Serial.available()) {
        int val = Serial.parseInt();
        if (val>0){
          tarBrightness =val;
          Serial.println(tarBrightness);
        }        
    }
}

void ICACHE_RAM_ATTR dimTimerISR() {
    if (fade == 1) {
      if (curBrightness > tarBrightness || (state == 0 && curBrightness > 0)) {
        --curBrightness;
      }
      else if (curBrightness < tarBrightness && state == 1 && curBrightness < 255) {
        ++curBrightness;
      }
    }
    else {
      if (state == 1) {
        curBrightness = tarBrightness;
      }
      else {
        curBrightness = 0;
      }
    }
    
    if (curBrightness == 0) {
      state = 0;
      digitalWrite(pwmPin, 0);
    }
    else if (curBrightness == 255) {
      state = 1;
      digitalWrite(pwmPin, 1);
    }
    else {
      digitalWrite(pwmPin, 1);
    }
    
    zcState = 0;
}

void ICACHE_RAM_ATTR zcDetectISR() {
  if (zcState == 0) {
    zcState = 1;
  
    if (curBrightness < 255 && curBrightness > 0) {
      digitalWrite(pwmPin, 0);
      
      int dimDelay = 30 * (255 - curBrightness) + 400;//400
      hw_timer_arm(dimDelay);
    }
  }
}
