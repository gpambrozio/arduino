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
      column = -128;
    }
    virtual void tearDown() {}
    virtual void draw(bool hasClick) {
      setAddrWindow(0, 0, 128, 128);

      uint16_t *dmaPtr;   // Pointer into DMA output buffer (16 bits/pixel)
      uint8_t   col, row; // X,Y pixel counters
      uint16_t  nBytes;   // Size of DMA transfer

      descriptor->BTCNT.reg = nBytes = 128 * 2;

      uint16_t *srcPtr;

      // Process rows
      uint16_t padding = (128 - NAME_ROWS) / 2;
      pad(padding, nBytes);
      for (row=0; row<NAME_ROWS; row++) {
        dmaPtr  = &dmaBuf[dmaIdx][0];

        // max below is for when column < 0.
        // In that case the if inside the for only starts
        // actually reading the line when column + col >= 0
        srcPtr = (uint16_t *)&nameData[row][max(0, column)];
        for (col=0; col<128; col++) {
          // Allows to wrap around nicely.
          if (col >= -column && column + col < NAME_COLS) {
            *dmaPtr++ = __builtin_bswap16(*srcPtr++);
          } else {
            *dmaPtr++ = 0xffff;
          }
        }
        dmaXfer(nBytes);
      }
      pad(padding, nBytes);
      if (++column >= NAME_COLS) {
        column = -128;
      }
      endDrawing();
    }
  
  private:
    int column;
    void pad(uint16_t padding, uint16_t  nBytes) {
      uint16_t *dmaPtr;
      for (int row=0; row<padding; row++) {
        dmaPtr = &dmaBuf[dmaIdx][0];
        for (int col=0; col<128; col++) {
          *dmaPtr++ = 0xffff;
        }
        dmaXfer(nBytes);
      }
    }
};

#endif
