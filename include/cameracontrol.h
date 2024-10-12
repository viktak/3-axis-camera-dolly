#ifndef CAMERACONTROL_H
#define CAMERACONTROL_H

#include <Arduino.h>

#define CAMERA_METER_GPIO 48
#define CAMERA_TRIGGER_GPIO 47

extern void ActivateMeter();
extern void DeactivateMeter();
extern void ActivateExposure();
extern void DeactivateExposure();

extern void setupCameraControl();
extern void loopCameraControl();


#endif