#include "cameracontrol.h"

unsigned long cameraMillis = 0;
unsigned long exposureMillis = 0;

void toggleMeter()
{
    digitalWrite(CAMERA_METER_GPIO, 1 - digitalRead(CAMERA_METER_GPIO));
}

void toggleTrigger()
{
    digitalWrite(CAMERA_TRIGGER_GPIO, 1 - digitalRead(CAMERA_TRIGGER_GPIO));
}

void ActivateMeter()
{
    digitalWrite(CAMERA_METER_GPIO, LOW);
}

void DeactivateMeter()
{
    digitalWrite(CAMERA_METER_GPIO, HIGH);
}

void ActivateExposure()
{
    digitalWrite(CAMERA_TRIGGER_GPIO, LOW);
    exposureMillis = millis();
}

void DeactivateExposure()
{
    digitalWrite(CAMERA_TRIGGER_GPIO, HIGH);
}

void setupCameraControl()
{
    pinMode(CAMERA_METER_GPIO, OUTPUT);
    pinMode(CAMERA_TRIGGER_GPIO, OUTPUT);

    digitalWrite(CAMERA_METER_GPIO, HIGH);
    digitalWrite(CAMERA_TRIGGER_GPIO, HIGH);
}

void loopCameraControl()
{
    // if (millis() - cameraMillis > 250)
    // {
    //     toggleMeter();
    //     toggleTrigger();
    //     cameraMillis = millis();
    // }
}
