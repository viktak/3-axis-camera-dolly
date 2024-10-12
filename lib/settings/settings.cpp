#include "settings.h"

#include <WiFi.h>
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

////////////////////////////////////////////////////////////////////
/// Deafult values
////////////////////////////////////////////////////////////////////

//  System
#define DEFAULT_ADMIN_USERNAME "admin"
#define DEFAULT_ADMIN_PASSWORD "admin"
#define DEFAULT_AP_PASSWORD "esp12345678"

#define DEFAULT_NODE_FRIENDLY_NAME "vNode"

//  WiFi
#define DEFAULT_SSID "ESP"
#define DEFAULT_PASSWORD "ESP"

Preferences prefs;

//  System
char adminPassword[32];

char wifiSSID[32];
char wifiPassword[32];

char AccessPointSSID[32];
char AccessPointPassword[32];

char friendlyName[32];

//  Timelapse
uint16_t timelapseNumberOfShots;
uint16_t timelapseInterval;
uint8_t timelapseExposureLength;
long timelapseStartPositionX;
long timelapseEndPositionX;

//  Panning video
long panningvideoStartPositionX;
long panningvideoEndPositionX;
uint16_t panningvideoSpeed;
bool panningvideoStartRecordingBeforeMoving;
bool panningvideoStopRecordingBeforeStopping;

//  Calculated settings
char hostName[RFC952_HOSTNAME_MAXLEN];

//  Measured settings
long maxSliderPosition;

void settings::SetupFileSystem()
{
    if (!LittleFS.begin(false))
    {
        SerialMon.println("Error: Failed to initialize the internal filesystem!");
    }
    if (!LittleFS.begin(true))
    {
        SerialMon.println("Error: Failed to format the internal filesystem!");
        delay(3000);
        ESP.restart();
    }
    SerialMon.println("Internal filesystem initialized.");
}

void settings::ClearNVS()
{
    nvs_flash_erase(); // erase the NVS partition and...
    nvs_flash_init();  // initialize the NVS partition.
}

String settings::GetDeviceMAC()
{
    String s = WiFi.macAddress();

    for (size_t i = 0; i < 5; i++)
        s.remove(14 - i * 3, 1);

    return s;
}

void settings::PrintSettings()
{
    SerialMon.println("====================================== App settings ======================================");
    SerialMon.printf("Settings\t\t%s\r\nApp name\t\t%s\r\nAdmin password\t\t%s\r\nAP SSID\t\t\t%s\r\nAP Password\t\t%s\r\n"
                     "SSID\t\t\t%s\r\nHostname:\t\t%s\r\n",
                     (String)SETTINGS_NAME,
                     friendlyName, adminPassword, AccessPointSSID, AccessPointPassword, wifiSSID, hostName);
    SerialMon.printf("Slider length [steps]:\t%i\r\nPan length [steps]:\t%i\r\nTilt length [steps]:\t%i\r\n", maxSliderPosition, maxPanPosition, maxTiltPosition);
    SerialMon.printf("TL shots:\t\t%u\r\nTL interval:\t\t%u\r\nTL exp length:\t\t%u\r\nTL start X:\t\t%i\r\nTL end X:\t\t%i\r\n"
                     "TL start Y:\t\t%i\r\nTL end Y:\t\t%i\r\nTL start Z:\t\t%i\r\nTL end Z:\t\t%i\r\n",
                     timelapseNumberOfShots, timelapseInterval, timelapseExposureLength, timelapseStartPositionX, timelapseEndPositionX,
                     timelapseStartPositionY, timelapseEndPositionY, timelapseStartPositionZ, timelapseEndPositionZ);
    SerialMon.printf("PV start X:\t\t%i\r\nPV end X:\t\t%i\r\nPV start Y:\t\t%i\r\nPV end Y:\t\t%i\r\nPV start Z:\t\t%i\r\nPV end Z:\t\t%i\r\n"
                     "PV speed:\t\t%u\r\nPV start before:\t%s\r\nPV stop before:\t\t%s\r\n",
                     panningvideoStartPositionX, panningvideoEndPositionX, panningvideoStartPositionY, panningvideoEndPositionY, panningvideoStartPositionZ,
                     panningvideoEndPositionZ, panningvideoSpeed, panningvideoStartRecordingBeforeMoving ? "true" : "false", panningvideoStopRecordingBeforeStopping ? "true" : "false");
    SerialMon.println("==========================================================================================");
}

void settings::Save()
{
    prefs.begin(SETTINGS_NAME, false);

    //  System
    prefs.putString("APP_NAME", friendlyName);

    prefs.putString("ADMIN_PASSWORD", adminPassword);

    prefs.putString("AP_SSID", AccessPointSSID);
    prefs.putString("AP_PASSWORD", AccessPointPassword);

    prefs.putString("SSID", wifiSSID);
    prefs.putString("PASSWORD", wifiPassword);

    prefs.putString("PASSWORD", wifiPassword);

    prefs.putLong("SLIDER_MAXSTEPS", maxSliderPosition);
    prefs.putLong("PAN_MAXSTEPS", maxPanPosition);
    prefs.putLong("TILT_MAXSTEPS", maxTiltPosition);

    //  Timelapse
    prefs.putUInt("TL_SHOTS", timelapseNumberOfShots);
    prefs.putUInt("TL_INTERVAL", timelapseInterval);
    prefs.putUChar("TL_EXPLENGTH", timelapseExposureLength);

    prefs.putLong("TL_STARTPOSX", timelapseStartPositionX);
    prefs.putLong("TL_ENDPOSX", timelapseEndPositionX);
    prefs.putLong("TL_STARTPOSY", timelapseStartPositionY);
    prefs.putLong("TL_ENDPOSY", timelapseEndPositionY);
    prefs.putLong("TL_STARTPOSZ", timelapseStartPositionZ);
    prefs.putLong("TL_ENDPOSZ", timelapseEndPositionZ);

    //  Panning video
    prefs.putLong("PV_STARTPOSX", panningvideoStartPositionX);
    prefs.putLong("PV_ENDPOSX", panningvideoEndPositionX);
    prefs.putLong("PV_STARTPOSY", panningvideoStartPositionY);
    prefs.putLong("PV_ENDPOSY", panningvideoEndPositionY);
    prefs.putLong("PV_STARTPOSZ", panningvideoStartPositionZ);
    prefs.putLong("PV_ENDPOSZ", panningvideoEndPositionZ);
    prefs.putUInt("PV_SPEED", panningvideoSpeed);
    prefs.putBool("PV_STARTBMOV", panningvideoStartRecordingBeforeMoving);
    prefs.putBool("PV_STOPBSTOP", panningvideoStopRecordingBeforeStopping);

    prefs.end();
    delay(100);

#ifdef __debugSettings
    SerialMon.println("Settings saved.");
#endif
}

void settings::Load(bool LoadDefaults)
{
    SetupFileSystem();

    if (LoadDefaults)
        ClearNVS();

    prefs.begin(SETTINGS_NAME, false);

    if (LoadDefaults)
        prefs.clear(); //  clear all settings

    String s = (String)DEFAULT_APP_FRIENDLY_NAME;
    s.trim();
    s.toLowerCase();
    s.replace(' ', '-');

    String DEFAULT_AP_SSID = s + "-" + GetDeviceMAC().substring(6);

    //  System
    strcpy(friendlyName, (prefs.getString("APP_NAME", DEFAULT_APP_FRIENDLY_NAME).c_str()));

    strcpy(adminPassword, (prefs.getString("ADMIN_PASSWORD", DEFAULT_ADMIN_PASSWORD).c_str()));

    strcpy(AccessPointSSID, (prefs.getString("AP_SSID", DEFAULT_AP_SSID).c_str()));
    strcpy(AccessPointPassword, (prefs.getString("AP_PASSWORD", DEFAULT_AP_PASSWORD).c_str()));

    strcpy(wifiSSID, (prefs.getString("SSID", DEFAULT_SSID).c_str()));
    strcpy(wifiPassword, (prefs.getString("PASSWORD", DEFAULT_PASSWORD).c_str()));

    maxSliderPosition = prefs.getLong("SLIDER_MAXSTEPS", -1);
    maxPanPosition = prefs.getLong("PAN_MAXSTEPS", -1);
    maxTiltPosition = prefs.getLong("TILT_MAXSTEPS", -1);

    //  Timelapse
    timelapseNumberOfShots = prefs.getUInt("TL_SHOTS", 3);
    timelapseInterval = prefs.getUInt("TL_INTERVAL", 5);
    timelapseExposureLength = prefs.getUInt("TL_EXPLENGTH", 1);
    timelapseStartPositionX = prefs.getLong("TL_STARTPOSX", 100);
    timelapseEndPositionX = prefs.getLong("TL_ENDPOSX", 500);
    timelapseStartPositionY = prefs.getLong("TL_STARTPOSY", 100);
    timelapseEndPositionY = prefs.getLong("TL_ENDPOSY", 500);
    timelapseStartPositionZ = prefs.getLong("TL_STARTPOSZ", 100);
    timelapseEndPositionZ = prefs.getLong("TL_ENDPOSZ", 500);

    //  Panning video
    panningvideoStartPositionX = prefs.getLong("PV_STARTPOSX", 0);
    panningvideoEndPositionX = prefs.getLong("PV_ENDPOSX", 0);
    panningvideoStartPositionY = prefs.getLong("PV_STARTPOSY", 100);
    panningvideoEndPositionY = prefs.getLong("PV_ENDPOSY", 500);
    panningvideoStartPositionZ = prefs.getLong("PV_STARTPOSZ", 100);
    panningvideoEndPositionZ = prefs.getLong("PV_ENDPOSZ", 500);

    panningvideoSpeed = prefs.getUInt("PV_SPEED", 50);
    panningvideoStartRecordingBeforeMoving = prefs.getBool("PV_STARTBMOV", false);
    panningvideoStopRecordingBeforeStopping = prefs.getBool("PV_STOPBSTOP", false);

    prefs.end();

    s = friendlyName;
    s.replace(' ', '-');
    s.toLowerCase();

    sprintf(hostName, "%s-%s", s.c_str(), GetDeviceMAC().substring(6).c_str());

#ifdef __debugSettings
    PrintSettings();
#endif
}

settings::settings()
{
}
