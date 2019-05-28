#ifndef ModeDrive_h
#define ModeDrive_h

#include "Mode.h"
#include <WiFi.h>
#include <vector>

#define FAR   1400.0
#define CLOSE 400.0

class ModeDrive : public Mode
{
  public:
    explicit ModeDrive() {
      // To initialize the DNS as it takes a while the first time.
      http.setConnectTimeout(2000);
      http.begin("agnespanel", 8080, "/ping");
      http.GET();
      http.end();
    }
    virtual String name() { return "Drive"; }
    virtual char identifier() { return 'd'; }
    virtual void setup() {
      addCommand("ParkingSensor:1");
    }
    virtual void tearDown() {
      addCommand("ParkingSensor:0");
    }
    virtual void checkKeys() {
      for (uint8_t i = 0; i < MODE_KEYS; i++) {
        if (justPressed(i)) {
          DP("v%d\n", i);

          http.setConnectTimeout(2000);
          if (i < files.size()) {
            String file = files[i];
            file.replace(" ", "+");
            String command = "/image/" + file + "/1";
            http.begin("agnespanel", 8080, command);
          }

          int httpCode = http.GET();
          if (httpCode > 0) {
            DP("[HTTP] GET... code: %d\n", httpCode);
          } else {
            D(F("[HTTP] GET... failed, error: ")); DL(http.errorToString(httpCode).c_str());
          }

          http.end();
        }
      }
    }
    virtual void setLEDs(uint8_t keys) {
      if (!isActive) return;
      for (uint8_t i = keys; i < NUM_KEYS; i++) {
        setLED(i, i < keys);
      }
    }
    virtual void checkCommand(String command) {
      if (command.startsWith("Ds")) {
        float value = command.substring(2).toFloat();
        distance.setValue(value);
        scheduleScreenRefresh();
        float relative = min(1.0, 1.0 - (value - CLOSE) / (FAR - CLOSE));
        if (relative < 0.0) relative = 0.0;
        setLEDs((uint8_t)(relative * NUM_KEYS));
      } else if (command.startsWith("Pf")) {
        files.clear();
        int position = 2;
        while (true) {
          int comma = command.indexOf(",", position);
          if (comma == -1) {
            if (position < command.length()) {
              files.push_back(command.substring(position));
            }
            break;
          }
          files.push_back(command.substring(position, comma));
          position = comma + 1;
        }
      }
    }
    virtual void draw() {
      img.setTextColor(TFT_WHITE);
      float value = distance.value();
      if (value > 0) {
        img.setTextFont(2);
        img.printf("Distance: %.1f\n", value);
      } else {
        setLEDs(0);
      }
      img.setTextFont(2);
      for (uint8_t i=0; i<files.size(); i++) {
        img.setTextColor(TFT_WHITE, TFT_BLACK);
        if (i > 0) img.print(", ");
        if (i % 2 == 1) {
          img.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        img.printf("%d-", i+1);
        img.print(files[i]);
      }
      img.print("\n");
    }
  private:
    VolatileValue<float> distance = VolatileValue<float>(0, 2);
    std::vector<String> files;
    HTTPClient http;
};

#endif
