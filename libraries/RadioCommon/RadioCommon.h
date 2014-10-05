#ifndef RadioCommon_h
#define RadioCommon_h

#include "Arduino.h"

//
// Topology
//

enum RADIO_MODULES {
    RADIO_SERVER = 0,
    RADIO_DRAPE,
    RADIO_ALARM,
	RADIO_MOBILE,
    RADIO_TOTAL_COUNT,
};

// Radio pipe addresses.
extern const uint64_t pipes[RADIO_TOTAL_COUNT];

#define START_RADIO(radio, address)  {              \
  radio.begin();                                    \
  radio.setRetries(15,15);                          \
  radio.setPayloadSize(sizeof(unsigned long));      \
  radio.openReadingPipe(1,pipes[address]);          \
  radio.setPALevel(RF24_PA_HIGH);                   \
  radio.setDataRate(RF24_250KBPS);                  \
  radio.startListening();                           \
}

#endif
