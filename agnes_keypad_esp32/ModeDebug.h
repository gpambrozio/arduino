#ifndef ModeDebug_h
#define ModeDebug_h

#include "Mode.h"

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
    }
    virtual void draw() {
      img.setTextFont(1);
      img.setTextColor(TFT_WHITE);
      img.print("Sketch has been\nrunning for ");
      img.setTextColor(TFT_MAGENTA, TFT_BLACK);
      img.print(millis() / 1000);
      img.setTextColor(TFT_WHITE, TFT_BLACK);
      img.println(" secs");
    
      img.print("Battery: ");
      img.println(battery);
      
      img.print("Power: ");
      img.println(power);

      String ssid = wifiSsid.value();
      if (ssid != "") {
        img.setTextFont(1);
        img.printf("WiFi: ");
        img.println(ssid);
      }
    }
  
  private:
    VolatileValue<String> wifiSsid = VolatileValue<String>("");
};

#endif
