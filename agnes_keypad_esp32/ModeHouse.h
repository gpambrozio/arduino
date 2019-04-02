#ifndef ModeHouse_h
#define ModeHouse_h

#include "Mode.h"

class Strip
{
  public:
    Strip(int baseKey, String identifier) : baseKey(baseKey), identifier(identifier) {}
    bool checkKeys() {
      bool shouldRefreshLeds = false;
      if (trellis.justPressed(baseKey)) {
        if (brightness < 100) {
          if (brightness == 0) {
            lightMode = lightModes.substring(0, 1);
          }
          brightness = min(100, brightness + 10);
          sendState();
          shouldRefreshLeds = true;
        }
      }
      if (trellis.justPressed(baseKey+4)) {
        if (brightness > 0) {
          brightness -= 10;
          if (brightness < 0) brightness = 0;
          sendState();
          shouldRefreshLeds = true;
        }
      }
      if (trellis.justPressed(baseKey+8)) {
        if (brightness == 0) {
          brightness = 100;
          lightMode = lightModes.substring(0, 1);
          sendState();
          shouldRefreshLeds = true;
        } else if (brightness > 0) {
          int currentMode = lightModes.indexOf(lightMode) + 1;
          if (currentMode >= lightModes.length()) {
            turnOff();
          } else {
            lightMode = lightModes.substring(currentMode, currentMode + 1);
            sendState();
          }
          shouldRefreshLeds = true;
        }
      }
      
      if (shouldRefreshLeds) {
        refreshLeds();
      }
      return shouldRefreshLeds;
    }

    void turnOff() {
      brightness = 0;
      sendState();
    }

    void refreshLeds() {
      if (brightness > 0) {
        trellis.setLED(baseKey);
        trellis.setLED(baseKey+4);
        trellis.setLED(baseKey+8);
      } else {
        trellis.clrLED(baseKey);
        trellis.clrLED(baseKey+4);
        trellis.clrLED(baseKey+8);
      }
      trellis.writeDisplay();
    }
    
    bool isOn() {
      return brightness > 0;
    }
  
    bool checkCommand(String command) {
      if (command.startsWith("L" + identifier)) {
        lightMode = command.substring(2, 3);
        brightness = command.substring(3).toInt();
        refreshLeds();
        return true;
      }
      return false;
    }

  private:
    String lightModes = "CRT";
    int baseKey;
    String identifier;
    int brightness = 0;
    String lightMode = "C";

    void sendState() {
      addCommand("Light:" + identifier + ":" + lightMode + ":" + String(brightness));
    }
};

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

      // Lock / unlock
      if (trellis.justPressed(8)) {
        addCommand("Locks:lock");
      } else if (trellis.justPressed(9)) {
        addCommand("Locks:unlock");
      }

      // Strips
      if (outside.checkKeys() ||
          inside.checkKeys()) {
        refreshLeds();
      }

      // All lights
      if (trellis.justPressed(5) && (inside.isOn() || outside.isOn())) {
        inside.turnOff();
        outside.turnOff();
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
      } else if (command.startsWith("Bo")) {
        batteryOutside.setValue(command.substring(2).toInt());
        scheduleScreenRefresh();
      } else if (command.startsWith("TO")) {
        thermostatOn = command.substring(2).toInt() != 0;
        refreshLeds();
        scheduleScreenRefresh();
      } else if (command.startsWith("Tt")) {
        thermostatTarget = command.substring(2).toFloat();
        scheduleScreenRefresh();
      }
      if (inside.checkCommand(command) ||
          outside.checkCommand(command)) {
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
        img.printf("F");
        value = batteryOutside.value();
        if (value > 0 && value <= 30) {
          img.setTextFont(1);
          img.printf(" %.0f%%", value);
        }
        img.printf("\n");
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
    Strip inside = Strip(2, "I");
    Strip outside = Strip(3, "O");
    VolatileValue<float> temperatureOutside = VolatileValue<float>(0);
    VolatileValue<float> temperatureInside = VolatileValue<float>(0);
    VolatileValue<int> batteryOutside = VolatileValue<int>(0);
    float thermostatTarget = 71.0;
    bool  thermostatOn = false;

    void refreshLeds() {
      if (thermostatOn) {
        trellis.setLED(1);
      } else {
        trellis.clrLED(1);
      }
      inside.refreshLeds();
      outside.refreshLeds();

      if (inside.isOn() || outside.isOn()) {
        trellis.setLED(5);
      } else {
        trellis.clrLED(5);
      }
      trellis.writeDisplay();
    }
};

#endif
