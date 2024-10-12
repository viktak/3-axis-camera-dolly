#include "stepper.h"

#include <ArduinoJson.h>

#include "main.h"
#include "common.h"
#include "battery.h"
#include "cameracontrol.h"

AccelStepper stepper_slider = AccelStepper(AccelStepper::DRIVER, STEPPER_SLIDER_DIR_GPIO, STEPPER_SLIDER_STEP_GPIO);
AccelStepper stepper_pan = AccelStepper(AccelStepper::DRIVER, STEPPER_PAN_DIR_GPIO, STEPPER_PAN_STEP_GPIO);
AccelStepper stepper_tilt = AccelStepper(AccelStepper::DRIVER, STEPPER_TILT_DIR_GPIO, STEPPER_TILT_STEP_GPIO);

MultiStepper multiStepper;

float slider_max_speed = STEPPER_SLIDER_MAX_SPEED;
float pan_max_speed = STEPPER_PAN_MAX_SPEED;
float tilt_max_speed = STEPPER_TILT_MAX_SPEED;

long targetPositions[3];

long maxPanPosition = 0;
long maxTiltPosition = 0;

bool isStepperSliderCalibrated = false;
bool isStepperPanCalibrated = false;
bool isStepperTiltCalibrated = false;

unsigned long stepperMillis = 0;
unsigned long meterMillis = 0;
unsigned long triggerMillis = 0;
unsigned long delayMillis = 0;

OPERATIONMODE opMode = OPERATIONMODE::NORMAL;

CALIBRATION_STAGE calStage = CALIBRATION_STAGE::STARTING;
TIMELAPSE_STAGE tlStage = TIMELAPSE_STAGE::TIMELAPSE_MOVE_IN_POSITION;
PANNINGVIDEO_STAGE pvStage = PANNINGVIDEO_STAGE::PV_START;

bool FullCalibration = false;

timelapseInfo tli;
panningVideoInfo pvi;

stepperCommandInfo sci;
String sc;

void HallInterruptHandler()
{
}

void DisplayReport()
{
    SerialMon.printf("Current position:\t\t%i, %i, %i\r\n", stepper_slider.currentPosition(), stepper_pan.currentPosition(), stepper_tilt.currentPosition());
    SerialMon.printf("Maximum position:\t\t%i, %i, %i\r\n", appSettings.maxSliderPosition, appSettings.maxPanPosition, appSettings.maxTiltPosition);
    SerialMon.printf("Maximum speed:\t\t\t%f, %f, %f\r\n", stepper_slider.maxSpeed(), stepper_pan.maxSpeed(), stepper_tilt.maxSpeed());
    SerialMon.printf("Current speed [steps/s]:\t%f, %f, %f\r\n", stepper_slider.speed(), stepper_pan.speed(), stepper_tilt.speed());
}

void enableSteppers(const bool en)
{
    stepper_slider.disableOutputs();
    stepper_pan.disableOutputs();
    stepper_tilt.disableOutputs();
    digitalWrite(STEPPER_ENABLE, !en);
}

void settargetPositions(long slider, long pan, long tilt)
{
    targetPositions[AXIS::AXIS_SLIDER] = slider;
    targetPositions[AXIS::AXIS_PAN] = pan;
    targetPositions[AXIS::AXIS_TILT] = tilt;

    multiStepper.moveTo(targetPositions);
}

void SetStepperSpeedPercent(uint8_t percent)
{
    stepper_slider.setMaxSpeed((float)STEPPER_SLIDER_MAX_SPEED / (float)100 * (float)percent);
    stepper_pan.setMaxSpeed((float)STEPPER_PAN_MAX_SPEED / (float)100 * (float)percent);
    stepper_tilt.setMaxSpeed((float)STEPPER_TILT_MAX_SPEED / (float)100 * (float)percent);

    // SerialMon.printf("Stepper speed: %f - %f - %f\r\n", stepper_slider.maxSpeed(), stepper_pan.maxSpeed(), stepper_tilt.maxSpeed());
}

void StopStepper(AXIS axis)
{
    switch (axis)
    {
    case AXIS::AXIS_SLIDER:
        stepper_slider.stop();
        break;
    case AXIS::AXIS_PAN:
        stepper_pan.stop();
        break;
    case AXIS::AXIS_TILT:
        stepper_tilt.stop();
        break;

    default:
        break;
    }
}

void CalibrateSteppers()
{
    SerialMon.println("Starting calibration.");

    //  Slider
    SerialMon.println("Calibrating SLIDER axis...");
    SetStepperSpeedPercent(100);

    if (appSettings.maxSliderPosition < 0)
    {

        settargetPositions(999999, stepper_pan.currentPosition(), stepper_tilt.currentPosition());
        while (multiStepper.run() && (digitalRead(SLIDER_HALL_B_GPIO) > 0))
        {
        }
        stepper_slider.stop();
        SerialMon.println("Found right edge.");

        stepper_slider.setCurrentPosition(0);
    }

    settargetPositions(-999999, stepper_pan.currentPosition(), stepper_tilt.currentPosition());
    while (multiStepper.run() && (digitalRead(SLIDER_HALL_A_GPIO) > 0))
    {
    }
    StopStepper(AXIS_SLIDER);

    SerialMon.println("Found left edge.");

    if (appSettings.maxSliderPosition < 0)
    {
        appSettings.maxSliderPosition = abs(stepper_slider.currentPosition()); //  Maximum length available in the future
        appSettings.Save();
    }

    stepper_slider.setCurrentPosition(0); //  New "home" location

    //  Pan
    SerialMon.println("Calibrating PAN axis...");

    if (appSettings.maxPanPosition < 0)
    {
        settargetPositions(stepper_slider.currentPosition(), -999999, stepper_tilt.currentPosition());
        while (multiStepper.run() && (digitalRead(PAN_HALL_GPIO) != 0))
        {
        }
        settargetPositions(stepper_slider.currentPosition(), -999999, stepper_tilt.currentPosition());
        while (multiStepper.run() && (digitalRead(PAN_HALL_GPIO) == 0))
        {
        }
    }

    stepper_pan.setCurrentPosition(0);
    settargetPositions(stepper_slider.currentPosition(), -999999, stepper_pan.currentPosition());
    while (multiStepper.run() && (digitalRead(PAN_HALL_GPIO) != 0))
    {
    }
    StopStepper(AXIS_PAN);
    SerialMon.println("PAN axis calibrated.");

    if (appSettings.maxPanPosition < 0)
    {
        appSettings.maxPanPosition = abs(stepper_pan.currentPosition()); //  Maximum length available in the future
        appSettings.Save();
    }

    stepper_pan.setCurrentPosition(0); //  New "home" location

    //  Tilt
    SerialMon.println("Calibrating TILT axis...");

    if (appSettings.maxTiltPosition < 0)
    {
        settargetPositions(stepper_slider.currentPosition(), stepper_pan.currentPosition(), -999999);
        while (multiStepper.run() && (digitalRead(TILT_HALL_GPIO) != 0))
        {
        }
        settargetPositions(stepper_slider.currentPosition(), stepper_pan.currentPosition(), -999999);
        while (multiStepper.run() && (digitalRead(TILT_HALL_GPIO) == 0))
        {
        }
    }

    stepper_tilt.setCurrentPosition(0);
    settargetPositions(stepper_slider.currentPosition(), stepper_pan.currentPosition(), -999999);
    while (multiStepper.run() && (digitalRead(TILT_HALL_GPIO) != 0))
    {
    }
    StopStepper(AXIS_TILT);
    SerialMon.println("TILT axis calibrated.");

    if (appSettings.maxTiltPosition < 0)
    {
        appSettings.maxTiltPosition = abs(stepper_tilt.currentPosition()); //  Maximum length available in the future
        appSettings.Save();
    }

    stepper_tilt.setCurrentPosition(0); //  New "home" location

    //  Finished

    DisplayReport();
    isStepperSliderCalibrated = true;
    SerialMon.println("Calibratiion finished.");
}

void MoveSlider(AXIS axis, int dir, long speed)
{
    switch (axis)
    {
    case AXIS::AXIS_SLIDER:
        stepper_slider.setMaxSpeed(speed);
        settargetPositions(dir * 999999, stepper_pan.currentPosition(), stepper_tilt.currentPosition());
        break;
    case AXIS::AXIS_PAN:
        stepper_pan.setMaxSpeed(speed);
        settargetPositions(stepper_slider.currentPosition(), dir * 999999, stepper_tilt.currentPosition());
        break;
    case AXIS::AXIS_TILT:
        stepper_tilt.setMaxSpeed(speed);
        settargetPositions(stepper_slider.currentPosition(), stepper_pan.currentPosition(), dir * 999999);
        break;

    default:
        break;
    }
}

void GotoPositions(long sliderPosition, long panPosition, long tiltPosition)
{
    if (isStepperSliderCalibrated)
    {
        SerialMon.printf("Current position:\t%i : %i : %i\r\n", stepper_slider.currentPosition(), stepper_pan.currentPosition(), stepper_tilt.currentPosition());
        SerialMon.printf("New position:\t\t%i : %i : %i\r\n", sliderPosition, panPosition, tiltPosition);

        SetStepperSpeedPercent(100);
        settargetPositions(sliderPosition, panPosition, tiltPosition);

        while (multiStepper.run())
        {
        }

        SerialMon.printf("Final position:\t\t%i : %i : %i\r\n", stepper_slider.currentPosition(), stepper_pan.currentPosition(), stepper_tilt.currentPosition());
    }
}

void setupStepperMotors()
{

    pinMode(SLIDER_HALL_A_GPIO, INPUT);
    pinMode(SLIDER_HALL_B_GPIO, INPUT);
    pinMode(PAN_HALL_GPIO, INPUT);
    pinMode(TILT_HALL_GPIO, INPUT);

    attachInterrupt(digitalPinToInterrupt(SLIDER_HALL_A_GPIO), HallInterruptHandler, FALLING);
    attachInterrupt(digitalPinToInterrupt(SLIDER_HALL_B_GPIO), HallInterruptHandler, FALLING);
    attachInterrupt(digitalPinToInterrupt(PAN_HALL_GPIO), HallInterruptHandler, FALLING);
    attachInterrupt(digitalPinToInterrupt(TILT_HALL_GPIO), HallInterruptHandler, FALLING);

    SetStepperSpeedPercent(100);

    stepper_slider.setAcceleration(STEPPER_SLIDER_ACCELERATION);
    stepper_pan.setAcceleration(STEPPER_PAN_ACCELERATION);
    stepper_tilt.setAcceleration(STEPPER_TILT_ACCELERATION);

    multiStepper.addStepper(stepper_slider);
    multiStepper.addStepper(stepper_pan);
    multiStepper.addStepper(stepper_tilt);

    pinMode(STEPPER_ENABLE, OUTPUT_OPEN_DRAIN);

    enableSteppers(true);
}

void loopStepper()
{
    if ((batteryStatus == BATTERYSTATUS::BATT_FULL) || (batteryStatus == BATTERYSTATUS::BATT_LOW))
    {
        //  Battery is OK
        switch (opMode)
        {
        case OPERATIONMODE::TEST:
        {
            if (!multiStepper.run())
            {
                long targetPositions1 = random(0, 10000);
                long targetPositions2 = random(0, 10000);
                long targetPositions3 = random(0, 10000);

                SerialMon.printf("Going to position: %i\t%i\t%i...\r\n", targetPositions1, targetPositions2, targetPositions3);

                SetStepperSpeedPercent(100);

                settargetPositions(targetPositions1, targetPositions2, targetPositions3);

                // multiStepper.runSpeedToPosition();
            }

            break;
        }

        case OPERATIONMODE::NORMAL:
        {
            if (sci.length)
            {
                JsonDocument doc;

                DeserializationError error = deserializeJson(doc, sci.data, sci.length);
                sci.length = 0;

                if (error)
                {
                    Serial.printf("deserializeJson() failed: %s\r\n", error.c_str());
                    return;
                }

                const char *command = doc["command"];

                if (!strcmp(command, "calibrate"))
                {
                    appSettings.maxSliderPosition = -1;
                    appSettings.maxPanPosition = -1;
                    appSettings.maxTiltPosition = -1;
                    CalibrateSteppers();
                }

                if (!strcmp(command, "home"))
                    if (!strcmp(doc["params"]["param1"], "pantilt"))
                        GotoPositions(stepper_slider.currentPosition(), 0, 0);

                if (!strcmp(command, "MoveRelative"))
                {
                    if (!strcmp(doc["params"]["param1"], "slider"))
                    {
                        long newPosition = stepper_slider.currentPosition() + (long)doc["params"]["param2"];
                        if (newPosition < 0)
                        {
                            newPosition = 0;
                        }
                        if (newPosition > appSettings.maxSliderPosition)
                        {
                            newPosition = appSettings.maxSliderPosition;
                        }

                        GotoPositions(newPosition, stepper_pan.currentPosition(), stepper_tilt.currentPosition());
                    }
                    if (!strcmp(doc["params"]["param1"], "pan"))
                    {
                        long newPosition = stepper_pan.currentPosition() + (long)doc["params"]["param2"];
                        if (newPosition < -appSettings.maxPanPosition)
                        {
                            newPosition = -appSettings.maxPanPosition;
                        }
                        if (newPosition > appSettings.maxPanPosition)
                        {
                            newPosition = appSettings.maxPanPosition;
                        }

                        GotoPositions(stepper_slider.currentPosition(), newPosition, stepper_tilt.currentPosition());
                    }
                    if (!strcmp(doc["params"]["param1"], "tilt"))
                    {
                        long newPosition = stepper_tilt.currentPosition() + (long)doc["params"]["param2"];
                        if (newPosition < -appSettings.maxTiltPosition)
                        {
                            newPosition = -appSettings.maxTiltPosition;
                        }
                        if (newPosition > appSettings.maxTiltPosition)
                        {
                            newPosition = appSettings.maxTiltPosition;
                        }

                        GotoPositions(stepper_slider.currentPosition(), stepper_pan.currentPosition(), newPosition);
                    }
                }

                if (!strcmp(command, "stop"))
                {
                    if (!strcmp(doc["params"]["param1"], "slider"))
                        StopStepper(AXIS::AXIS_SLIDER);
                    if (!strcmp(doc["params"]["param1"], "pan"))
                        StopStepper(AXIS::AXIS_PAN);
                    if (!strcmp(doc["params"]["param1"], "tilt"))
                        StopStepper(AXIS::AXIS_TILT);
                }

                if (!strcmp(command, "GotoPosition"))
                    GotoPositions((float)doc["params"]["param1"], (float)doc["params"]["param2"], (float)doc["params"]["param3"]);

                if (!strcmp(command, "SaveSettings"))
                {
                    if (doc["params"]["param1"] == "timelapseSaveSettings")
                    {
                        appSettings.timelapseStartPositionX = (long)doc["params"]["param2"];
                        appSettings.timelapseStartPositionY = (long)doc["params"]["param3"];
                        appSettings.timelapseStartPositionZ = (long)doc["params"]["param4"];
                        appSettings.timelapseEndPositionX = (long)doc["params"]["param5"];
                        appSettings.timelapseEndPositionY = (long)doc["params"]["param6"];
                        appSettings.timelapseEndPositionZ = (long)doc["params"]["param7"];

                        appSettings.timelapseNumberOfShots = (uint16_t)doc["params"]["param8"];
                        appSettings.timelapseInterval = (uint16_t)doc["params"]["param9"];
                        appSettings.timelapseExposureLength = (uint8_t)doc["params"]["param10"];

                        appSettings.Save();
                    }
                    if (doc["params"]["param1"] == "panningvideoSaveSettings")
                    {

                        appSettings.panningvideoStartPositionX = (long)doc["params"]["param2"];
                        appSettings.panningvideoStartPositionY = (long)doc["params"]["param3"];
                        appSettings.panningvideoStartPositionZ = (long)doc["params"]["param4"];
                        appSettings.panningvideoEndPositionX = (long)doc["params"]["param5"];
                        appSettings.panningvideoEndPositionY = (long)doc["params"]["param6"];
                        appSettings.panningvideoEndPositionZ = (long)doc["params"]["param7"];

                        appSettings.panningvideoSpeed = (long)doc["params"]["param8"];

                        appSettings.Save();
                    }
                }

                if (!strcmp(command, "startprogram"))
                {
                    if (!isStepperSliderCalibrated)
                    {
                        // DisplayError(2);
                        return;
                    }
                    String pgmName = doc["params"]["param1"];
                    // SerialMon.println(pgmName);

                    if (pgmName == "Timelapse")
                    {
                        tli.currentShot = 0;

                        tli.StartPositionSlider = (long)doc["params"]["param2"];
                        tli.StartPositionPan = (long)doc["params"]["param3"];
                        tli.StartPositionTilt = (long)doc["params"]["param4"];
                        tli.EndPositionSlider = (long)doc["params"]["param5"];
                        tli.EndPositionPan = (long)doc["params"]["param6"];
                        tli.EndPositionTilt = (long)doc["params"]["param7"];

                        tli.numberOfShots = (uint16_t)doc["params"]["param8"];
                        tli.interval = (uint16_t)doc["params"]["param9"];
                        tli.exposureLength = (uint8_t)doc["params"]["param10"];

                        tli.distanceBetweenShotsSlider = 0;
                        tli.distanceBetweenShotsPan = 0;
                        tli.distanceBetweenShotsTilt = 0;

                        if (tli.numberOfShots > 1)
                        {
                            tli.distanceBetweenShotsSlider = (tli.EndPositionSlider - tli.StartPositionSlider) / (tli.numberOfShots - 1);
                            tli.distanceBetweenShotsPan = (tli.EndPositionPan - tli.StartPositionPan) / (tli.numberOfShots - 1);
                            tli.distanceBetweenShotsTilt = (tli.EndPositionTilt - tli.StartPositionTilt) / (tli.numberOfShots - 1);
                        }

                        opMode = OPERATIONMODE::TIMELAPSE;
                        tlStage = TIMELAPSE_STAGE::TIMELAPSE_START;
                    }

                    if (pgmName == "PanningVideo")
                    {
                        pvi.StartPositionSlider = (long)doc["params"]["param2"];
                        pvi.StartPositionPan = (long)doc["params"]["param3"];
                        pvi.StartPositionTilt = (long)doc["params"]["param4"];
                        pvi.EndPositionSlider = (long)doc["params"]["param5"];
                        pvi.EndPositionPan = (long)doc["params"]["param6"];
                        pvi.EndPositionTilt = (long)doc["params"]["param7"];

                        pvi.speed = (uint16_t)doc["params"]["param8"];

                        opMode = OPERATIONMODE::PANNING_VIDEO;
                        pvStage = PANNINGVIDEO_STAGE::PV_START;
                    }
                }
            }
        }
        break;

        case OPERATIONMODE::TIMELAPSE:
        {
            switch (tlStage)
            {
            case TIMELAPSE_STAGE::TIMELAPSE_START:
            {
                SetStepperSpeedPercent(100);
                SerialMon.println("Starting TIMELAPSE...");
                SerialMon.print("Moving into position... ");
                settargetPositions(tli.StartPositionSlider, tli.StartPositionPan, tli.StartPositionTilt);
                multiStepper.run();
                tlStage = TIMELAPSE_STAGE::TIMELAPSE_MOVE_IN_POSITION;
            }
            break;

            case TIMELAPSE_STAGE::TIMELAPSE_MOVE_IN_POSITION:
            {
                if (stepper_slider.distanceToGo() == 0)
                {
                    SerialMon.printf("Arrived in starting position %i : %i : %i for shot #%u.\r\nPausing for %i seconds... ", stepper_slider.currentPosition(), stepper_pan.currentPosition(), stepper_tilt.currentPosition(), tli.currentShot, tli.interval);
                    delayMillis = millis();
                    tlStage = TIMELAPSE_STAGE::TIMELAPSE_DELAY_BEFORE_SHOOT;
                }
            }
            break;

            case TIMELAPSE_STAGE::TIMELAPSE_DELAY_BEFORE_SHOOT:
            {
                if (millis() - delayMillis > tli.interval * 1000)
                {
                    SerialMon.printf("Done.\r\nMetering... ");
                    ActivateMeter();
                    meterMillis = millis();

                    tlStage = TIMELAPSE_STAGE::TIMELAPSE_METER;
                }
            }
            break;

            case TIMELAPSE_STAGE::TIMELAPSE_METER:
            {
                if (millis() - meterMillis > METER_DELAY)
                {
                    SerialMon.printf("Done.\r\nTaking shot #%u... ", tli.currentShot + 1);
                    ActivateExposure();
                    triggerMillis = millis();
                    tlStage = TIMELAPSE_SHOOT;
                }
            }
            break;

            case TIMELAPSE_STAGE::TIMELAPSE_SHOOT:
            {
                if (millis() - triggerMillis > appSettings.timelapseExposureLength * 1000)
                {
                    DeactivateExposure();
                    DeactivateMeter();

                    SerialMon.printf("Done.\r\n");

                    tli.currentShot++;
                    if (tli.currentShot < tli.numberOfShots)
                    {
                        SerialMon.print("Moving into position... ");
                        settargetPositions(tli.StartPositionSlider + tli.currentShot * tli.distanceBetweenShotsSlider, tli.StartPositionPan + tli.currentShot * tli.distanceBetweenShotsPan, tli.StartPositionTilt + tli.currentShot * tli.distanceBetweenShotsTilt);
                        multiStepper.run();
                        tlStage = TIMELAPSE_STAGE::TIMELAPSE_MOVE_IN_POSITION;
                    }
                    else
                    {
                        tlStage = TIMELAPSE_FINISHED;
                    }
                }
            }
            break;

            case TIMELAPSE_STAGE::TIMELAPSE_FINISHED:
            {
                SerialMon.println("TIMELAPSE finished.");
                opMode = OPERATIONMODE::NORMAL;
            }
            break;

            default:
                break;
            }
        }
        break;

        case OPERATIONMODE::PANNING_VIDEO:
        {
            switch (pvStage)
            {
            case PANNINGVIDEO_STAGE::PV_START:
            {
                SetStepperSpeedPercent(100);

                SerialMon.println("Starting PANNING VIDEO...");
                SerialMon.print("Moving into position... ");

                settargetPositions(pvi.StartPositionSlider, pvi.StartPositionPan, pvi.StartPositionTilt);
                multiStepper.run();
                pvStage = PANNINGVIDEO_STAGE::PV_MOVE_IN_POSITION;
            }
            break;

            case PANNINGVIDEO_STAGE::PV_MOVE_IN_POSITION:
            {
                while (multiStepper.run())
                {
                }

                SerialMon.printf(" Arrived in starting position %i : %i : %i.\r\n", stepper_slider.currentPosition(), stepper_pan.currentPosition(), stepper_tilt.currentPosition());
                ActivateMeter();
                meterMillis = millis();
                pvStage = PANNINGVIDEO_STAGE::PV_METER;
            }
            break;

            case PANNINGVIDEO_STAGE::PV_METER:
            {
                if (millis() - meterMillis > METER_DELAY)
                {
                    SerialMon.println("Metering done, filming sequence...");

                    SetStepperSpeedPercent(pvi.speed);
                    settargetPositions(pvi.EndPositionSlider, pvi.EndPositionPan, pvi.EndPositionTilt);
                    ActivateExposure();
                    pvStage = PANNINGVIDEO_STAGE::PV_FINISHING;
                }
            }
            break;

            case PANNINGVIDEO_STAGE::PV_FINISHING:
            {
                while (multiStepper.run())
                {
                }
                DeactivateExposure();
                DeactivateMeter();
                SerialMon.println("PANNING VIDEO finished.");
                opMode = OPERATIONMODE::NORMAL;
            }
            break;

            default:
                break;
            }
        }
        break;

        default:
            break;
        }
        multiStepper.run();
    }
    else
    {
        //  Battery is dead
        enableSteppers(false);
    }
}