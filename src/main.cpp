#include <WiFi.h>

#include "main.h"
#include "version.h"
#include "common.h"
#include "asyncwebserver.h"
#include "filesystem.h"
// #include "i2c.h"
#include "stepper.h"
#include "cameracontrol.h"
#include "battery.h"
#include "ledstrip.h"

enum CONNECTION_STATE
{
    STATE_CHECK_WIFI_CONNECTION,
    STATE_WIFI_CONNECT,
    STATE_CHECK_INTERNET_CONNECTION,
    STATE_INTERNET_CONNECTED
} connectionState;

settings appSettings;
bool isAccessPoint = false;
bool isAccessPointCreated = false;
bool isInternetConnected = false;
bool isFirstRun = true;

const char *connectionServer = "diy.viktak.com";

boolean checkInternetConnection()
{
    return true;
    IPAddress timeServerIP;
    int result = WiFi.hostByName(connectionServer, timeServerIP);
    return (result == 1);
}

void setup()
{
    SerialMon.begin(DEBUG_SPEED);

    SerialMon.printf("\r\n\n\nBooting ESP node %u...\r\n", GetChipID());
    SerialMon.println("Hardware ID:      " + HARDWARE_ID);
    SerialMon.println("Hardware version: " + HARDWARE_VERSION);
    SerialMon.println("Software ID:      " + FIRMWARE_ID);
    SerialMon.println("Software version: " + String(FIRMWARE_VERSION));
    SerialMon.println();

    appSettings.Load(false);

    setupLEDs();
    setupFileSystem();
    setupStepperMotors();
    setupBattery();
    setupCameraControl();

    //  Finished setup()
    SerialMon.println("Setup finished successfully.");

    // while (true)
    // {
    //     SerialMon.println(digitalRead(PAN_HALL_GPIO));
    // }
}

void loop()
{

    if (isAccessPoint)
    {
        if (!isAccessPointCreated)
        {
            SerialMon.printf("Reverting to Access Point mode.\r\n", appSettings.wifiSSID);

            delay(500);

            WiFi.mode(WIFI_AP);
            WiFi.softAP(appSettings.hostName, appSettings.accessPointPassword);

            IPAddress myIP = WiFi.softAPIP();
            isAccessPointCreated = true;

            InitAsyncWebServer();

            SerialMon.println("Access point created. Use the following information to connect to the ESP device, then follow the on-screen instructions.");

            SerialMon.print("SSID:\t\t\t");
            SerialMon.println(appSettings.hostName);

            SerialMon.print("Password:\t\t");
            SerialMon.println(appSettings.accessPointPassword);

            SerialMon.print("Access point address:\t");
            SerialMon.println(myIP);

            if (isFirstRun)
            {
                CalibrateSteppers();
                isFirstRun = false;
            }
        }
    }
    else
    {
        switch (connectionState)
        {

        // Check the WiFi connection
        case STATE_CHECK_WIFI_CONNECTION:
        {
            // Are we connected ?
            if (WiFi.status() != WL_CONNECTED)
            {
                // Wifi is NOT connected
                connectionState = STATE_WIFI_CONNECT;
            }
            else
            {
                // Wifi is connected so check Internet
                connectionState = STATE_CHECK_INTERNET_CONNECTION;
            }
            break;
        }

        // No Wifi so attempt WiFi connection
        case STATE_WIFI_CONNECT:
        {
            isInternetConnected = false;
            SerialMon.printf("Trying to connect to WIFI network: %s", appSettings.wifiSSID);

            // Start connection process
            WiFi.setHostname(appSettings.hostName);
            WiFi.mode(WIFI_STA);

            WiFi.begin(appSettings.wifiSSID, appSettings.wifiPassword);
            WiFi.setSleep(false);

            // Initialize iteration counter
            uint8_t attempt = 0;

            while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT))
            {
                SerialMon.print(".");
                delay(50);
                delay(950);
            }
            if (attempt >= WIFI_CONNECTION_TIMEOUT)
            {
                SerialMon.println();
                SerialMon.println("Could not connect to WiFi.");
                delay(100);

                isAccessPoint = true;

                break;
            }

            SerialMon.println(" Success!");

            IPAddress ip = WiFi.localIP();

            SerialMon.printf("WiFi channel:\t%u\r\n", WiFi.channel());
            SerialMon.printf("IP address:\t%u.%u.%u.%u\r\n", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, ip >> 24);
            connectionState = STATE_CHECK_INTERNET_CONNECTION;
            break;
        }

        case STATE_CHECK_INTERNET_CONNECTION:
        {
            // Do we have a connection to the Internet ?
            if (checkInternetConnection())
            {
                if (!isInternetConnected)
                {
                    // We have an Internet connection
                    InitAsyncWebServer();
                }
                isInternetConnected = true;
                connectionState = STATE_INTERNET_CONNECTED;
            }
            else
            {
                connectionState = STATE_CHECK_WIFI_CONNECTION;
            }
            break;
        }

        case STATE_INTERNET_CONNECTED:
        {
            // Set next connection state
            connectionState = STATE_CHECK_WIFI_CONNECTION;

            if (isFirstRun)
            {
                CalibrateSteppers();
                isFirstRun = false;
            }

            break;
        }
        }
    }

    // loopI2C();
    loopStepper();
    loopCameraControl();
    loopAsyncWebserver();
    loopBattery();
    loopLEDs();
}
