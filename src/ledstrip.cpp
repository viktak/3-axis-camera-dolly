#include <NeoPixelBus.h>

#include "ledstrip.h"
#include "common.h"
#include "battery.h"

#define colorSaturation 128

unsigned long batteryCriticalMillis = 0;

NeoGamma<NeoGammaTableMethod> colorGamma;

RgbwColor batteryLowColor(255, 255, 0, 0);
RgbwColor batteryCriticalColor(255, 0, 0, 0);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

//  WS2812
// NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PIXELCOUNT, NEOPIXEL_GPIO);

//  SK6812
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(PIXELCOUNT, NEOPIXEL_GPIO);

void setupLEDs()
{
    strip.Begin();
    for (size_t i = 0; i < PIXELCOUNT; i++)
    {
        strip.SetPixelColor(i, RgbColor(0));
    }

    strip.Show();

    for (size_t i = 0; i < PIXELCOUNT; i++)
    {
        strip.SetPixelColor(i, colorGamma.Correct(red));
        strip.Show();
    }
    delay(500);

    for (size_t i = 0; i < PIXELCOUNT; i++)
    {
        strip.SetPixelColor(i, colorGamma.Correct(green));
        strip.Show();
    }
    delay(500);

    for (size_t i = 0; i < PIXELCOUNT; i++)
    {
        strip.SetPixelColor(i, colorGamma.Correct(blue));
        strip.Show();
    }
    delay(500);

    for (size_t i = 0; i < PIXELCOUNT; i++)
    {
        strip.SetPixelColor(i, colorGamma.Correct(white));
        strip.Show();
    }

    delay(500);
    for (size_t i = 0; i < PIXELCOUNT; i++)
    {
        strip.SetPixelColor(i, colorGamma.Correct(black));
        strip.Show();
    }
}

void loopLEDs()
{
    switch (batteryStatus)
    {
    case BATTERYSTATUS::BATT_FULL:
        break;
    case BATTERYSTATUS::BATT_LOW:
        if (millis() - batteryCriticalMillis > 1000)
        {
            if (strip.GetPixelColor(0).CompareTo(batteryLowColor) == 0)
                strip.SetPixelColor(0, black);
            else
                strip.SetPixelColor(0, batteryLowColor);

            batteryCriticalMillis = millis();
            strip.Show();
        }
        break;
    case BATTERYSTATUS::BATT_CRITICAL:
        strip.SetPixelColor(0, batteryCriticalColor);
        strip.Show();
        break;

    default:
        break;
    }
}