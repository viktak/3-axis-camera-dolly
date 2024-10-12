#include "battery.h"
#include "common.h"

ESP32AnalogRead battery;

unsigned long batteryMillis = 0;

BATTERYSTATUS batteryStatus = BATTERYSTATUS::BATT_FULL;

float GetBatteryLevel()
{
    uint16_t rawValue = 0;

    //  Shift down old values
    for (size_t i = 5; i > 0; i--)
        batteryRawValue[i] = batteryRawValue[i - 1];

    //  Read and store new value
    rawValue = analogRead(ADC_BATTERY_GPIO);
    batteryRawValue[0] = (float)rawValue;

    float myaverage = (batteryRawValue[0] + batteryRawValue[1] + batteryRawValue[2] + batteryRawValue[3] + batteryRawValue[4] + batteryRawValue[5]) / 6;

    if (myaverage < LOW_BATTERY_LEVEL)
        if (batteryStatus == BATTERYSTATUS::BATT_FULL)
        {
            batteryStatus = BATTERYSTATUS::BATT_LOW;
            SerialMon.println("Battery level is getting low...");
        }

    if (myaverage < CUTOFF_BATTERY_LEVEL)
        if (batteryStatus == BATTERYSTATUS::BATT_LOW)
        {
            batteryStatus = BATTERYSTATUS::BATT_CRITICAL;
            SerialMon.println("Battery level is critical. Suspending motor operations...");
        }

    return myaverage;
}

void setupBattery()
{
    pinMode(ADC_BATTERY_GPIO, ANALOG);
    analogReadResolution(ADC_RESOLUTION_BITS);

    //  First time fill buffer
    for (size_t i = 0; i < 5; i++)
    {
        uint16_t rawValue = 0;
        rawValue = analogRead(ADC_BATTERY_GPIO);
        batteryRawValue[i] = (float)rawValue;
    }
}

void loopBattery()
{
    if (millis() - batteryMillis > 1000)
    {
        // SerialMon.println(GetBatteryLevel());
        GetBatteryLevel();
        batteryMillis = millis();
    }
}