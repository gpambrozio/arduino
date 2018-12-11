#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>

#define DEBUG

#ifdef DEBUG

#define D(d)  Serial.print(d)
#define DL(d) Serial.println(d)
#define MARK  {Serial.print(F("Running line "));Serial.println(__LINE__);}

#else

#define D(d)  {}
#define DL(d) {}
#define MARK  {}

#endif

#define PIN_OUTSIDE 15
#define PIN_INSIDE 16

#define MAX_BRIGHTNESS_OUTSIDE 200
#define MAX_BRIGHTNESS_INSIDE 40

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip_inside = Adafruit_NeoPixel(259, PIN_INSIDE, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip_outside = Adafruit_NeoPixel(150, PIN_OUTSIDE, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

BLEUart bleuart;

#define MIN_PACKET_SIZE       5 // '!', size, command, segment, ...., checksum
#define READ_BUFSIZE                    (20)
 
/* Buffer to hold incoming characters */
uint8_t packetbuffer[READ_BUFSIZE+1];

#define PACKET_COMMAND   (packetbuffer[2])
#define PACKET_SEGMENT   (packetbuffer[3])
#define PACKET_DATA      (packetbuffer+4)
#define PACKET_DATA_SIZE (packetbuffer[1] - MIN_PACKET_SIZE)

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DL("Start");
  pinMode(LED_RED, OUTPUT);

  delay(1000);

  DL("Starting strips");
  strip_inside.begin();
  strip_outside.begin();
  
  DL("Resetting strips");
  strip_inside.setBrightness(MAX_BRIGHTNESS_INSIDE);
  strip_inside.show(); // Initialize all pixels to 'off'
  strip_outside.setBrightness(MAX_BRIGHTNESS_OUTSIDE);
  strip_outside.show(); // Initialize all pixels to 'off'

  delay(1000);
  colorWipe(&strip_inside, 0xFF0000);
  colorWipe(&strip_outside, 0xFF0000);
  delay(300);
  colorWipe(&strip_inside, 0x00FF00);
  colorWipe(&strip_outside, 0x00FF00);
  delay(300);
  colorWipe(&strip_inside, 0x0000FF);
  colorWipe(&strip_outside, 0x0000FF);
  delay(300);
  colorWipe(&strip_inside, 0);
  colorWipe(&strip_outside, 0);

  DL("Starting BT");
  Bluefruit.begin(2);
  Bluefruit.autoConnLed(false);

  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("AgnesLights");

  // Set the connect/disconnect callback handlers
  Bluefruit.setConnectCallback(connectCallback);
  Bluefruit.setDisconnectCallback(disconnectCallback);

  // Configure and start the BLE Uart service
  bleuart.begin();
 
  // Set up the advertising packet and start it
  startAdv();
 
  DL("Ending setup");
}

int target_brightness_outside = 0;
int target_brightness_inside = 0;
int brightness_outside = 0;
int brightness_inside = 0;
uint32_t color_outside = 0;
uint32_t color_inside = 0;
uint8_t mode_outside = 'C';
uint8_t mode_inside = 'C';
uint16_t cyclePosition = 0;
uint16_t cycleDelay = 1;
unsigned long next_cycle_change = 0;
unsigned long next_blink = 0;

void loop() {
  if (millis() >= next_blink) {
    if (millis() >= next_blink + 10) {
      digitalWrite(LED_RED, LOW);
      next_blink = millis() + 15000;
      DL("Loop");
    } else {
      digitalWrite(LED_RED, HIGH);
    }
  }

  // Wait for new data to arrive
  uint8_t len = readPacket(&bleuart);
  if (len > 0) {
    bool commandOK = false;
    char segment = PACKET_SEGMENT;
    Adafruit_NeoPixel* strip;
    int max_brightness;
    int *target_brightness;
    uint32_t *color;
    
    if (segment == 'O') {
      strip = &strip_outside;
      max_brightness = MAX_BRIGHTNESS_OUTSIDE;
      target_brightness = &target_brightness_outside;
      color = &color_outside;
    } else {
      strip = &strip_inside;
      max_brightness = MAX_BRIGHTNESS_INSIDE;
      target_brightness = &target_brightness_inside;
      color = &color_inside;
    }
    
    if (PACKET_COMMAND == 'C' && PACKET_DATA_SIZE == 4) {
      *target_brightness = min(PACKET_DATA[0], max_brightness);
      *color = ((uint32_t)PACKET_DATA[1] << 16) | ((uint32_t)PACKET_DATA[2] <<  8) | PACKET_DATA[3];
      colorWipe(strip, *color);
      commandOK = true;
    } else if ((PACKET_COMMAND == 'R' || PACKET_COMMAND == 'T') && PACKET_DATA_SIZE == 2) {
      *target_brightness = min(PACKET_DATA[0], max_brightness);
      cycleDelay = PACKET_DATA[1];
      commandOK = true;
    }
    if (commandOK) {
      if (segment == 'O') {
        mode_outside = PACKET_COMMAND;
      } else {
        mode_inside = PACKET_COMMAND;
      }
      bleuart.print("OK");
    }
  }

  if (target_brightness_outside != brightness_outside) {
    brightness_outside += (target_brightness_outside > brightness_outside) ? 1 : -1;
    strip_outside.setBrightness(brightness_outside);
    if (mode_outside == 'C') {
      colorWipe(&strip_outside, color_outside);
    }
  }
  if (target_brightness_inside != brightness_inside) {
    brightness_inside += (target_brightness_inside > brightness_inside) ? 1 : -1;
    strip_inside.setBrightness(brightness_inside);
    if (mode_inside == 'C') {
      colorWipe(&strip_inside, color_inside);
    }
  }
  
  switch (mode_outside) {
    case 'C':
      // No need to do anything
      break;

    case 'R':
      rainbowCycle(&strip_outside, cyclePosition);
      break;

    case 'T':
      theaterChaseRainbow(&strip_outside, cyclePosition);
      break;
  }
  switch (mode_inside) {
    case 'C':
      // No need to do anything
      break;

    case 'R':
      rainbowCycle(&strip_inside, cyclePosition);
      break;

    case 'T':
      theaterChaseRainbow(&strip_inside, cyclePosition);
      break;
  }

  if (millis() > next_cycle_change) {
    next_cycle_change += cycleDelay;
    cyclePosition++;
  }
}

void startAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);
  
  // There is no room for 'Name' in the Advertising packet
  // Use the optional secondary Scan Response packet for 'Name' instead
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void connectCallback(uint16_t conn_handle) {
  D("Connected to ");
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));
  DL(central_name);
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason) {
  DL("Disconnected");
  DL("Advertising!");
}

// Fill the dots one after the other with a color
void colorWipe(Adafruit_NeoPixel *strip, uint32_t c) {
  for(uint16_t i=0; i<strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
  }
  strip->show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(Adafruit_NeoPixel *strip, uint16_t j) {
  j &= 0xFF;

  for (uint16_t i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, Wheel((((i << 8) / strip->numPixels()) + j) & 0xFF));
  }
  strip->show();
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(Adafruit_NeoPixel *strip, uint16_t j) {
  j %= 256 * 3;
  int q = j % 3;
  j /= 3;
  for (uint16_t i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, 0);
  }
  for (uint16_t i = 0; i < strip->numPixels(); i = i + 3) {
    strip->setPixelColor(i+q, Wheel( (i+j) & 0xFF));    //turn every third pixel on
  }
  strip->show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip_inside.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip_inside.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip_inside.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

unsigned long last_received_byte = 0;
uint16_t replyidx = 0, replysize = READ_BUFSIZE;

uint8_t readPacket(BLEUart *ble_uart)  {
  if (replyidx > 0 && millis() - last_received_byte > 2000) {
     replyidx = 0;
     replysize = READ_BUFSIZE;
     bleuart.print("TO");
  }
  
  while (ble_uart->available() && replyidx < replysize) {
    char c =  ble_uart->read();
    D("Received ");DL(c);
    last_received_byte = millis();
    if (replyidx == 0) {
      if (c == '!') {
        replysize = READ_BUFSIZE;
        packetbuffer[replyidx++] = c;
      }
    } else {
      if (replyidx == 1) {
        replysize = max(MIN_PACKET_SIZE, min(READ_BUFSIZE, c));
      }
      packetbuffer[replyidx] = c;
      replyidx++;
    }
  }
 
  if (replyidx < replysize) {  // no data (yet...)
    return 0;
  }
  if (packetbuffer[0] != '!') { // doesn't start with '!' packet beginning
    MARK;
    return 0;
  }
  
  // check checksum!
  uint8_t xsum = 0;
  uint8_t checksum = packetbuffer[replyidx-1];
  
  for (uint8_t i=0; i<replyidx-1; i++) {
    xsum += packetbuffer[i];
  }
  xsum = ~xsum;

  uint16_t messagesize = replyidx;
  replyidx = 0;
  replysize = READ_BUFSIZE;
  
  // Throw an error message if the checksum's don't match
  if (xsum != checksum) {
    bleuart.print("NO");
    MARK;
    return 0;
  }
  
  // checksum passed!
  MARK;
  return messagesize;
}
