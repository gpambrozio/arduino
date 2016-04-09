#ifndef Mode_h
#define Mode_h

#include "Arduino.h"

class Mode
{
  public:
    Mode();
    virtual void init();
    virtual bool step(unsigned long dt);
  protected:
    uint32_t wheel(byte WheelPos);
};

#endif

