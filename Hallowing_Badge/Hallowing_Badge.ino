// Badge code for Adafruit Hallowing. Uses DMA and related shenanigans for smooth animation.

#include <Adafruit_ZeroDMA.h>
#include <Adafruit_SPIFlash.h>

#include "common.h"
#include "mode.h"
#include "mode_eyes.h"
#include "mode_name.h"
#include "mode_images.h"

#define TFT_CS        39    // Hallowing display control pins: chip select
#define TFT_RST       37    // Display reset
#define TFT_DC        38    // Display data/command select
#define TFT_BACKLIGHT  7    // Display backlight pin

// Declarations for various Hallowing hardware -- display, accelerometer
// and SPI rate & mode.
Adafruit_ST7735 tft    = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_LIS3DH accel  = Adafruit_LIS3DH();
SPISettings settings(12000000, MSBFIRST, SPI_MODE0);

// Declarations related to DMA (direct memory access), which lets us walk
// and chew gum at the same time.  This is VERY specific to SAMD chips and
// means this is not trivially ported to other devices.
Adafruit_ZeroDMA  dma;
DmacDescriptor   *descriptor;
uint16_t          dmaBuf[2][128];
uint8_t           dmaIdx = 0; // Active DMA buffer # (alternate fill/send)

// Configuration of the flash chip pins and flash fatfs object.
// You don't normally need to change these if using a Feather/Metro
// M0 express board.
#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
                                              // If you change this be
                                              // sure to change the fatfs
                                              // object type below to match.

#if defined(__SAMD51__)
  // Alternatively you can define and use non-SPI pins, QSPI isnt on a sercom
  Adafruit_SPIFlash flash(PIN_QSPI_SCK, PIN_QSPI_IO1, PIN_QSPI_IO0, PIN_QSPI_CS);
#else
  #if (SPI_INTERFACES_COUNT == 1)
    #define FLASH_SS       SS                    // Flash chip SS pin.
    #define FLASH_SPI_PORT SPI                   // What SPI port is Flash on?
  #else
    #define FLASH_SS       SS1                    // Flash chip SS pin.
    #define FLASH_SPI_PORT SPI1                   // What SPI port is Flash on?
  #endif

Adafruit_SPIFlash flash(FLASH_SS, &FLASH_SPI_PORT);     // Use hardware SPI
#endif

// Finally create an Adafruit_M0_Express_CircuitPython object which gives
// an SD card-like interface to interacting with files stored in CircuitPython's
// flash filesystem.
Adafruit_M0_Express_CircuitPython fs(flash);

// DMA transfer-in-progress indicator and callback
static volatile bool dma_busy = false;
static void dma_callback(Adafruit_ZeroDMA *dma) {
  dma_busy = false;
}

Mode *modes[] = {
  new ModeName(),
  new ModeImages(),
  new ModeEyes(),
};
#define NUMBER_OF_MODES  (sizeof(modes) / sizeof(Mode *))
byte mode = 0;

// SETUP FUNCTION -- runs once at startup ----------------------------------

void setup(void) {
  Serial.begin(9600);
  //while (!Serial);

  // Initialize flash library and check its chip ID.
  if (!flash.begin(FLASH_TYPE)) {
    Serial.println("Error, failed to initialize flash chip!");
    while(1);
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.GetJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fs.begin()) {
    Serial.println("Failed to mount filesystem!");
    Serial.println("Was CircuitPython loaded on the board first to create the filesystem?");
    while(1);
  }
  Serial.println("Mounted filesystem!");

  // Hardware init
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(2); // Display is rotated 180° on Hallowing
  tft.fillScreen(0);

  pinMode(TFT_BACKLIGHT, OUTPUT);

  if (accel.begin(0x18) || accel.begin(0x19)) {
    accel.setRange(LIS3DH_RANGE_8_G);
    accel.setClick(1, 60);
  }

  // Set up SPI DMA.
  int                dmac_id;
  volatile uint32_t *data_reg;
  dma.allocate();
  dma.setTrigger(SERCOM5_DMAC_ID_TX);
  data_reg = &SERCOM5->SPI.DATA.reg;
  dma.setAction(DMA_TRIGGER_ACTON_BEAT);
  descriptor = dma.addDescriptor(
    NULL,               // move data
    (void *)data_reg,   // to here
    sizeof dmaBuf[0],   // this many...
    DMA_BEAT_SIZE_BYTE, // bytes/hword/words
    true,               // increment source addr?
    false);             // increment dest addr?
  dma.setCallback(dma_callback);

  analogWrite(TFT_BACKLIGHT, 40);
  for (uint8_t i = 0; i < NUMBER_OF_MODES; i++) {
    modes[i]->init();
  }
  modes[mode]->setup();
}

// LOOP FUNCTION -- repeats indefinitely -----------------------------------

long nextClick = 0;

void loop(void) {
  bool hasClick = false;
  uint8_t click = accel.getClick();
  if (click & 0x30 && millis() > nextClick) {
    nextClick = millis() + 500;
    hasClick = true;
    Serial.print("Click detected (0x"); Serial.print(click, HEX); Serial.print("): ");
    if (click & 0x10) Serial.println("single click");
    if (click & 0x20) Serial.println("double click");
    if (!modes[mode]->manualAdvance()) {
      nextMode();
      hasClick = false;
    }
  }
  accel.read();
  modes[mode]->draw(hasClick);
}

void nextMode() {
  modes[mode]->tearDown();
  if (++mode >= NUMBER_OF_MODES) {
    mode = 0;
  }
  modes[mode]->setup();
}

void setAddrWindow(int x, int y, int w, int h) {
  SPI.beginTransaction(settings);    // SPI init
  digitalWrite(TFT_CS, LOW);         // Chip select
  tft.setAddrWindow(x, y, w, h);
  digitalWrite(TFT_CS, LOW);         // Re-select after addr function
  digitalWrite(TFT_DC, HIGH);        // Data mode...
}

void endDrawing() {
  while(dma_busy);            // Wait for last DMA transfer to complete
  digitalWrite(TFT_CS, HIGH); // Deselect
  SPI.endTransaction();       // SPI done
}

void dmaXfer(uint16_t n) { // n = Transfer size in bytes
  while(dma_busy);         // Wait for prior DMA transfer to finish
  // Set up DMA transfer from newly-filled buffer
  descriptor->SRCADDR.reg = (uint32_t)&dmaBuf[dmaIdx] + n;
  dma_busy = true;         // Flag as busy
  dma.startJob();          // Start new DMA transfer
  dmaIdx = 1 - dmaIdx;     // And swap DMA buffer indices
}
