#ifndef ModeHouse_h
#define ModeHouse_h

#include "Mode.h"

class ModeHouse : public Mode
{
  public:
    explicit ModeHouse() {}
    virtual String name() { return "House"; }
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
      }
    }
    virtual void draw() {
      img.setTextColor(TFT_WHITE);
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
