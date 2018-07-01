#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 16

#define MAX_BRIGHTNESS 255

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

BLEUart bleuart;

#define MIN_PACKET_SIZE       4 // '!', size, command, ...., checksum
#define READ_BUFSIZE                    (20)
 
/* Buffer to hold incoming characters */
uint8_t packetbuffer[READ_BUFSIZE+1];

#define PACKET_COMMAND   (packetbuffer[2])
#define PACKET_DATA      (packetbuffer+3)
#define PACKET_DATA_SIZE (packetbuffer[1] - MIN_PACKET_SIZE)


void setup() {
  Bluefruit.begin();
  Bluefruit.autoConnLed(false);
  
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("AgnesOutsideLights");

  // Configure and start the BLE Uart service
  bleuart.begin();
 
  // Set up the advertising packet
  setupAdv();
 
  // Start advertising
  Bluefruit.Advertising.start();

  strip.begin();
  strip.setBrightness(MAX_BRIGHTNESS);
  strip.show(); // Initialize all pixels to 'off'
  delay(1000);
  colorWipe(0xFF0000);
  delay(300);
  colorWipe(0x00FF00);
  delay(300);
  colorWipe(0x0000FF);
  delay(300);
  colorWipe(0);
}

uint8_t mode = 0;
uint16_t cyclePosition = 0;
uint16_t cycleDelay = 1;

void loop() {
  // Wait for new data to arrive
  uint8_t len = readPacket(&bleuart);
  if (len > 0) {
    bool commandOK = false;
    if (PACKET_COMMAND == 'C' && PACKET_DATA_SIZE == 4) {
      strip.setBrightness(min(PACKET_DATA[0], MAX_BRIGHTNESS));
      uint32_t color = ((uint32_t)PACKET_DATA[1] << 16) | ((uint32_t)PACKET_DATA[2] <<  8) | PACKET_DATA[3];
      colorWipe(color);
      commandOK = true;
    } else if ((PACKET_COMMAND == 'R' || PACKET_COMMAND == 'T') && PACKET_DATA_SIZE == 2) {
      strip.setBrightness(min(PACKET_DATA[0], MAX_BRIGHTNESS));
      cycleDelay = PACKET_DATA[1];
      commandOK = true;
    }
    if (commandOK) {
      cyclePosition = 0;
      mode = PACKET_COMMAND;
      bleuart.println("OK");
    }
  }
  
  switch (mode) {
    case 'C':
      // No need to do anything
      break;

    case 'R':
      rainbowCycle(cyclePosition++);
      break;

    case 'T':
      theaterChaseRainbow(cyclePosition++);
      break;
  }
  delay(cycleDelay);
}

void setupAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);
  
  // There is no room for 'Name' in the Advertising packet
  // Use the optional secondary Scan Response packet for 'Name' instead
  Bluefruit.ScanResponse.addName();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint16_t j) {
  j &= 0xFF;

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((((i << 8) / strip.numPixels()) + j) & 0xFF));
  }
  strip.show();
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint16_t j) {
  j %= 256 * 3;
  int q = j % 3;
  j /= 3;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
    strip.setPixelColor(i+q, Wheel( (i+j) & 0xFF));    //turn every third pixel on
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint16_t replyidx = 0, replysize = READ_BUFSIZE;
uint8_t readPacket(BLEUart *ble_uart)  {
  while (ble_uart->available() && replyidx < replysize) {
    char c =  ble_uart->read();
    if (replyidx == 0) {
      if (c == '!') {
        replysize = READ_BUFSIZE;
        packetbuffer[replyidx] = c;
        replyidx++;
      }
    } else {
      if (replyidx == 1) {
        replysize = min(READ_BUFSIZE, c);
      }
      packetbuffer[replyidx] = c;
      replyidx++;
    }
  }
 
  if (replyidx < replysize)  // no data (yet...)
    return 0;
  if (packetbuffer[0] != '!')  // doesn't start with '!' packet beginning
    return 0;
  
  // check checksum!
  uint8_t xsum = 0;
  uint8_t checksum = packetbuffer[replyidx-1];
  
  for (uint8_t i=0; i<replyidx-1; i++) {
    xsum += packetbuffer[i];
  }
  xsum = ~xsum;

  uint16_t messagesize = replyidx;
  replyidx = 0, replysize = READ_BUFSIZE;
  
  // Throw an error message if the checksum's don't match
  if (xsum != checksum) {
    bleuart.print("NO");
    bleuart.write((const char*)&xsum, 1);
    return 0;
  }
  
  // checksum passed!
  return messagesize;
}
