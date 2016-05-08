#ifndef Mode_h
#define Mode_h

#include "Arduino.h"
#include "CaneCommon.h"

class Mode
{
  public:
    Mode();
    virtual void init();
    virtual String name();
    virtual bool step(unsigned long dt);
  protected:
    CHSV wheel(byte WheelPos);
    CHSV wheel(byte WheelPos, int brightness);
};

#endif

