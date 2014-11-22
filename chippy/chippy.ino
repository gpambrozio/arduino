#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define PIN    3
#define BUTTON 2

typedef enum {
  ModeChippyRandom = 0,
  ModeRandom,
  ModeChippy,
  ModeGravity,
  ModeAntiGravity,
  ModeRainbow,
  ModeRainbowCycle,
  ModeTheaterChaseRainbow,
  ModeBounce,
  ModeJet,
  ModeOff,
  ModeCount
} Mode;

Mode currentMode = ModeChippyRandom;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);

// Pi for calculations - not the raspberry type
#define  Pi  3.14159
#define increment (2 * Pi / 16) // distance between pixels in radians

float pos = 8;  // Starting center position of pupil
float MomentumH = 0; // horizontal component of pupil rotational inertia
float MomentumV = 0; // vertical component of pupil rotational inertia

// Tuning constants. (a.k.a. "Fudge Factors)  
// These can be tweaked to adjust the liveliness and sensitivity of the eyes.
#define friction 0.995   // frictional damping constant.  1.0 is no friction.
#define swing    60.0    // arbitrary divisor for gravitational force
#define gravity  200.0   // arbitrary divisor for lateral acceleration

#define halfWidth  1.25 // half-width of pupil (in pixels)

void setup() {
  pinMode(A1, INPUT_PULLUP);

  power_timer1_disable();    // Disable unused peripherals
  power_adc_disable();       // to save power
  attachInterrupt(0, wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function
                                      // wakeUpNow when pin 2 gets LOW 
  // Initialize the sensors
  accel.begin();
  mag.begin();

  strip.begin();
  strip.setBrightness(20, 32, 120);
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
    case ModeChippyRandom: 
      chippyRandom(100, counter++);
      if (counter >= 256) counter = 0;
      break;
   
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
      
    case ModeGravity:
    case ModeAntiGravity:
    {
      sensors_event_t event; 
      accel.getEvent(&event);

      // apply a little frictional damping to keep things in control and prevent perpetual motion
      MomentumH *= friction;
      MomentumV *= friction;
      
      // Calculate the horizontal and vertical effect on the virtual pendulum
      // 'pos' is a pixel address, so we multiply by 'increment' to get radians.
      float TorqueH = cos(pos * increment);  // peaks at top and bottom of the swing
      float TorqueV = sin(pos * increment);    // peaks when the pendulum is horizontal
      
      // Add the incremental acceleration to the existing momentum
      // This code assumes that the accelerometer is mounted upside-down, level
      // and with the X-axis pointed forward.  So the Y axis reads the horizontal
      // acceleration and the inverse of the Z axis is gravity.
      // For other orientations of the sensor, just change the axis to match.
       
      // This is horizontal acceleration
      MomentumH -= TorqueH * event.acceleration.x / swing;
       
      // Below if vertical acceleration (gravity)
      MomentumV += TorqueV * event.acceleration.y / gravity;
      
      // Calculate the new position
      pos += MomentumH + MomentumV;
       
      // handle the wrap-arounds at the top
      while (round(pos) < 0) pos += 16.0;
      while (round(pos) > 15) pos -= 16.0;

      if (++counter >= 1024) counter = 0;

      // Now re-compute the display
      for (int i = 0; i < 16; i++) {
         // Compute the distance bewteen the pixel and the center
         // point of the virtual pendulum.
         float diff = fabs((float)i - pos);
         if (diff > 8.0) diff = 16 - diff;  // wrap around

         // Light up nearby pixels proportional to their proximity to 'pos'
         if (diff <= halfWidth)  {
            float proximity = (halfWidth - diff) * 200;

            // pick a color based on heading & proximity to 'pos'
            strip.setPixelColor(currentMode == ModeAntiGravity ? i : i < 8 ? i + 8 : i - 8, Wheel(counter >> 2, proximity));
         } else {
           // all others are off 
           strip.setPixelColor(currentMode == ModeAntiGravity ? i : i < 8 ? i + 8 : i - 8, 0);
         }
      }
      strip.show();
      delay(1);
      break;
    }
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

void chippyRandom(uint8_t wait, uint16_t counter) {
  uint16_t j = counter;

  for(uint16_t i=0; i< strip.numPixels(); i++) {
    if (i < 2 || i > 5)
      strip.setPixelColor(i, Wheel(random(256)));
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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint16_t WheelPos, byte proximity) {
  float p = (float)proximity / 255.0;
  if(WheelPos < 85) {
   return strip.Color(p * WheelPos * 3, p *(255 - WheelPos * 3), 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(p *(255 - WheelPos * 3), 0, p * WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, p * WheelPos * 3, p * (255 - WheelPos * 3));
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

