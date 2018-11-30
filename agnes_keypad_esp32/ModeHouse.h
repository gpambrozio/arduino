#ifndef ModeHouse_h
#define ModeHouse_h

#include "Mode.h"

class ModeHouse : public Mode
{
  public:
    explicit ModeHouse() {}
    virtual String name() { return "House"; }
    virtual void init() {}
    virtual void setup() {}
    virtual void tearDown() {}
    virtual void checkKeys() {}
    virtual void checkCommand(String command) {
      if (command.startsWith("To")) {
        temperatureOutside.setValue(command.substring(2).toFloat() / 10.0);
      } else if (command.startsWith("Ti")) {
        temperatureInside.setValue(command.substring(2).toFloat() / 10.0);
      } else if (command.startsWith("TO")) {
        thermostatOn.setValue(command.substring(2).toInt() != 0);
      } else if (command.startsWith("Tt")) {
        thermostatTarget.setValue(command.substring(2).toFloat() / 10.0);
      }
    }
    virtual void draw() {
      img.setTextColor(TFT_WHITE);
      float value;
      value = temperatureInside.value();
      if (value > 0) {
        img.setTextFont(2);
        img.printf("Inside: %.1f", value);
        img.setTextFont(1);
        img.printf("o");
        img.setTextFont(2);
        img.printf("F\n");
      }
      value = temperatureOutside.value();
      if (value > 0) {
        img.setTextFont(2);
        img.printf("Outside: %.1f", value);
        img.setTextFont(1);
        img.printf("o");
        img.setTextFont(2);
        img.printf("F\n");
      }
      value = thermostatTarget.value();
      if (value > 0) {
        img.setTextFont(2);
        img.printf("Thermostat: %.1f", value);
        img.setTextFont(1);
        img.printf("o");
        img.setTextFont(2);
        img.printf("F\n");
      }
    }
  
  private:
    VolatileValue<float> temperatureOutside = VolatileValue<float>(0);
    VolatileValue<float> temperatureInside = VolatileValue<float>(0);
    VolatileValue<float> thermostatTarget = VolatileValue<float>(0);
    VolatileValue<bool>  thermostatOn = VolatileValue<bool>(false);
};

#endif
