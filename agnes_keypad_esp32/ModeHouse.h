#ifndef ModeHouse_h
#define ModeHouse_h

#include "Mode.h"

class Strip
{
  public:
    Strip(int baseKey, String identifier) : baseKey(baseKey), identifier(identifier) {}
    
    bool checkKeys(bool isActive) {
      bool shouldRefreshLeds = false;
      if (justPressed(baseKey)) {
        if (brightness < 100) {
          if (brightness == 0) {
            lightMode = lightModes.substring(0, 1);
          }
          brightness = min(100, brightness + 10);
          sendState();
          shouldRefreshLeds = isActive;
        }
      }
      if (justPressed(baseKey+4)) {
        if (brightness > 0) {
          brightness -= 10;
          if (brightness < 0) brightness = 0;
          sendState();
          shouldRefreshLeds = isActive;
        }
      }
      if (justPressed(baseKey+8)) {
        if (brightness == 0) {
          brightness = 100;
          lightMode = lightModes.substring(0, 1);
          sendState();
          shouldRefreshLeds = isActive;
        } else if (brightness > 0) {
          int currentMode = lightModes.indexOf(lightMode) + 1;
          if (currentMode >= lightModes.length()) {
            turnOff();
          } else {
            lightMode = lightModes.substring(currentMode, currentMode + 1);
            sendState();
          }
          shouldRefreshLeds = isActive;
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
      setLED(baseKey, brightness > 0);
      setLED(baseKey+4, brightness > 0);
      setLED(baseKey+8, brightness > 0);
    }
    
    bool isOn() {
      return brightness > 0;
    }
  
    bool checkCommand(String command, bool isActive) {
      if (command.startsWith("L" + identifier)) {
        lightMode = command.substring(2, 3);
        brightness = command.substring(3).toInt();
        if (isActive) refreshLeds();
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
    virtual char identifier() { return 'h'; }
    virtual void init() {}
    virtual void setup() {
      setKeysBrightness(1);
      refreshLeds();
    }
    
    virtual void checkKeys() {
      // Thermostat
      if (justPressed(0)) {
        thermostatTarget += 1.0;
        addCommand("ThermostatTarget:" + String(thermostatTarget, 0));
        scheduleScreenRefresh();
      }
      if (justPressed(4)) {
        thermostatTarget -= 1.0;
        addCommand("ThermostatTarget:" + String(thermostatTarget, 0));
        scheduleScreenRefresh();
      }
      if (justPressed(1)) {
        thermostatOn = !thermostatOn;
        addCommand("ThermostatOnOff:" + String(thermostatOn ? "1" : "0"));
        scheduleScreenRefresh();
        refreshLeds();
      }

      // Fan
      if (justPressed(3)) {
        addCommand("Fan:U");
      }
      if (justPressed(7)) {
        addCommand("Fan:D");
      }
      if (justPressed(11)) {
        addCommand("Fan:C");
      }

      // Lock / unlock
      if (justPressed(8)) {
        addCommand("Locks:lock");
      } else if (justPressed(9)) {
        addCommand("Locks:unlock");
      }

      // Strips
      if (/*outside.checkKeys(isActive) ||*/
          inside.checkKeys(isActive)) {
        refreshLeds();
      }

      // All lights
      if (justPressed(5) && (inside.isOn() /*|| outside.isOn()*/)) {
        inside.turnOff();
//        outside.turnOff();
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
      } else if (command.startsWith("Fp")) {
        fanPosition = command.substring(2).toInt();
        refreshLeds();
      }
      if (inside.checkCommand(command, isActive) /*||
          outside.checkCommand(command, isActive)*/) {
        refreshLeds();
      }
    }
    
    virtual void draw() {
      img.println("");
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
//    Strip outside = Strip(3, "O");
    VolatileValue<float> temperatureOutside = VolatileValue<float>(0);
    VolatileValue<float> temperatureInside = VolatileValue<float>(0);
    VolatileValue<int> batteryOutside = VolatileValue<int>(0);
    float thermostatTarget = 71.0;
    bool  thermostatOn = false;
    int fanPosition = 0;

    void refreshLeds() {
      if (!isActive) return;
      setLED(1, thermostatOn);
      setLED(3, fanPosition > 0);
      setLED(7, fanPosition > 0);
      setLED(11, fanPosition > 0);
      inside.refreshLeds();
//      outside.refreshLeds();
      setLED(5, inside.isOn()/* || outside.isOn()*/);
    }
};

#endif
