#include "Common.h"

void writeCallback(BLECharacteristic& chr, uint8_t* data, uint16_t len, uint16_t offset) {
  AddressableCharacteristic *addrChar = (AddressableCharacteristic*)&chr;
  addrChar->strip->writeCallback(data, len, offset);
}

void AddressableCharacteristic::setCallback() {
  setWriteCallback(writeCallback);
}
