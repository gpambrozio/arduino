#include "Common.h"

void writeCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  AddressableCharacteristic *addrChar = (AddressableCharacteristic*)&chr;
  addrChar->strip->writeCallback(data, len);
}

void AddressableCharacteristic::setCallback() {
  setWriteCallback(writeCallback);
}
