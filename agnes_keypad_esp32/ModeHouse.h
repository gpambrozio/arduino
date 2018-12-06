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
    virtual void checkKeys() {
      if (trellis.justPressed(0)) {
        thermostatTarget += 1.0;
        addCommand("ThermostatTarget:" + String(thermostatTarget, 0));
        scheduleScreenRefresh();
      }
      if (trellis.justPressed(4)) {
        thermostatTarget -= 1.0;
        addCommand("ThermostatTarget:" + String(thermostatTarget, 0));
        scheduleScreenRefresh();
      }
      if (trellis.justPressed(1)) {
        thermostatOn = !thermostatOn;
        addCommand("ThermostatOnOff:" + String(thermostatOn ? "1" : "0"));
        scheduleScreenRefresh();
      }
    }
    virtual void checkCommand(String command) {
      if (command.startsWith("To")) {
        temperatureOutside.setValue(command.substring(2).toFloat() / 10.0);
        scheduleScreenRefresh();
      } else if (command.startsWith("Ti")) {
        temperatureInside.setValue(command.substring(2).toFloat() / 10.0);
        scheduleScreenRefresh();
      } else if (command.startsWith("TO")) {
        thermostatOn = command.substring(2).toInt() != 0;
        scheduleScreenRefresh();
      } else if (command.startsWith("Tt")) {
        thermostatTarget = command.substring(2).toFloat();
        scheduleScreenRefresh();
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
      if (thermostatTarget > 0) {
        img.setTextFont(2);
        img.printf("Thermostat: %.0f", thermostatTarget);
        img.setTextFont(1);
        img.printf("o");
        img.setTextFont(2);
        img.printf("F\n(");
        img.printf(thermostatOn ? "On)\n" : "Off)\n");
      }
    }
  
  private:
    VolatileValue<float> temperatureOutside = VolatileValue<float>(0);
    VolatileValue<float> temperatureInside = VolatileValue<float>(0);
    float thermostatTarget = 71.0;
    bool  thermostatOn = false;
};

#endif
