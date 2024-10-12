#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <rom/rtc.h>

#define SerialMon Serial

extern String TimeIntervalToString(const time_t time);
extern uint32_t GetChipID();
extern String GetDeviceMAC();
extern const char *GetResetReasonString(RESET_REASON reason);

#endif