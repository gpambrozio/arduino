#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <Adafruit_Trellis.h>
#include "VolatileValue.h"

extern TFT_eSPI tft;
extern TFT_eSprite img;

extern float battery;
extern float power;

void addCommand(String command);
void scheduleScreenRefresh();

void setLED(uint8_t n, bool onOff);
bool justPressed(uint8_t n);
bool justReleased(uint8_t n);
void setKeysBrightness(uint8_t brightness);

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define NUM_KEYS  (NUMTRELLIS * 16)
#define MODE_KEYS (NUM_KEYS - 4)

#define DEBUG

#ifdef DEBUG

#define D(d)  Serial.print(d)
#define DL(d) Serial.println(d)
#define DP(args...) Serial.printf(args)
#define MARK  {Serial.print(F("Running line "));Serial.println(__LINE__);}

#else

#define D(d)  {}
#define DL(d) {}
#define DP(...) {}
#define MARK  {}

#endif
