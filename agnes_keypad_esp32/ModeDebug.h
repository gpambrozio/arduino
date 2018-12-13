#ifndef ModeDebug_h
#define ModeDebug_h

#include "Mode.h"
#include <WiFi.h>

class ModeDebug : public Mode
{
  public:
    explicit ModeDebug() {}
    virtual String name() { return "Debug"; }
    virtual void init() {}
    virtual void setup() {}
    virtual void tearDown() {}
    virtual void checkKeys() {}
    virtual void checkCommand(String command) {
      if (command.startsWith("Ws")) {
        wifiSsid.setValue(command.substring(2));
        scheduleScreenRefresh();
      }
      else if (command.startsWith("WI")) {
        wifiIP.setValue(command.substring(2));
        scheduleScreenRefresh();
      }
    }
    virtual void draw() {
      long runningSeconds = millis() / 1000;
      img.setTextFont(1);
      img.setTextColor(TFT_WHITE);
      img.println("Sketch running time:");
      img.setTextColor(TFT_MAGENTA, TFT_BLACK);
      long days = runningSeconds / (24 * 60 * 60);
      if (days > 0) {
        img.printf("%dd, ", days);
        runningSeconds %= 24 * 60 * 60;
      }
      img.printf("%02d:", runningSeconds / (60 * 60));
      runningSeconds %= 60 * 60;
      img.printf("%02d.%02d\n", runningSeconds / 60, runningSeconds % 60);
      img.setTextColor(TFT_WHITE, TFT_BLACK);
    
      img.print("Battery: ");
      img.println(battery);
      
      img.print("Power: ");
      img.println(power);

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
};

#endif
