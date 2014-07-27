// Googly Eye Goggles
// By Bill Earl
// For Adafruit Industries
//
// The googly eye effect is based on a physical model of a pendulum.
// The pendulum motion is driven by accelerations in 2 axis.
// Eye color varies with orientation of the magnetometer

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>

#define neoPixelPin 3

// We could do this as 2 16-pixel rings wired in parallel.
// But keeping them separate lets us do the right and left
// eyes separately if we want.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(32, neoPixelPin, NEO_GRB + NEO_KHZ800);

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
#define nod      7.5     // accelerometer threshold for toggling modes

long nodStart = 0;
long nodTime = 1000;

#define MODE_NORMAL   0
#define MODE_ANTI     1
#define MODE_MIRROR   2
#define MODE_RAINBOW  3
#define MODE_OFF      4
#define MODE_COUNT    5

int mode = MODE_NORMAL;

bool antiGravity = false;  // The pendulum will anti-gravitate to the top.
bool mirroredEyes = false; // The left eye will mirror the right.

uint16_t j_rainbow;

#define halfWidth  1.25 // half-width of pupil (in pixels)

void setup(void)
{
   Serial.begin(9600);
   strip.begin();
   strip.setBrightness(30);
   strip.show(); // Initialize all pixels to 'off'  sensor_t sensor;

   // Initialize the sensors
   accel.begin();
   mag.begin();
   
   j_rainbow = 0;
   resetModes();
}

void setPixelColor(int i, uint32_t color)
{
  if (i < 15)
  {
    i++;
  }
  else if (i < 16)
  {
    i = 15 - i;
  }
  else if (i < 31)
  {
    i++;
  }
  else
  {
    i = 31 + 16 - i;
  }
  strip.setPixelColor(i, color);
}

// main processing loop
void loop(void) 
{
   // Read the magnetometer and determine the compass heading:
   sensors_event_t event; 
   mag.getEvent(&event);

   // Calculate the angle of the vector y,x from magnetic North
   float heading = (atan2(event.magnetic.y,event.magnetic.x) * 180) / Pi;

   // Normalize to 0-360 for a compass heading
   if (heading < 0)
   {
      heading = 360 + heading;
   }

   // Now read the accelerometer to control the motion.
   accel.getEvent(&event);

   // Check for mode change commands
   CheckForNods(event);
   switch (mode)
   {
     case MODE_NORMAL:
       antiGravity = false;  
       mirroredEyes = false;
       break;
     case MODE_ANTI:
       antiGravity = true;  
       mirroredEyes = false;
       break;
     case MODE_MIRROR:
       antiGravity = false;  
       mirroredEyes = true;
       break;
     case MODE_RAINBOW:
       rainbowCycle(2, j_rainbow);
       if (++j_rainbow >= 256*5) j_rainbow = 0;
       break;
     case MODE_OFF:
      for(uint16_t i=0; i< strip.numPixels(); i++) {
        setPixelColor(i, 0);
      }
      break;
   }

   if (mode != MODE_RAINBOW && mode != MODE_OFF)
   {
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
     MomentumH -= TorqueH * event.acceleration.z / swing;
     
     // Below if vertical acceleration (gravity)
     if (antiGravity)
     {
       MomentumV -= TorqueV * event.acceleration.x / gravity;
     }
     else
     {
       MomentumV += TorqueV * event.acceleration.x / gravity;
     }
  
     // Calculate the new position
     pos += MomentumH + MomentumV;
     
     // handle the wrap-arounds at the top
     while (round(pos) < 0) pos += 16.0;
     while (round(pos) > 15) pos -= 16.0;
  
     // Now re-compute the display
     Serial.println(pos);
     for (int i = 0; i < 16; i++)
     {
        // Compute the distance bewteen the pixel and the center
        // point of the virtual pendulum.
        float diff = fabs((float)i - pos);
        if (diff > 8.0) diff = 16 - diff;  // wrap around

        // Light up nearby pixels proportional to their proximity to 'pos'
        if (diff <= halfWidth) 
        {
           uint32_t color;
           float proximity = (halfWidth - diff) * 200;
  
           // pick a color based on heading & proximity to 'pos'
           color = selectColor(heading, proximity);
           
           // do both eyes
           setPixelColor(i, color);
           if (mirroredEyes)
           {
             setPixelColor(31 - i, color);
           }
           else
           {
             setPixelColor(i + 16, color);
           }
        }
        else // all others are off
        {
           setPixelColor(i, 0);
           if (mirroredEyes)
           {
             setPixelColor(31 - i, 0);
           }
           else
           {
             setPixelColor(i + 16, 0);
           }
        }
     }
   }
   // Now show it!
   strip.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, uint16_t j) {
  uint16_t i;

  for(i=0; i< strip.numPixels()/2; i++) {
    setPixelColor(i, Wheel(((i * 512 / strip.numPixels()) + j) & 255));
    setPixelColor(strip.numPixels()/2+i, Wheel(((i * 512 / strip.numPixels()) + j) & 255));
  }
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

// choose a color based on the compass heading and proximity to "pos".
uint32_t selectColor(float heading, float proximity)
{
     uint32_t color;

     // Choose eye color based on the compass heading
     if (heading < 60)
     {
        color = strip.Color(0, 0, proximity);
     }
     else if (heading < 120)
     {
        color = strip.Color(0, proximity, proximity);
     }
     else if (heading < 180)
     {
        color = strip.Color(0, proximity, 0);
     }
     else if (heading < 240)
     {
        color = strip.Color(proximity, proximity, 0);
     }
     else if (heading < 300)
     {
        color = strip.Color(proximity, 0, 0);
     }
     else // 300-360
     {
        color = strip.Color(proximity, 0, proximity);
     }
}

// monitor orientation for mode-change 'gestures'
void CheckForNods(sensors_event_t event)
{
   if (event.acceleration.y > nod)
   {
     if (millis() - nodStart > nodTime)
     {
       mode = 0;
       nodStart = millis(); // reset timer     
       spinDown();
     }
   }
   else if (event.acceleration.y < -(nod + 1))
   {
     if (millis() - nodStart > nodTime)
     {
       mode++;
       if (mode >= MODE_COUNT) {
         mode = 0;
       }
       spinUp();
       nodStart = millis(); // reset timer     
     }
   }
   else if (event.acceleration.z > nod)
   {
     if (millis() - nodStart > nodTime)
     {
       mode = MODE_OFF;
       nodStart = millis(); // reset timer     
       spinDown();
     }
   }
   else // no nods in progress
   {
     nodStart = millis(); // reset timer
   }
}

// Reset to default
void resetModes()
{
   antiGravity = false;
   mirroredEyes = false;
   
   /// spin-up
   spin(strip.Color(255,0,0), 1, 500);
   spin(strip.Color(0,255,0), 1, 500);
   spin(strip.Color(0,0,255), 1, 500);
   spinUp();
}

// gradual spin up
void spinUp()
{
   for (int i = 300; i > 0;  i -= 20)
   {
     spin(strip.Color(255,255,255), 1, i);
   }
   pos = 0;
   // leave it with some momentum and let it 'coast' to a stop
   MomentumH = 3;  
}

// Gradual spin down
void spinDown()
{
   for (int i = 1; i < 300; i++)
   {
     spin(strip.Color(255,255,255), 1, i += 20);
   }
   // Stop it dead at the top and let it swing to the bottom on its own
   pos = 0;
   MomentumH = MomentumV = 0;
}


// utility function for feedback on mode changes.
void spin(uint32_t color, int count, int time)
{
  for (int j = 0; j < count; j++)
  {
    for (int i = 0; i < 16; i++)
    {
      setPixelColor(i, color);
      setPixelColor(31 - i, color);
      strip.show();
      delay(max(time / 16, 1));
      setPixelColor(i, 0);
      setPixelColor(31 - i, 0);
      strip.show();
    }
  }
}
