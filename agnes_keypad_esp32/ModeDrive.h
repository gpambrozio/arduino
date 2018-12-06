#ifndef ModeDrive_h
#define ModeDrive_h

#include "Mode.h"

#define FAR   1400.0
#define CLOSE 400.0

class ModeDrive : public Mode
{
  public:
    explicit ModeDrive() {}
    virtual String name() { return "Drive"; }
    virtual void init() {
      // To initialize the DNS as it takes a while the first time.
      http.begin("agnespanel", 8080, "/text//0");
      http.GET();
      http.end();
    }
    virtual void setup() {
      addCommand("ParkingSensor:1");
    }
    virtual void tearDown() {
      addCommand("ParkingSensor:0");
    }
    virtual void checkKeys() {
      for (uint8_t i = 0; i < MODE_KEYS; i++) {
        if (trellis.justPressed(i)) {
          Serial.printf("v%d\n", i);

          if (i % 2 == 1) {
            http.begin("agnespanel", 8080, "/text//0");
          } else {
            http.begin("agnespanel", 8080, "/image/thanks/1");
          }

          int httpCode = http.GET();
          if (httpCode > 0) {
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
          } else {
            Serial.print("[HTTP] GET... failed, error: "); Serial.println(http.errorToString(httpCode).c_str());
          }

          http.end();
        }
      }
    }
    virtual void checkCommand(String command) {
      if (command.startsWith("Ds")) {
        float value = command.substring(2).toFloat();
        distance.setValue(value);
        scheduleScreenRefresh();
        if (isActive) {
          float relative = min(1.0, 1.0 - (value - CLOSE) / (FAR - CLOSE));
          if (relative < 0.0) relative = 0.0;
          uint8_t keys = (uint8_t)(relative * NUM_KEYS);
          for (uint8_t i = 0; i < keys; i++) {
            trellis.setLED(i);
          }
          for (uint8_t i = keys; i < NUM_KEYS; i++) {
            trellis.clrLED(i);
          }
          trellis.writeDisplay();
        }
      }
    }
    virtual void draw() {
      img.setTextColor(TFT_WHITE);
      float value = distance.value();
      if (value > 0) {
        img.setTextFont(2);
        img.printf("Distance: %.1f\n", value);
      }
    }
  private:
    VolatileValue<float> distance = VolatileValue<float>(0, 5);
};

#endif
