#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define PIN    3
#define BUTTON 2

typedef enum {
  ModeChippy = 0,
  ModeRainbow,
  ModeRainbowCycle,
  ModeTheaterChaseRainbow,
  ModeBounce,
  ModeJet,
  ModeRandom,
  ModeOff,
  ModeCount
} Mode;

Mode currentMode = ModeChippy;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  pinMode(A1, INPUT_PULLUP);

  power_timer1_disable();    // Disable unused peripherals
  power_adc_disable();       // to save power
  attachInterrupt(0, wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function
                                      // wakeUpNow when pin 2 gets LOW 
  strip.begin();
  strip.setBrightness(32, 32, 100);
  strip.show(); // Initialize all pixels to 'off'
}

unsigned long buttonStart = 0;
uint16_t counter;

void loop() {
  if (digitalRead(BUTTON) == LOW && millis() > buttonStart + 500) {
    buttonStart = millis();
    currentMode = (Mode)((int)currentMode + 1);
    if (currentMode >= ModeCount) currentMode = (Mode)0;
    counter = 0;
    randomSeed(millis());
  }

  switch(currentMode) {
    case ModeChippy:
      chippy(15, counter++);
      if (counter >= 256) counter = 0;
      break;
      
    case ModeRainbow:
      rainbow(15, counter++);
      if (counter >= 256) counter = 0;
      break;
      
    case ModeRainbowCycle:
      rainbowCycle(15, counter++);
      if (counter >= 256 * 5) counter = 0;
      break;
      
    case ModeTheaterChaseRainbow:
      theaterChaseRainbow(50, counter++);
      if (counter >= 256 * 3) counter = 0;
      break;
      
    case ModeBounce:
      bounceRainbow(50, counter++);
      if (counter >= 256 * strip.numPixels()) counter = 0;
      break;
      
    case ModeJet:
      jetRainbow(50, counter++);
      if (counter >= 128 * strip.numPixels()) counter = 0;
      break;

    case ModeRandom:
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(random(256)));
      }
      strip.show();
      delay(100);
      break;
      
    case ModeOff:
      for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
      strip.show();
      while (digitalRead(BUTTON) == LOW) delay(10);
      delay(10);
      sleepNow();
      currentMode = (Mode)0;

      break;
  }
}

void rainbow(uint8_t wait, uint16_t counter) {
  uint16_t j = counter;

  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i+j) & 255));
  }
  strip.show();
  delay(wait);
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, uint16_t counter) {
  uint16_t j = counter;

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
  delay(wait);
}

void chippy(uint8_t wait, uint16_t counter) {
  uint16_t j = counter;

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    if (i < 2 || i > 5)
      strip.setPixelColor(i, Wheel(((i * 256 / (strip.numPixels()-5)) + j) & 255));
    else 
      strip.setPixelColor(i, 0);
  }
  strip.show();
  delay(wait);
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait, uint16_t counter) {
  uint16_t q = counter % 3;
  uint16_t j = counter / 3;
  for (int i=0; i < strip.numPixels(); i=i+3) {
    strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
  }
  strip.show();
 
  delay(wait);
 
  for (int i=0; i < strip.numPixels(); i=i+3) {
    strip.setPixelColor(i+q, 0);        //turn every third pixel off
  }
}

void bounceRainbow(uint8_t wait, uint16_t counter) {
  uint16_t j = counter / strip.numPixels();
  uint16_t q = counter % strip.numPixels();

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    if (i == q || (strip.numPixels() - i) == q)
      strip.setPixelColor(i, Wheel(((q * 256 / strip.numPixels()) + j) & 255));
    else
      strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(wait);
}

void jetRainbow(uint8_t wait, uint16_t counter) {
  uint16_t j = counter * 2 / strip.numPixels();
  uint16_t q = counter % (strip.numPixels() / 2);

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    if (i == q || (strip.numPixels() - i - 1) == q)
      strip.setPixelColor(i, Wheel(((q * 256 / strip.numPixels()) + j) & 255));
    else
      strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(wait);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void sleepNow()         // here we put the arduino to sleep
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep modus.
     *
     * In the avr/sleep.h file, the call names of these sleep modus are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, 
     * so we choose the according sleep modus: SLEEP_MODE_PWR_DOWN
     * 
     */  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

    sleep_enable();              // enables the sleep bit in the mcucr register
                                 // so sleep is possible. just a safety pin 

    /* Now is time to enable a interrupt. we do it here so an 
     * accidentally pushed interrupt button doesn't interrupt 
     * our running program. if you want to be able to run 
     * interrupt code besides the sleep function, place it in 
     * setup() for example.
     * 
     * In the function call attachInterrupt(A, B, C)
     * A   can be either 0 or 1 for interrupts on pin 2 or 3.   
     * 
     * B   Name of a function you want to execute at interrupt for A.
     *
     * C   Trigger mode of the interrupt pin. can be:
     *             LOW        a low level triggers
     *             CHANGE     a change in level triggers
     *             RISING     a rising edge of a level triggers
     *             FALLING    a falling edge of a level triggers
     *
     * In all but the IDLE sleep modes only LOW can be used.
     */

    attachInterrupt(0, wakeUpNow, LOW);// use interrupt 0 (pin 2) and run function
                                       // wakeUpNow when pin 2 gets LOW 

    sleep_mode();                // here the device is actually put to sleep!!
                                 // 

    sleep_disable();             // first thing after waking from sleep:
                                 // disable sleep...
    detachInterrupt(0);          // disables interrupt 0 on pin 2 so the 
                                 // wakeUpNow code will not be executed 
                                 // during normal running time.
    while (digitalRead(BUTTON) == LOW) delay(10);
    delay(10);
}

void wakeUpNow()        // here the interrupt is handled after wakeup 
{
  //execute code here after wake-up before returning to the loop() function
  // timers and code using timers (serial.print and more...) will not work here.
  digitalWrite(BUTTON, HIGH);
}

