#ifndef AddressableCharacteristic_h
#define AddressableCharacteristic_h

#include "Strip.h"

class Strip;
class AddressableCharacteristic: public BLECharacteristic
{
  public:
    AddressableCharacteristic(BLEUuid bleuuid, Strip *strip) : BLECharacteristic(bleuuid), strip(strip)
    {}

    virtual void setCallback();
    Strip *strip;
};

#endif
