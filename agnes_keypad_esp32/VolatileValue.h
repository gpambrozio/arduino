#ifndef VolatileValue_h
#define VolatileValue_h

#include "Arduino.h"

template<typename T> class VolatileValue
{
  public:
    VolatileValue(T noValue, long timeout = 120) {
      this->noValue = noValue;
      this->lastValue = noValue;
      this->lastUpdate = 0;
      this->timeout = timeout * 1000;
    }
    T value() {
      if (millis() > lastUpdate + timeout) {
        return noValue;
      } else {
        return lastValue;
      }
    }
    void setValue(T value) {
      lastValue = value;
      lastUpdate = millis();
    }

  private:
    T lastValue;
    T noValue;
    long lastUpdate;
    long timeout;
};

#endif
