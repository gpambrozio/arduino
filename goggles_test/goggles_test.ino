// Googly Eye Goggles
// By Bill Earl
// For Adafruit Industries
//
// The googly eye effect is based on a physical model of a pendulum.
// The pendulum motion is driven by accelerations in 2 axis.
// Eye color varies with orientation of the magnetometer

#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define neoPixelPin 3

// We could do this as 2 16-pixel rings wired in parallel.
// But keeping them separate lets us do the right and left
// eyes separately if we want.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(32, neoPixelPin, NEO_GRB + NEO_KHZ800);

float pos = 8;  // Starting center position of pupil

#define halfWidth  1.25 // half-width of pupil (in pixels)

void setup(void)
{
   Serial.begin(9600);
   strip.begin();
   strip.setBrightness(30);
   strip.show(); // Initialize all pixels to 'off'  sensor_t sensor;

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
  if (Serial.available()) {
    pos = Serial.parseFloat();
  }
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
         color = strip.Color(0,0,255);
         Serial.print("ON ");
         Serial.print(proximity);
         Serial.print(" ");
         Serial.println(i);
         
         // do both eyes
         setPixelColor(i, color);
         setPixelColor(i + 16, color);
      }
      else // all others are off
      {
         setPixelColor(i, 0);
         setPixelColor(i + 16, 0);
      }
   }
   // Now show it!
   strip.show();
}

