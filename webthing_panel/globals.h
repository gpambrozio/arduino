#include <TFT_eSPI.h>
#include <HTTPClient.h>
extern TFT_eSPI tft;
extern TFT_eSprite img;

extern float battery;
extern float power;

// set to however many you're working with here, up to 8
#define NUMTRELLIS 1

#define NUM_KEYS  (NUMTRELLIS * 16)
#define MODE_KEYS (NUM_KEYS - 4)
