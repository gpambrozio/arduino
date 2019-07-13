#ifndef ModeDebug_h
#define ModeDebug_h

#include "Mode.h"
#include <WiFi.h>

class ModeDebug : public Mode
{
  public:
    explicit ModeDebug() {}
    virtual String name() { return "Debug"; }
    virtual char identifier() { return 'D'; }
    virtual void init() {}
    virtual void setup() { touches = 0; }
    virtual void tearDown() {}
    
    virtual void checkKeys() {
      for (uint8_t i = 0; i < MODE_KEYS; i++) {
        if (justPressed(i)) {
          touches++;
          setLED(i, true);
        } else if (justReleased(i)) {
          setLED(i, false);
        }
      }
    }
    
    virtual void checkCommand(String command) {
      if (command.startsWith("Ws")) {
        wifiSsid.setValue(command.substring(2));
        scheduleScreenRefresh();
      } else if (command.startsWith("Bo")) {
        batteryOutside.setValue(command.substring(2).toInt());
        scheduleScreenRefresh();
      } else if (command.startsWith("WI")) {
        wifiIP.setValue(command.substring(2));
        scheduleScreenRefresh();
      }
    }
    
    virtual void draw() {
      img.println("");
      long runningSeconds = millis() / 1000;
      img.setTextFont(1);
      img.setTextColor(TFT_WHITE);
      img.println("Sketch running time:");
      img.setTextColor(TFT_MAGENTA, TFT_BLACK);
      long days = runningSeconds / (24 * 60 * 60);
      if (days > 0) {
        img.printf("%ldd, ", days);
        runningSeconds %= 24 * 60 * 60;
      }
      img.printf("%02ld:", runningSeconds / (60 * 60));
      runningSeconds %= 60 * 60;
      img.printf("%02ld.%02ld\n", runningSeconds / 60, runningSeconds % 60);
      img.setTextColor(TFT_WHITE, TFT_BLACK);
    
      img.printf("Battery: %.2fV\n", battery);
      img.printf("Power: %.2fV\n", power);
      img.printf("Touches: %d\n", touches);

      int outsideBattery = batteryOutside.value();
      if (outsideBattery > 0) {
        img.printf("Outside batt: %d%%\n", outsideBattery);
      }
      
      if (WiFi.isConnected()) {
        img.print("IP: ");
        img.println(WiFi.localIP());
      } else {
        img.println("Disconnected!!");
      }

      String value = wifiSsid.value();
      if (value != "") {
        img.print("WiFi: ");
        img.print(value);
        value = wifiIP.value();
        if (value != "") {
          img.print(" (");
          img.print(value);
          img.println(")");
        }
      }
    }
  
  private:
    VolatileValue<String> wifiSsid = VolatileValue<String>("");
    VolatileValue<String> wifiIP = VolatileValue<String>("");
    VolatileValue<int> batteryOutside = VolatileValue<int>(0);
    int touches = 0;
};

#endif
