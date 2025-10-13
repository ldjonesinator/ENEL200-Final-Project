#ifndef TIME_H
#define TIME_H

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

extern RTC_DS3231 rtc; // real time clock object
extern DateTime now; // real time clock time

// timing intervals
constexpr unsigned long SENSOR_CHECK_INTERVAL_SEC = 60;
constexpr unsigned long ERROR_CHECK_INTERVAL_SEC = 1800;

// timing trackers
extern unsigned long idleStartTime;
extern unsigned long errorStartTime;
extern long lastSensorCheckSec;
extern long lastErrorCheckSec;

extern int dayStartHour;
extern int nightStartHour;
extern bool daytime;

// user-chosen hour for start of day and night
extern int dayStartHour;
extern int nightStartHour;

String hourToString(int hour24); // convert 24-hour hour to 12-hour string with AM/PM
long currentTimeInSeconds();

#endif // TIME_H