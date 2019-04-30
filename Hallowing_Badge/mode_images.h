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
      resetModeCounter();

      isFirstFile = false;
      
      String fileName = "image" + String(currentFile) + ".raw";
      File imageFile = fs.open(fileName, FILE_READ);
      
      uint16_t *dmaPtr;   // Pointer into DMA output buffer (16 bits/pixel)
      uint8_t   row;
      uint16_t  nBytes;   // Size of DMA transfer

      descriptor->BTCNT.reg = nBytes = TFT_W * 2;

      uint16_t *srcPtr;

      setAddrWindow(0, 0, TFT_W, TFT_H);

      // Process rows
      for (row=0; row<TFT_H; row++) {
        dmaPtr = &dmaBuf[dmaIdx][0];
        if (!imageFile.read(dmaPtr, nBytes)) {
          memset(dmaPtr, 0xff, nBytes);
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
