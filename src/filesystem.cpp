#include <LittleFS.h>

#include "common.h"

#define FORMAT_LITTLEFS_IF_FAILED true

void setupFileSystem()
{
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    {
        Serial.println("Error: Failed to initialize the internal filesystem!");
        delay(5);
        ESP.restart();
    }
    SerialMon.println("Internal filesystem initialized.");
}
