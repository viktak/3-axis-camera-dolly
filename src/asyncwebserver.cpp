#include <Arduino.h>
#include <Update.h>

#include <DNSServer.h>

#include <ArduinoJson.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <AceSorting.h>

#include "asyncwebserver.h"
#include "stepper.h"
#include "common.h"
#include "version.h"
#include "settings.h"
#include "main.h"

#define OTA_PROGRESS_UPDATE_INTERVAL 500

const char *ADMIN_USERNAME = "admin";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

size_t updateSize = 0;
size_t lastCurrent = 0;
bool shouldReboot = false;
unsigned long ota_progress_millis = 0;
unsigned long otaStartMillis = 0;
size_t otaSize = 0;

unsigned long clearWSmillis = 0;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        // SerialMon.printf("ws[%s][%u] connect\r\n", server->url(), client->id());
        // client->printf("Client %%u connected.)", client->id());
        // client->printf("Hello Client %u :)", client->id());
        // client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        // SerialMon.printf("ws[%s][%u] disconnect\r\n", server->url(), client->id());
    }
    else if (type == WS_EVT_ERROR)
    {
        SerialMon.printf("ws[%s][%u] error(%u): %s\r\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
    else if (type == WS_EVT_PONG)
    {
        // SerialMon.printf("ws[%s][%u] pong[%u]: %s\r\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        String msg = "";
        if (info->final && info->index == 0 && info->len == len)
        {
            // the whole message is in a single frame and we got all of it's data
            // SerialMon.printf("1ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    msg += (char)data[i];
                }

                sci.data = (char *)data;
                sci.length = len;

                if (sci.length > 0)
                {
                    JsonDocument doc;

                    DeserializationError error = deserializeJson(doc, sci.data);
                    if (error)
                    {
                        Serial.print("deserializeJson() failed: ");
                        Serial.println(error.c_str());
                        return;
                    }

                    serializeJsonPretty(doc, SerialMon);
                    SerialMon.println();

                    const char *command = doc["command"];
                    if (!strcmp(command, "GetCurrent"))
                    {
                        client->text((String)stepper_slider.currentPosition() + ":" + (String)stepper_pan.currentPosition() + ":" + (String)stepper_tilt.currentPosition());
                    }
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < info->len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }

            // if (info->opcode == WS_TEXT)
            //     client->text("I got your text message");
            // else
            //     client->binary("I got your binary message");
        }
        else
        {
            // message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0)
            {
                if (info->num == 0)
                    SerialMon.printf("2ws[%s][%u] %s-message start\r\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                SerialMon.printf("3ws[%s][%u] frame[%u] start[%llu]\r\n", server->url(), client->id(), info->num, info->len);
            }

            SerialMon.printf("4ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }
            SerialMon.printf("%s\r\n", msg.c_str());

            if ((info->index + len) == info->len)
            {
                SerialMon.printf("5ws[%s][%u] frame[%u] end[%llu]\r\n", server->url(), client->id(), info->num, info->len);
                if (info->final)
                {
                    SerialMon.printf("6ws[%s][%u] %s-message end\r\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    // if (info->message_opcode == WS_TEXT)
                    //     client->text("I got your text message");
                    // else
                    //     client->binary("I got your binary message");
                }
            }
        }
    }
}

//  Helper functions - not used in production
void PrintParameters(AsyncWebServerRequest *request)
{
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isFile())
        { // p->isPost() is also true
            SerialMon.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        }
        else if (p->isPost())
        {
            SerialMon.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
        else
        {
            SerialMon.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
    }
}

void PrintHeaders(AsyncWebServerRequest *request)
{
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++)
    {
        AsyncWebHeader *h = request->getHeader(i);
        SerialMon.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
}

// Root/Status
String StatusTemplateProcessor(const String &var)
{
    //  System information
    if (var == "chipid")
        return (String)GetChipID();
    if (var == "hardwareid")
        return HARDWARE_ID;
    if (var == "hardwareversion")
        return HARDWARE_VERSION;
    if (var == "firmwareid")
        return FIRMWARE_ID;
    if (var == "firmwareversion")
        return FIRMWARE_VERSION;
    if (var == "uptime")
        return TimeIntervalToString(millis() / 1000);
    if (var == "lastresetreason0")
        return GetResetReasonString(rtc_get_reset_reason(0));
    if (var == "lastresetreason1")
        return GetResetReasonString(rtc_get_reset_reason(1));
    if (var == "flashchipsize")
        return String(ESP.getFlashChipSize());
    if (var == "flashchipspeed")
        return String(ESP.getFlashChipSpeed());
    if (var == "freeheapsize")
        return String(ESP.getFreeHeap());
    if (var == "freesketchspace")
        return String(ESP.getFreeSketchSpace());

    //  Network

    switch (WiFi.getMode())
    {
    case WIFI_AP:
        if (var == "wifimode")
            return "Access Point";
        if (var == "ssid")
            return "-";
        if (var == "channel")
            return String(WiFi.channel());
        if (var == "hostname")
            return appSettings.hostName;
        if (var == "macaddress")
            return "-";
        if (var == "ipaddress")
            return WiFi.softAPIP().toString();
        if (var == "subnetmask")
            return WiFi.softAPSubnetMask().toString();
        if (var == "apssid")
            return appSettings.hostName;
        if (var == "apmacaddress")
            return String(WiFi.softAPmacAddress());
        if (var == "gateway")
            return "-";
        break;

    case WIFI_STA:
        if (var == "wifimode")
            return "Station";
        if (var == "ssid")
            return String(WiFi.SSID());
        if (var == "channel")
            return String(WiFi.channel());
        if (var == "hostname")
            return appSettings.hostName;
        if (var == "macaddress")
            return String(WiFi.macAddress());
        if (var == "ipaddress")
            return WiFi.localIP().toString();
        if (var == "subnetmask")
            return WiFi.subnetMask().toString();
        if (var == "apssid")
            return "Not active!";
        if (var == "apmacaddress")
            return "Not active!";
        if (var == "gateway")
            return WiFi.gatewayIP().toString();

        break;
    default:
        //  This should not happen...
        break;
    }

    return String();
}

//  General
String GeneralSettingsTemplateProcessor(const String &var)
{
    if (var == "friendlyname")
        return appSettings.friendlyName;

    return String();
}

void POSTGeneralSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("txtFriendlyName", true))
    {
        p = request->getParam("txtFriendlyName", true);
        sprintf(appSettings.friendlyName, "%s", p->value().c_str());
    }

    appSettings.Save();
}

//  Slider-Panning-Video
String SliderPanningVideoTemplateProcessor(const String &var)
{
    if (var == "ipaddress")
    {
        switch (WiFi.getMode())
        {
        case WIFI_STA:
            return WiFi.localIP().toString();
            break;
        case WIFI_AP:
            return WiFi.softAPIP().toString();
            break;
        default:
            break;
        }
    }

    //  Alert
    if (var == "danger-hidden")
    {
        switch (WiFi.getMode())
        {
        case WIFI_STA:
            return "hidden";
            break;
        case WIFI_AP:
            return WiFi.softAPIP().toString().length() > 0 ? "hidden" : "";
            break;
        default:
            break;
        }
    }

    //  Manual jog
    if (var == "maxSlideManualSpeed")
        return (String)STEPPER_SLIDER_MAX_SPEED;

    if (var == "panningvideo-maxX")
        return (String)(appSettings.maxSliderPosition);
    if (var == "panningvideo-maxY")
        return (String)(appSettings.maxPanPosition);
    if (var == "panningvideo-maxZ")
        return (String)(appSettings.maxTiltPosition);

    if (var == "StartPositionX")
        return (String)appSettings.panningvideoStartPositionX;
    if (var == "StartPositionY")
        return (String)appSettings.panningvideoStartPositionY;
    if (var == "StartPositionZ")
        return (String)appSettings.panningvideoStartPositionZ;

    if (var == "EndPositionX")
        return (String)appSettings.panningvideoEndPositionX;
    if (var == "EndPositionY")
        return (String)appSettings.panningvideoEndPositionY;
    if (var == "EndPositionZ")
        return (String)appSettings.panningvideoEndPositionZ;

    if (var == "Speed")
        return (String)appSettings.panningvideoSpeed;

    return "n/a";
}

void POSTSliderPanningVideo(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("txtFriendlyName", true))
    {
        p = request->getParam("txtFriendlyName", true);
        sprintf(appSettings.friendlyName, "%s", p->value().c_str());
    }

    appSettings.Save();
}

//  Slider-Timelapse
String SliderTimelapseTemplateProcessor(const String &var)
{
    if (var == "ipaddress")
    {
        switch (WiFi.getMode())
        {
        case WIFI_STA:
            return WiFi.localIP().toString();
            break;
        case WIFI_AP:
            return WiFi.softAPIP().toString();
            break;
        default:
            break;
        }
    }

    //  Alert
    if (var == "danger-hidden")
    {
        switch (WiFi.getMode())
        {
        case WIFI_STA:
            return "hidden";
            break;
        case WIFI_AP:
            return WiFi.softAPIP().toString().length() > 0 ? "hidden" : "";
            break;
        default:
            break;
        }
    }

    //  Manual jog
    if (var == "maxSlideManualSpeed")
        return (String)STEPPER_SLIDER_MAX_SPEED;

    //  Timelapse
    if (var == "numNumberofShots")
        return (String)appSettings.timelapseNumberOfShots;
    if (var == "numInterval")
        return (String)appSettings.timelapseInterval;
    if (var == "numExposureLength")
        return (String)appSettings.timelapseExposureLength;

    if (var == "timelapse-maxX")
        return (String)(appSettings.maxSliderPosition);
    if (var == "timelapse-maxY")
        return (String)(appSettings.maxPanPosition);
    if (var == "timelapse-maxZ")
        return (String)(appSettings.maxTiltPosition);

    if (var == "StartPositionX")
        return (String)appSettings.timelapseStartPositionX;
    if (var == "StartPositionY")
        return (String)appSettings.timelapseStartPositionY;
    if (var == "StartPositionZ")
        return (String)appSettings.timelapseStartPositionZ;

    if (var == "EndPositionX")
        return (String)appSettings.timelapseEndPositionX;
    if (var == "EndPositionY")
        return (String)appSettings.timelapseEndPositionY;
    if (var == "EndPositionZ")
        return (String)appSettings.timelapseEndPositionZ;

    return "n/a";
}

void POSTSliderTimelapse(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("txtFriendlyName", true))
    {
        p = request->getParam("txtFriendlyName", true);
        sprintf(appSettings.friendlyName, "%s", p->value().c_str());
    }

    appSettings.Save();
}

//  Slider-Timelapse
String SliderMaintenanceProcessor(const String &var)
{
    if (var == "ipaddress")
    {
        switch (WiFi.getMode())
        {
        case WIFI_STA:
            return WiFi.localIP().toString();
            break;
        case WIFI_AP:
            return WiFi.softAPIP().toString();
            break;
        default:
            break;
        }
    }

    //  Alert
    if (var == "danger-hidden")
    {
        switch (WiFi.getMode())
        {
        case WIFI_STA:
            return "hidden";
            break;
        case WIFI_AP:
            return WiFi.softAPIP().toString().length() > 0 ? "hidden" : "";
            break;
        default:
            break;
        }
    }




    return "n/a";
}

void POSTSliderMaintenance(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    // if (request->hasParam("txtFriendlyName", true))
    // {
    //     p = request->getParam("txtFriendlyName", true);
    //     sprintf(appSettings.friendlyName, "%s", p->value().c_str());
    // }

    appSettings.Save();
}

//  Network
String NetworkSettingsTemplateProcessor(const String &var)
{
    if (var == "networklist")
    {
        int numberOfNetworks = WiFi.scanComplete();
        if (numberOfNetworks == -2)
        {
            WiFi.scanNetworks(true);
        }
        else if (numberOfNetworks)
        {
            String aSSIDs[numberOfNetworks];
            for (int i = 0; i < numberOfNetworks; ++i)
            {
                aSSIDs[i] = WiFi.SSID(i);
            }

            ace_sorting::insertionSort(aSSIDs, numberOfNetworks);

            String wifiList;
            for (size_t i = 0; i < numberOfNetworks; i++)
            {
                wifiList += "<option value='";
                wifiList += aSSIDs[i];
                wifiList += "'";

                if (!strcmp(aSSIDs[i].c_str(), (appSettings.wifiSSID)))
                {
                    wifiList += " selected ";
                }

                wifiList += ">";
                wifiList += aSSIDs[i] + " (" + (String)WiFi.RSSI(i) + ")";
                wifiList += "</option>";
            }
            WiFi.scanDelete();
            if (WiFi.scanComplete() == -2)
            {
                WiFi.scanNetworks(true);
            }
            return wifiList;
        }
    }

    if (var == "password")
        return "";

    return String();
}

void POSTNetworkSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("lstNetworks", true))
    {
        p = request->getParam("lstNetworks", true);
        sprintf(appSettings.wifiSSID, "%s", p->value().c_str());
    }

    if (request->hasParam("txtPassword", true))
    {
        p = request->getParam("txtPassword", true);
        sprintf(appSettings.wifiPassword, "%s", p->value().c_str());
    }

    appSettings.Save();
}

//  Tools
String ToolsTemplateProcessor(const String &var)
{
    return String();
}

//  OTA
void onOTAStart()
{
    SerialMon.println("OTA update started!");
    otaStartMillis = millis();
}

void onOTAProgress(size_t current, size_t final)
{
    if (millis() - ota_progress_millis > (unsigned long)OTA_PROGRESS_UPDATE_INTERVAL)
    {
        SerialMon.printf("OTA: %3.0f%%\tTransfered %7u of %7u bytes\tSpeed: %4.3fkB/s\r\n", ((float)current / (float) final) * 100, current, final, ((float)(current - lastCurrent)) / 1024);
        lastCurrent = current;
        ota_progress_millis = millis();
        otaSize = final;
    }
}

void onOTAEnd(bool success)
{
    float totalTime = (float)(millis() - otaStartMillis) / 1000;
    SerialMon.printf("OTA: %u bytes uploaded in %4.1f seconds. Speed: %4.3fkB/s                \r\n", otaSize, totalTime, (float)otaSize / totalTime / 1024);
    if (success)
    {
        SerialMon.println("OTA update finished successfully! Restarting...");
    }
    else
    {
        SerialMon.print("OTA failed: ");
        SerialMon.println("There was an error during OTA update.");
    }
}

/// Init
void InitAsyncWebServer()
{
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    //  Bootstrap
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/bootstrap.bundle.min.js", "text/javascript"); });

    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/bootstrap.min.css", "text/css"); });

    //  Images
    server.on("/favico.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favico.ico", "image/x-icon"); });

    server.on("/favico.icon", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favico.ico", "image/x-icon"); });

    server.on("/menu.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/menu.png", "image/png"); });

    //  Manual controls
    server.on("/rotate-clockwise.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/rotate-clockwise.png", "image/png"); });

    server.on("/rotate-counterclockwise.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/rotate-counterclockwise.png", "image/png"); });

    server.on("/arrow-right.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/arrow-right.png", "image/png"); });

    server.on("/arrow-left.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/arrow-left.png", "image/png"); });

    server.on("/rotate-backward.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/rotate-backward.png", "image/png"); });

    server.on("/rotate-forward.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/rotate-forward.png", "image/png"); });

    server.on("/home.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/home.png", "image/png"); });

    server.on("/stop.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/stop.png", "image/png"); });

    //  Logout
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  request->requestAuthentication();
                  request->redirect("/"); });

    //  Status
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/status.html", String(), false, StatusTemplateProcessor); });

    server.on("/status.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/status.html", String(), false, StatusTemplateProcessor); });

    //  General
    server.on("/general-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/general-settings.html", String(), false, GeneralSettingsTemplateProcessor); });

    server.on("/general-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  PrintParameters(request);
                  POSTGeneralSettings(request);

                  ESP.restart();

                  request->redirect("/general-settings.html"); });

    //  Slider-Panning-Video
    server.on("/slider-panning-video.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/slider-panning-video.html", String(), false, SliderPanningVideoTemplateProcessor); });

    //  Slider-Timelapse
    server.on("/slider-timelapse.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/slider-timelapse.html", String(), false, SliderTimelapseTemplateProcessor); });

    //  Slider-Maintenance
    server.on("/slider-mainenance.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/slider-mainenance.html", String(), false, SliderTimelapseTemplateProcessor); });

    //  Network
    server.on("/network-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                
                request->send(LittleFS, "/network-settings.html", String(), false, NetworkSettingsTemplateProcessor); });

    server.on("/network-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  PrintParameters(request);
                  POSTNetworkSettings(request);

                  ESP.restart();

                  request->redirect("/network-settings.html"); });

    //  Tools
    server.on("/tools.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                        return request->requestAuthentication();
                    request->send(LittleFS, "/tools.html", String(), false, ToolsTemplateProcessor); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                        return request->requestAuthentication();
                    ESP.restart(); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                        return request->requestAuthentication();
                    appSettings.Load(true);
                    ESP.restart(); });

    //  OTA
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server, ADMIN_USERNAME, appSettings.adminPassword);
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();

    SerialMon.println("AsyncWebServer started.");
}

void loopAsyncWebserver()
{
    ElegantOTA.loop();
    if (millis() - clearWSmillis > 1000)
    {
        ws.cleanupClients();
        clearWSmillis = millis();
    }
}