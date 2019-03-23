/**** 
 * Basic demo for Hallowing 
 * Displays SPI flash details, accelerometer readings, light reading,
 *  external sensor port reading, and capacitive touch sensing on the 
 *  four 'teeth'
 * At the same time, if a tap is detected, audio will play from the speaker
 * And, any NeoPixels connected on the NeoPixel port will rainbow swirl
 */


#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_FreeTouch.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

#define TFT_CS     39
#define TFT_RST    37
#define TFT_DC     38
#define TFT_BACKLIGHT 7

#define FLASHCS   SS1
Adafruit_SPIFlash flash(FLASHCS, &SPI1);

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

Adafruit_FreeTouch qt_1 = Adafruit_FreeTouch(A2, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_2 = Adafruit_FreeTouch(A3, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_3 = Adafruit_FreeTouch(A4, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_4 = Adafruit_FreeTouch(A5, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

int lastRotation = 2;

void setup(void) {
  Serial.begin(9600);
  //while (!Serial);

  Serial.println("Hallowing test!");

  // Start TFT and fill black
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(lastRotation);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);

  // Turn on backlight
  pinMode(TFT_BACKLIGHT, OUTPUT);
  analogWrite(TFT_BACKLIGHT, 40);

  flash.begin(SPIFLASHTYPE_W25Q16BV);

  uint8_t manid, devid;
  Serial.println("Reading Manuf ID");
  flash.GetManufacturerInfo(&manid, &devid);
  Serial.print("JEDEC ID: 0x"); Serial.println(flash.GetJEDECID(), HEX);
  Serial.print("Manuf: 0x"); Serial.print(manid, HEX);
  Serial.print(" Device: 0x"); Serial.println(devid, HEX);

  if (!lis.begin(0x18) && !lis.begin(0x19)) {
    Serial.println("Couldnt start lis3dh");
    while (1);
  }
  Serial.println("LIS3DH found!");
  
  lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
  lis.setClick(1, 80);
  
  if (! qt_1.begin())  
    Serial.println("Failed to begin qt on pin A2");
  if (! qt_2.begin())  
    Serial.println("Failed to begin qt on pin A3");
  if (! qt_3.begin())  
    Serial.println("Failed to begin qt on pin A4");
  if (! qt_4.begin())  
    Serial.println("Failed to begin qt on pin A5");

  analogWriteResolution(10);
  analogWrite(A0, 128);
}

uint8_t j = 0;

void loop() {
  uint8_t click = lis.getClick();
  if (click & 0x30) {
    Serial.print("Click detected (0x"); Serial.print(click, HEX); Serial.print("): ");
    if (click & 0x10) Serial.println("single click");
    if (click & 0x20) Serial.println("double click");
  }

  sensors_event_t event; 
  lis.getEvent(&event);
  
  bool isX = abs(event.acceleration.x) > abs(event.acceleration.y);
  int rotation;
  if (isX) {
    rotation = event.acceleration.x > 0 ? 0 : 2;
  } else {
    rotation = event.acceleration.y > 0 ? 1 : 3;
  }
  if (max(abs(event.acceleration.x), abs(event.acceleration.y)) >= 2.0) {
    if (lastRotation != rotation) {
      lastRotation = rotation;
      tft.setRotation(lastRotation);
      tft.fillScreen(ST77XX_BLACK);
    }
  }

  /* Display the results (acceleration is measured in m/s^2) */
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print("X:"); tft.print(event.acceleration.x, 1);
  tft.print(" Y:"); tft.print(event.acceleration.y, 1);
  tft.print(" Z:"); tft.print(event.acceleration.z, 1); tft.println("    ");

  // Read light sensor
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.print("Light: "); tft.print(analogRead(A1)); tft.println("    ");

  tft.setCursor(0, 20);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  float vbat = analogRead(A6)*2*3.3/1024;
  tft.print("Battery: "); tft.print(vbat); tft.print("V"); tft.println("    ");

  tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_BLUE, ST77XX_BLACK);
  float vsense = analogRead(A11)*3.3/1024;
  tft.print("Sensor: "); tft.print(vsense); tft.print("V"); tft.println("    ");

  if (qt_4.measure() > 700) {
     tft.fillCircle(16, 110, 10, ST77XX_BLUE);
  } else {
     tft.fillCircle(16, 110, 10, ST77XX_BLACK);
  }
  if (qt_3.measure() > 700) {
     tft.fillCircle(48, 110, 10, ST77XX_BLUE);
  } else {
     tft.fillCircle(48, 110, 10, ST77XX_BLACK);
  }
  if (qt_2.measure() > 700) {
     tft.fillCircle(80, 110, 10, ST77XX_BLUE);
  } else {
     tft.fillCircle(80, 110, 10, ST77XX_BLACK);
  }
  if (qt_1.measure() > 700) {
     tft.fillCircle(112, 110, 10, ST77XX_BLUE);
  } else {
     tft.fillCircle(112, 110, 10, ST77XX_BLACK);
  }

}
