#include <Adafruit_ST7735.h>
#include <Adafruit_LIS3DH.h>

extern Adafruit_ST7735 tft;
extern Adafruit_LIS3DH accel;

extern Adafruit_ZeroDMA  dma;
extern DmacDescriptor   *descriptor;
extern uint16_t          dmaBuf[2][128];
extern uint8_t           dmaIdx; // Active DMA buffer # (alternate fill/send)

void dmaXfer(uint16_t n);
void setAddrWindow(int x, int y, int w, int h);
void nextMode();
