#ifndef ModeImages_h
#define ModeImages_h

#include "mode.h"

class ModeImages : public Mode
{
  public:
    explicit ModeImages() {}
    virtual void init() {
      totalFiles = 0;
      while (true) {
        String fileName = "image" + String(totalFiles) + ".raw";
        if (!fs.exists(fileName)) {
          break;
        }
        totalFiles++;
      }
    }
    virtual void setup() {
      currentFile = 0;
      isFirstFile = true;
    }
    virtual void tearDown() {}
    virtual bool manualAdvance() { return true; }
    virtual void draw(bool hasClick) {
      if (!isFirstFile && !hasClick) return;
      if (!isFirstFile && ++currentFile >= totalFiles) {
        nextMode();
        return;
      }
      isFirstFile = false;
      
      String fileName = "image" + String(currentFile) + ".raw";
      File imageFile = fs.open(fileName, FILE_READ);
      
      uint16_t *dmaPtr;   // Pointer into DMA output buffer (16 bits/pixel)
      uint8_t   col, row; // X,Y pixel counters
      uint16_t  nBytes;   // Size of DMA transfer

      descriptor->BTCNT.reg = nBytes = 128 * 2;

      uint16_t *srcPtr;

      setAddrWindow(0, 0, 128, 128);

      // Process rows
      for (row=0; row<128; row++) {
        dmaPtr  = &dmaBuf[dmaIdx][0];

        for (col=0; col<128; col++) {
          if (imageFile.read(dmaPtr, 2)) {
            dmaPtr++;
          } else {
            *dmaPtr++ = 0x00ff;
          }
        }
        dmaXfer(nBytes);
      }
      imageFile.close();
      endDrawing();
    }
  
  private:
    bool isFirstFile;
    int currentFile;
    int totalFiles;
};

#endif
