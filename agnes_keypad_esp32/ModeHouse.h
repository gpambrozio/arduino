#ifndef ModeHouse_h
#define ModeHouse_h

#include "Mode.h"

class ModeHouse : public Mode
{
  public:
    explicit ModeHouse() {}
    virtual String name() { return "House"; }
    virtual void init() {}
    virtual void setup() {
      trellis.setBrightness(1);
      refreshLeds();
    }
    virtual void tearDown() {
      trellis.setBrightness(15);
    }
    virtual void checkKeys() {
      // Thermostat
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
        refreshLeds();
      }

      // Outside lights
      if (trellis.justPressed(2)) {
        int brightness = lightOutside.value();
        if (brightness >= 0 && brightness < 100) {
          brightness = min(100, brightness + 10);
          lightOutside.setValue(brightness);
          addCommand("LO:" + String(brightness));
          refreshLeds();
        }
      }
      if (trellis.justPressed(6)) {
        int brightness = lightOutside.value();
        if (brightness > 0) {
          brightness = brightness - 10;
          if (brightness < 0) brightness = 0;
          lightOutside.setValue(brightness);
          addCommand("LO:" + String(brightness));
          refreshLeds();
        }
      }
      if (trellis.justPressed(10)) {
        int brightness = lightOutside.value();
        if (brightness == 0) {
          lightOutside.setValue(100);
          addCommand("LO:100");
          String mode = lightModes.substring(0, 1);
          lightOutsideMode.setValue(mode);
          addCommand("MO:" + mode);
          refreshLeds();
        } else if (brightness > 0) {
          String mode = lightOutsideMode.value();
          if (mode.length() > 0) {
            int currentMode = lightModes.indexOf(mode) + 1;
            if (currentMode >= lightModes.length()) currentMode = 0;
            mode = lightModes.substring(currentMode, currentMode + 1);
            lightOutsideMode.setValue(mode);
            addCommand("MO:" + mode);
          }
        }
      }

      // Inside lights
      if (trellis.justPressed(3)) {
        int brightness = lightInside.value();
        if (brightness >= 0 && brightness < 100) {
          brightness = min(100, brightness + 10);
          lightInside.setValue(brightness);
          addCommand("LI:" + String(brightness));
          refreshLeds();
        }
      }
      if (trellis.justPressed(7)) {
        int brightness = lightInside.value();
        if (brightness > 0) {
          brightness = brightness - 10;
          if (brightness < 0) brightness = 0;
          lightInside.setValue(brightness);
          addCommand("LI:" + String(brightness));
          refreshLeds();
        }
      }
      if (trellis.justPressed(11)) {
        int brightness = lightInside.value();
        if (brightness == 0) {
          lightInside.setValue(100);
          addCommand("LI:100");
          String mode = lightModes.substring(0, 1);
          lightInsideMode.setValue(mode);
          addCommand("MI:" + mode);
          refreshLeds();
        } else if (brightness > 0) {
          String mode = lightInsideMode.value();
          if (mode.length() > 0) {
            int currentMode = lightModes.indexOf(mode) + 1;
            if (currentMode >= lightModes.length()) currentMode = 0;
            mode = lightModes.substring(currentMode, currentMode + 1);
            lightInsideMode.setValue(mode);
            addCommand("MI:" + mode);
          }
        }
      }

      // All lights
      if (trellis.justPressed(5) && (lightInside.value() > 0 || lightOutside.value() > 0)) {
        lightOutside.setValue(0);
        lightInside.setValue(0);
        addCommand("LO:0");
        addCommand("LI:0");
        refreshLeds();
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
        refreshLeds();
        scheduleScreenRefresh();
      } else if (command.startsWith("Tt")) {
        thermostatTarget = command.substring(2).toFloat();
        scheduleScreenRefresh();
      } else if (command.startsWith("LO")) {
        lightOutsideMode.setValue(command.substring(2, 3));
        lightOutside.setValue(command.substring(3).toInt());
        refreshLeds();
      } else if (command.startsWith("LI")) {
        lightInsideMode.setValue(command.substring(2, 3));
        lightInside.setValue(command.substring(3).toInt());
        refreshLeds();
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
        img.printf("F\n");
      }
    }
  
  private:
    String lightModes = "CRT";
    VolatileValue<float> temperatureOutside = VolatileValue<float>(0);
    VolatileValue<float> temperatureInside = VolatileValue<float>(0);
    VolatileValue<int> lightOutside = VolatileValue<int>(-1);
    VolatileValue<int> lightInside = VolatileValue<int>(-1);
    VolatileValue<String> lightOutsideMode = VolatileValue<String>("");
    VolatileValue<String> lightInsideMode = VolatileValue<String>("");
    float thermostatTarget = 71.0;
    bool  thermostatOn = false;

    void refreshLeds() {
      if (thermostatOn) {
        trellis.setLED(1);
      } else {
        trellis.clrLED(1);
      }
      if (lightOutside.value() > 0) {
        trellis.setLED(2);
        trellis.setLED(6);
        trellis.setLED(10);
      } else {
        trellis.clrLED(2);
        trellis.clrLED(6);
        trellis.clrLED(10);
      }
      if (lightInside.value() > 0) {
        trellis.setLED(3);
        trellis.setLED(7);
        trellis.setLED(11);
      } else {
        trellis.clrLED(3);
        trellis.clrLED(7);
        trellis.clrLED(11);
      }
      if (lightInside.value() > 0 || lightOutside.value() > 0) {
        trellis.setLED(5);
      } else {
        trellis.clrLED(5);
      }
      trellis.writeDisplay();
    }
};

#endif
