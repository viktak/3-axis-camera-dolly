#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

#define SerialMon Serial

#define RFC952_HOSTNAME_MAXLEN 24

#define ESP_ACCESS_POINT_NAME_SIZE 63

const int32_t DEBUG_SPEED = 115200;
const String HARDWARE_ID = "ESP32-S3-Mini-1 BB";
const String HARDWARE_VERSION = "1.0";
const String FIRMWARE_ID = "Camera Slider";

class settings
{
public:
    ///  Saved values
    char wifiSSID[32];
    char wifiPassword[32];

    char adminPassword[32];

    char accessPointPassword[32];

    char friendlyName[32];

    //  timelapse
    uint16_t timelapseNumberOfShots;
    uint16_t timelapseInterval;
    uint8_t timelapseExposureLength;

    long timelapseStartPositionX;
    long timelapseStartPositionY;
    long timelapseStartPositionZ;

    long timelapseEndPositionX;
    long timelapseEndPositionY;
    long timelapseEndPositionZ;

    //  panning video
    long panningvideoStartPositionX;
    long panningvideoStartPositionY;
    long panningvideoStartPositionZ;

    long panningvideoEndPositionX;
    long panningvideoEndPositionY;
    long panningvideoEndPositionZ;

    uint16_t panningvideoSpeed;
    bool panningvideoStartRecordingBeforeMoving;
    bool panningvideoStopRecordingBeforeStopping;

    // Calculated values
    char hostName[RFC952_HOSTNAME_MAXLEN];

    //  Measured values
    long maxSliderPosition;
    long maxPanPosition;
    long maxTiltPosition;

    //  functions
    void SetupFileSystem();
    void Load(bool LoadDefaults = false);
    void Save();

    void ClearNVS();

    String GetDeviceMAC();
    void PrintSettings();

    settings();
};

#endif