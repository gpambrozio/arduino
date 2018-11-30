#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <Adafruit_Trellis.h>
#include "VolatileValue.h"

extern Adafruit_TrellisSet trellis;

extern TFT_eSPI tft;
extern TFT_eSprite img;

extern HTTPClient http;
extern WiFiClient client;

extern float battery;
extern float power;

void addCommand(String command);

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define NUM_KEYS  (NUMTRELLIS * 16)
#define MODE_KEYS (NUM_KEYS - 4)
