#ifndef RTC_h
#define RTC_h

#include "Arduino.h"

extern byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

void RTCStart();
void RTCGetDateDs1307();
void RTCSetDateDs1307();
void RTCPrintDateToSerial();

#endif
