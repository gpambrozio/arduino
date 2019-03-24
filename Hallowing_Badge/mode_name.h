#ifndef ModeName_h
#define ModeName_h

#include "mode.h"
#include "name.h"

class ModeName : public Mode
{
  public:
    explicit ModeName() {}
    virtual void init() {}
    virtual void setup() {
      column = -TFT_W;
      tft.fillScreen(0xffff);
    }
    virtual void tearDown() {}
    virtual void draw(bool hasClick) {
      setAddrWindow(0, (TFT_H - NAME_ROWS) / 2, TFT_W, NAME_ROWS);

      uint16_t *dmaPtr;   // Pointer into DMA output buffer (16 bits/pixel)
      uint8_t   row;
      uint16_t  nBytes;   // Size of DMA transfer

      descriptor->BTCNT.reg = nBytes = TFT_W * 2;

      uint16_t *srcPtr;

      // Process rows
      for (row=0; row<NAME_ROWS; row++) {
        dmaPtr  = &dmaBuf[dmaIdx][0];

        // max below is for when column < 0.
        uint16_t firstColumn = max(0, column);
        uint16_t lastColumn = min(NAME_COLS, column + TFT_W);
        srcPtr = (uint16_t *)&nameData[row][firstColumn];
        dmaPtr += pad(dmaPtr, -column);
        uint16_t bytes = lastColumn - firstColumn + 1;
        memcpy(dmaPtr, srcPtr, bytes * 2);
        dmaPtr += bytes;
        pad(dmaPtr, column + TFT_W - NAME_COLS);
        dmaXfer(nBytes);
      }
      if (++column >= NAME_COLS) {
        column = -TFT_W;
      }
      endDrawing();
    }
  
  private:
    int column;
    uint16_t pad(uint16_t *dmaPtr, int16_t columns) {
      if (columns <= 0) return 0;
      memset(dmaPtr, 0xff, columns * 2);
      return columns;
    }
};

#endif
