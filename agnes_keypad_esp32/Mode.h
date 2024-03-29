#ifndef Mode_h
#define Mode_h

#include "Arduino.h"
#include "Common.h"

class Mode
{
  public:
    Mode();
    virtual void init();
    virtual char identifier();
    virtual String name();
    virtual void setup();
    virtual void tearDown();
    virtual void checkKeys();
    virtual void checkCommand(String command);
    virtual void draw();
    bool isActive = false;
  protected:
};

#endif
