#ifndef BATTERY_H
#define BATTERY_H

#include "ESP32AnalogRead.h"
#include <Arduino.h>

#define ADC_RESOLUTION_BITS 12
#define ADC_RESOLUTION pow(2, ADC_RESOLUTION_BITS)

//  Determine the following 2 values by applying a known voltage and enabling "SerialMon.println(GetBatteryLevel());" in battery.cpp temporarily.
//  These values depend on the resistor divider used to measure battery voltage.
#define LOW_BATTERY_LEVEL (float)1835
#define CUTOFF_BATTERY_LEVEL (float)1793

#define ADC_BATTERY_GPIO 10

static float batteryRawValue[6];

enum BATTERYSTATUS
{
    BATT_NULL = 0,
    BATT_FULL,
    BATT_LOW,
    BATT_CRITICAL
};

extern BATTERYSTATUS batteryStatus;

float GetBatteryLevel();

void setupBattery();
void loopBattery();

#endif