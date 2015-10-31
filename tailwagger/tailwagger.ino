/*
Simple servo tail wagger for Adafruit 5V Trinket (not Pro) microcontroller.
Uses servo on pin 0.  The tail has no 'tendons' -- it's a passive thing,
simply hanging off the servo -- though a weak 'spine' (such as aquarium tube)
adds just enough body to help.  Pendulum math is then used to induce a
reasonable wag effect.

To break up the repetition and appear a little more 'alive,' the speed,
magnitude and duration of the tail wag is randomized (within certain ranges),
and it periodically settles down and stops (adds variety and also saves some
battery).  There's an optimal period (single-swing time) for a given tail
length, but it may randomly go a little faster or slower than this to add
some 'english' to the wag.

You'll need to calibrate this, editing a few lines below.  TAIL_LENGTH is the
length of the tail in meters (e.g. a 40 cm tail is 0.4 meters); for inches,
multiply by 2.54 to get centimeters, then divide by 100 for meters.
SERVO_MIN and SERVO_MAX are the pulse times (in microseconds) for the leftmost
and rightmost servo positions; though nominally these are 1000 and 2000 usec
(1.0 to 2.0 milliseconds), every servo in reality is a little different, and
you'll need to tune these values for your actual desired swing range.
*/

#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif

// CONFIGURABLE STUFF --------------------------------------------------------

#define TAIL_LENGTH 0.4  // Nominal tail length (meters)
#define SERVO_PIN   0    // Servo is connected here
#define SERVO_MIN   500  // Servo pulse times
#define SERVO_MAX   1800 // in microseconds

// Tail cycles through four states: off, ramp up, steady wag, ramp down.
// Durations are semi-random; this table sets min & max times for each.
static const uint8_t PROGMEM modeTime[4][2] = {
  { 4,  9 }, // 4 to 9 second off time
  { 3,  6 }, // 3 to 6 second ramp up
  { 4, 12 }, // 4 to 12 sec steady wag
  { 2,  5 }  // 2 to 5 sec ramp down
};

// SHOULDN'T NEED TO EDIT BELOW THIS LINE ------------------------------------

#define SERVO_RANGE    (SERVO_MAX - SERVO_MIN)
#define MODE_OFF       0
#define MODE_RAMP_UP   1
#define MODE_HOLD      2
#define MODE_RAMP_DOWN 3

uint8_t  tailMode      = MODE_OFF;
float    wagnitude     = 0.8, // Magnitude of current wag cycle
         period        = M_PI * 2.0 * sqrt(TAIL_LENGTH / 9.8);
uint32_t modeStartTime = 0,
         modeDuration  = 0,
         lastPulseTime = 0;

// SETUP just configures prescaler & enables servo output --------------------

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // 16 MHz Trinket (not Pro) requires this
#endif
  pinMode(SERVO_PIN, OUTPUT);
  randomSeed(analogRead(1));
}

// LOOP does all the tail-waggling math --------------------------------------

void loop() {
  uint32_t t = millis(); // Elapsed time, milliseconds

  // Compare time in current mode against planned duration
  if((t - modeStartTime) > modeDuration) { // Time's up!
    for(;;) {
      if(++tailMode > MODE_RAMP_DOWN)      // Cycle to next mode,
        tailMode = MODE_OFF;               // wrap if needed
      modeDuration = 1000 * random(        // Randomize mode duration
        pgm_read_byte(&modeTime[tailMode][0]),
        pgm_read_byte(&modeTime[tailMode][1]));
      if(tailMode != MODE_OFF) break;      // If 'off' mode...
      // Randomize magnitude of next wag cycle (70% to 100%)
      wagnitude = (float)random(7, 10) / 10.0;
      // Randomize tail length for next wag cycle (60% to 120%)
      float len = TAIL_LENGTH * (float)random(6, 12) / 10.0;
      // Solve for period (sec) from length (meters):
      period = M_PI * 2.0 * sqrt(len / 9.8);
      delay(modeDuration);                 //   Stop servo,
      t = millis();                        //   revise time and
    }                                      //   mode-cycle again
    modeStartTime = t;                     // Save mode start time
  }

  // Calc amplitude of wag at current time (ramping up/down/steady)
  float a = wagnitude; // Assume MODE_HOLD
  if(tailMode != MODE_HOLD) {
    a = wagnitude * (float)(t - modeStartTime) / (float)modeDuration;
    if(tailMode == MODE_RAMP_DOWN) a = wagnitude - a;
  }

  uint16_t servoPulseLength = SERVO_MIN + (int)((float)SERVO_RANGE *
    ((sin(
    ((float)t / 1000.0)   // Current time in seconds
    / period * M_PI * 2.0 // Seconds to wag cycles
    ) * a)                // Sine wave * amplitude (-1.0 to 1.0)
    + 1.0) * 0.5);        // Convert to integer servo usec pulse

  // Handle servo pulse @ 50 Hz...
  while(((t = micros()) - lastPulseTime) < 20000); // Wait for it...
  digitalWrite(SERVO_PIN, HIGH);
  delayMicroseconds(servoPulseLength);
  digitalWrite(SERVO_PIN, LOW);
  lastPulseTime = t;
}
    
    
