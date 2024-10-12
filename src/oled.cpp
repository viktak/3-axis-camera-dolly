// #include <TimeLib.h>

// #include "i2c.h"
// #include "oled.h"
// #include "main.h"

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oLed(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/SCL_GPIO, /* data=*/SDA_GPIO); // ESP32 Thing, HW I2C with pin remapping

// unsigned long oLedMillis = 0;

// void setupOLED()
// {
//     oLed.begin();

//     oLed.clearBuffer();
//     oLed.setFont(u8g2_font_ncenB08_tr);
//     oLed.drawStr(0, 10, "Camera Dolly 1.0!");
//     oLed.sendBuffer();
// }

// void DisplayTime()
// {

//     oLed.clearBuffer();

//     String myTime = "";
//     char s[2];

//     //  hours
//     if (hour(now()) < 10)
//         myTime += " ";

//     itoa((hour(now())), s, DEC);
//     myTime += s;

//     oLed.drawStr(3, 20, s);

//     //  minutes
//     myTime = "";
//     if (minute(now()) < 10)
//         myTime += "0";

//     itoa(minute(now()), s, DEC);
//     myTime += s;

//     oLed.drawStr(3, 30, myTime.c_str());

//     //  seconds
//     myTime = "";
//     if (second(now()) < 10)
//         myTime += "0";

//     itoa(second(now()), s, DEC);
//     myTime += s;

//     oLed.drawStr(3, 40, myTime.c_str());

//     oLed.sendBuffer();
// }

// void loopOLED()
// {
// }