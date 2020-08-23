/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 * Connecting the radio: https://www.mysensors.org/build/connect_radio
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * This is an example that demonstrates how to report the battery level for a sensor
 * Instructions for measuring battery capacity on A0 are available here:
 * http://www.mysensors.org/build/battery
 *
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define   BATTERY_SENSE_PIN  A0
#define   MOISTURE_PIN       A1

const uint32_t SLEEP_TIME = 5 * 60000;  // sleep time between reads (seconds * 1000 milliseconds)

#define CHILD_ID 0
MyMessage msgMoisture(CHILD_ID, V_LEVEL);
MyMessage msgVoltage(CHILD_ID+1, V_VOLTAGE);

void setup()
{
	// use the 3.3V reference
	analogReference(DEFAULT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(BATTERY_SENSE_PIN, INPUT);
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
  sendSketchInfo("Soil Moisture Sensor", "1.0");
  present(CHILD_ID, S_MOISTURE);
  present(CHILD_ID+1, S_MULTIMETER);
}

const int AirValue = 900;
const int WaterValue = 385;
float oldBatteryPcnt = 0;

void loop()
{
	// get the battery Voltage
	int sensorValue = analogRead(BATTERY_SENSE_PIN);
  int soilMoistureValue = analogRead(MOISTURE_PIN);
  int soilMoisturePercent = constrain(map(soilMoistureValue, AirValue, WaterValue, 0, 100), 0, 100);

  // 1M, 1M divider across battery and using internal ADC ref of 3.3V
  // ((1e6+1e6)/1e6)*3.3 = Vmax = 6.6 Volts
  // 6.6/1023 = Volts per bit = 0.006451612903226

  float batteryV = sensorValue * 0.06451612903226;
  float batteryPcnt = constrain(map(batteryV, 33, 40, 0, 1000), 0, 1000) / 10;

  if (oldBatteryPcnt != batteryPcnt) {
    // Power up radio after sleep
    sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
  }

#ifdef MY_DEBUG
  Serial.println(sensorValue);
	Serial.print("Battery Voltage: ");
	Serial.print(batteryV);
	Serial.println(" V");

	Serial.print("Battery percent: ");
	Serial.print(batteryPcnt);
	Serial.println(" %");

  Serial.println(soilMoistureValue);

  Serial.print(soilMoisturePercent);
  Serial.println("%");
#endif
  send(msgMoisture.set((int32_t)ceil(soilMoisturePercent)));
  send(msgVoltage.set((float)batteryV/10.0, 2));
	sleep(SLEEP_TIME);
}
