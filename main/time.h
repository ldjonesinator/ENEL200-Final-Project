#ifndef TIME_H
#define TIME_H

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

// RTC and DateTime objects
extern RTC_DS3231 rtc;
extern DateTime now;

// Timing intervals
constexpr unsigned long SENSOR_CHECK_INTERVAL_SEC = 10;
constexpr unsigned long ERROR_CHECK_INTERVAL_SEC = 60;

// Timing trackers
extern unsigned long idleStartTime;
extern unsigned long errorStartTime;
extern long lastSensorCheckSec;
extern long lastErrorCheckSec;
extern int dayStartHour;
extern int nightStartHour;
extern bool daytime;

// User-chosen hour for start of day-time and night-time
extern int dayStartHour;
extern int nightStartHour;

String hourToString(int hour24); // Converts 24-hour hour to 12-hour string with AM/PM
long currentTimeInSeconds(); // Returns the current time in seconds since midnight

#endif // TIME_H