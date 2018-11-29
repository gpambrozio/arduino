#ifndef ModeDrive_h
#define ModeDrive_h

#include "Mode.h"

class ModeDrive : public Mode
{
  public:
    explicit ModeDrive() {}
    virtual String name() { return "Drive"; }
    virtual void init() { }
    virtual void setup() {
      addCommand("ParkingSensor:1");
    }
    virtual void tearDown() {
      addCommand("ParkingSensor:0");
    }
    virtual void checkKeys() {
      for (uint8_t i=0; i<12; i++) {
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
        distance = command.substring(2).toFloat();
      }
    }
    virtual void draw() {
      img.setTextColor(TFT_WHITE);
      if (distance > 0) {
        img.setTextFont(2);
        img.printf("Distance: %.1f\n", distance);
      }
    }
  private:
    float distance = 0;
};

#endif
