/*****************************************************************************
 * rainbow.ino
 *
 * This example will send a sequence of rainbow colors down a 25 pixel long
 * strand of total control lighting. The Arduino will send an update every
 * second to the strand, although this is unnecessary and if you disconnect
 * the Arduino from the strand, you will see that as long as the lights have
 * power, they will retain the last color information sent to them.
 *
 * Copyright 2011 Christopher De Vries
 * This program is distributed under the Artistic License 2.0, a copy of which
 * is included in the file LICENSE.txt along with this library.
 ****************************************************************************/
#include <SPI.h>
#include <TCL.h>

#define LEDS  50

// Initial formula from here: http://blog.bodurov.com/Common-Coefficient-Transformations/
// Python code for the table below
/*
import math

curve = 3.9
for i in range(128):
	x = float(i) / 127.0
	sign = 1.0
	if x-0.5 < 0:
		sign = -1.0
	value = sign*math.pow(math.fabs(x-0.5)/curve,1.0/3.0)+0.5
	if value < 0.0:
		value = 0.0
	if value > 1.0:
		value = 1.0
	print "%.8f, " % value,
	if ((i+1) % 8 == 0):
		print ""
		
for i in range(128):
	x = float(127-i) / 127.0
	sign = 1.0
	if x-0.5 < 0:
		sign = -1.0
	value = sign*math.pow(math.fabs(x-0.5)/curve,1.0/3.0)+0.5
	if value < 0.0:
		value = 0.0
	if value > 1.0:
		value = 1.0
	print "%.8f, " % value,
	if ((i+1) % 8 == 0):
		print ""
				
print "};"
*/

PROGMEM float fade_table[256] = {
0.00000000,  0.00000000,  0.00111291,  0.00383170,  0.00658061,  0.00936050,  0.01217225,  0.01501679,  

0.01789509,  0.02080818,  0.02375713,  0.02674306,  0.02976715,  0.03283064,  0.03593485,  0.03908115,  

0.04227100,  0.04550594,  0.04878760,  0.05211771,  0.05549808,  0.05893066,  0.06241752,  0.06596086,  

0.06956301,  0.07322648,  0.07695395,  0.08074829,  0.08461258,  0.08855014,  0.09256454,  0.09665964,  

0.10083964,  0.10510906,  0.10947285,  0.11393641,  0.11850564,  0.12318703,  0.12798771,  0.13291561,  

0.13797949,  0.14318914,  0.14855553,  0.15409101,  0.15980957,  0.16572713,  0.17186196,  0.17823519,  

0.18487142,  0.19179958,  0.19905407,  0.20667625,  0.21471660,  0.22323766,  0.23231846,  0.24206109,  

0.25260117,  0.26412513,  0.27690074,  0.29133574,  0.30810371,  0.32846323,  0.35532029,  0.39968469,  

0.60031531,  0.64467971,  0.67153677,  0.69189629,  0.70866426,  0.72309926,  0.73587487,  0.74739883,  

0.75793891,  0.76768154,  0.77676234,  0.78528340,  0.79332375,  0.80094593,  0.80820042,  0.81512858,  

0.82176481,  0.82813804,  0.83427287,  0.84019043,  0.84590899,  0.85144447,  0.85681086,  0.86202051,  

0.86708439,  0.87201229,  0.87681297,  0.88149436,  0.88606359,  0.89052715,  0.89489094,  0.89916036,  

0.90334036,  0.90743546,  0.91144986,  0.91538742,  0.91925171,  0.92304605,  0.92677352,  0.93043699,  

0.93403914,  0.93758248,  0.94106934,  0.94450192,  0.94788229,  0.95121240,  0.95449406,  0.95772900,  

0.96091885,  0.96406515,  0.96716936,  0.97023285,  0.97325694,  0.97624287,  0.97919182,  0.98210491,  

0.98498321,  0.98782775,  0.99063950,  0.99341939,  0.99616830,  0.99888709,  1.00000000,  1.00000000,  

1.00000000,  1.00000000,  0.99888709,  0.99616830,  0.99341939,  0.99063950,  0.98782775,  0.98498321,  

0.98210491,  0.97919182,  0.97624287,  0.97325694,  0.97023285,  0.96716936,  0.96406515,  0.96091885,  

0.95772900,  0.95449406,  0.95121240,  0.94788229,  0.94450192,  0.94106934,  0.93758248,  0.93403914,  

0.93043699,  0.92677352,  0.92304605,  0.91925171,  0.91538742,  0.91144986,  0.90743546,  0.90334036,  

0.89916036,  0.89489094,  0.89052715,  0.88606359,  0.88149436,  0.87681297,  0.87201229,  0.86708439,  

0.86202051,  0.85681086,  0.85144447,  0.84590899,  0.84019043,  0.83427287,  0.82813804,  0.82176481,  

0.81512858,  0.80820042,  0.80094593,  0.79332375,  0.78528340,  0.77676234,  0.76768154,  0.75793891,  

0.74739883,  0.73587487,  0.72309926,  0.70866426,  0.69189629,  0.67153677,  0.64467971,  0.60031531,  

0.39968469,  0.35532029,  0.32846323,  0.30810371,  0.29133574,  0.27690074,  0.26412513,  0.25260117,  

0.24206109,  0.23231846,  0.22323766,  0.21471660,  0.20667625,  0.19905407,  0.19179958,  0.18487142,  

0.17823519,  0.17186196,  0.16572713,  0.15980957,  0.15409101,  0.14855553,  0.14318914,  0.13797949,  

0.13291561,  0.12798771,  0.12318703,  0.11850564,  0.11393641,  0.10947285,  0.10510906,  0.10083964,  

0.09665964,  0.09256454,  0.08855014,  0.08461258,  0.08074829,  0.07695395,  0.07322648,  0.06956301,  

0.06596086,  0.06241752,  0.05893066,  0.05549808,  0.05211771,  0.04878760,  0.04550594,  0.04227100,  

0.03908115,  0.03593485,  0.03283064,  0.02976715,  0.02674306,  0.02375713,  0.02080818,  0.01789509,  

0.01501679,  0.01217225,  0.00936050,  0.00658061,  0.00383170,  0.00111291,  0.00000000,  0.00000000,  

};

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition g -r -b - back to g
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition g -r -b - back to g
uint32_t Spectrum(byte SpectrumPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("begin");

  TCL.begin();
}

void rainbow(uint8_t wait, uint8_t wait2) {
  int i, j, adjj, pos, pos2;
  uint32_t r, g, b, color;
  float alpha;
  int ratio = wait / wait2;
  
  for (j=0; j < 256 * ratio; j++) {
    TCL.sendEmptyFrame();
    adjj = j / ratio;
    for (i=0; i < LEDS/2; i++) {
      pos = i + adjj;
      pos2 = i + j;
      alpha = pgm_read_float(&fade_table[pos2 & 0xff]);
      color = Wheel(pos & 0xff);
      r = (uint32_t)round(alpha * (float)((color >> 16) & 0xff));
      g = (uint32_t)round(alpha * (float)((color >>  8) & 0xff));
      b = (uint32_t)round(alpha * (float)((color      ) & 0xff));
      TCL.sendColor(r, g, b);
    }
    for (i=LEDS/2-1; i >= 0; i--) {
      pos = i + adjj;
      pos2 = i + j;
      alpha = pgm_read_float(&fade_table[pos2 & 0xff]);
      color = Wheel(pos & 0xff);
      r = (uint32_t)round(alpha * (float)((color >> 16) & 0xff));
      g = (uint32_t)round(alpha * (float)((color >>  8) & 0xff));
      b = (uint32_t)round(alpha * (float)((color      ) & 0xff));
      TCL.sendColor(r, g, b);
    }
    TCL.sendEmptyFrame();
    delay(wait2);
  }
  delay(wait);
}

void loop() {
  rainbow(80, 4);
}
