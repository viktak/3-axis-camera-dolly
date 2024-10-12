#ifndef STEPPER_H
#define STEPPER_H

#include <Arduino.h>
#include <AccelStepper.h>
#include <MultiStepper.h>


#define STEPPER_SLIDER_DIR_GPIO 2
#define STEPPER_SLIDER_STEP_GPIO 1

#define STEPPER_PAN_DIR_GPIO 4
#define STEPPER_PAN_STEP_GPIO 3

#define STEPPER_TILT_DIR_GPIO 6
#define STEPPER_TILT_STEP_GPIO 5

#define STEPPER_ENABLE 7

#define SLIDER_HALL_A_GPIO 11
#define SLIDER_HALL_B_GPIO 12
#define PAN_HALL_GPIO 13
#define TILT_HALL_GPIO 14


#define STEPPER_SLIDER_MAX_SPEED 8000
#define STEPPER_PAN_MAX_SPEED 6000
#define STEPPER_TILT_MAX_SPEED 3000

#define STEPPER_SLIDER_ACCELERATION 20
#define STEPPER_PAN_ACCELERATION 20
#define STEPPER_TILT_ACCELERATION 20

#define METER_DELAY 2000

enum AXIS
{
    AXIS_SLIDER = 0,
    AXIS_PAN,
    AXIS_TILT
};

enum OPERATIONMODE
{
    TEST = 0,
    NORMAL,
    CALIBRATING,
    MOVE_IN_POSITION,
    TIMELAPSE,
    PANNING_VIDEO
};

enum CALIBRATION_STAGE
{
    STARTING = 0,
    DETECT_RIGHT,
    DETECT_LEFT,
    MOVE_TO_START_POSITION,
    FINISHED
};

enum TIMELAPSE_STAGE
{
    TIMELAPSE_START = 0,
    TIMELAPSE_MOVE_IN_POSITION,
    TIMELAPSE_DELAY_BEFORE_SHOOT,
    TIMELAPSE_METER,
    TIMELAPSE_SHOOT,
    TIMELAPSE_FINISHED
};

enum PANNINGVIDEO_STAGE
{
    PV_START = 0,
    PV_MOVE_IN_POSITION,
    PV_METER,
    PV_FINISHING,
    PV_FINISHED
};

typedef struct
{
    char *data;
    size_t length;
} stepperCommandInfo;

typedef struct
{
    uint16_t numberOfShots;
    uint16_t interval;
    uint8_t exposureLength;

    long StartPositionSlider;
    long EndPositionSlider;

    long StartPositionPan;
    long EndPositionPan;

    long StartPositionTilt;
    long EndPositionTilt;

    uint16_t currentShot;

    long distanceBetweenShotsSlider;
    long distanceBetweenShotsPan;
    long distanceBetweenShotsTilt;

} timelapseInfo;

typedef struct
{
    long StartPositionSlider;
    long EndPositionSlider;

    long StartPositionPan;
    long EndPositionPan;

    long StartPositionTilt;
    long EndPositionTilt;

    uint16_t speed;
    bool startVideoBeforeMoving;
    bool stopVideoBeforeStopping;

} panningVideoInfo;

extern OPERATIONMODE opMode;
extern CALIBRATION_STAGE calStage;

extern stepperCommandInfo sci;
extern String sc;

extern long maxPanPosition;
extern long maxTiltPosition;
extern long maxSliderPosition;

extern AccelStepper stepper_slider;
extern AccelStepper stepper_pan;
extern AccelStepper stepper_tilt;


void CalibrateSteppers();
void loopStepper();
void setupStepperMotors();

#endif