#ifndef ModeDebug_h
#define ModeDebug_h

#include "Mode.h"

class ModeDebug : public Mode
{
  public:
    explicit ModeDebug() {}
    virtual String name() { return "Debug"; }
    virtual void init() {}
    virtual void checkKeys() {}
    virtual void checkCommand(String command) {
      if (command.startsWith("To")) {
        temperatureOutside = command.substring(2).toFloat() / 10.0;
      } else if (command.startsWith("Ti")) {
        temperatureInside = command.substring(2).toFloat() / 10.0;
      } else if (command.startsWith("TO")) {
        thermostatOn = command.substring(2).toInt() != 0;
      } else if (command.startsWith("Tt")) {
        thermostatTarget = command.substring(2).toFloat() / 10.0;
      } else if (command.startsWith("Ws")) {
        wifiSsid = command.substring(2);
      }
    }
    virtual void draw() {
      img.setTextFont(1);
      img.setTextColor(TFT_WHITE);
      img.println("Sketch has been running for");
      img.setTextColor(TFT_MAGENTA, TFT_BLACK);
      img.print(millis() / 1000);
      img.setTextColor(TFT_WHITE, TFT_BLACK);
      img.println(" seconds.");
    
      img.print("Battery: ");
      img.println(battery);
      
      img.print("Power: ");
      img.println(power);
      
      if (wifiSsid != "") {
        img.setTextFont(1);
        img.printf("WiFi: ");
        img.println(wifiSsid);
      }
      if (temperatureInside > 0) {
        img.setTextFont(2);
        img.printf("Inside: %.1f", temperatureInside);
        img.setTextFont(1);
        img.printf("o");
        img.setTextFont(2);
        img.printf("F\n");
      }
      if (temperatureOutside > 0) {
        img.setTextFont(2);
        img.printf("Outside: %.1f", temperatureOutside);
        img.setTextFont(1);
        img.printf("o");
        img.setTextFont(2);
        img.printf("F\n");
      }
    }
  
  private:
    float temperatureOutside = 0;
    float temperatureInside = 0;
    float thermostatTarget = 0;
    bool thermostatOn = false;
};

#endif
