#include "time.h"

RTC_DS3231 rtc;
DateTime now;

unsigned long idleStartTime;
unsigned long errorStartTime;
long lastSensorCheckSec = -1;
long lastErrorCheckSec  = -1;
int dayStartHour;
int nightStartHour;
bool daytime;

String hourToString(int hour24)
{
    int hour12 = hour24 % 12;
    if (hour12 == 0) {
        hour12 = 12;
    }

    String period;
    if (hour24 < 12 || hour24 == 24) {
        period = "AM";
    } else {
        period = "PM";
    }

    return "Hour " + String(hour12) + " " + period;
}

long currentTimeInSeconds()
{
    return now.hour() * 3600L + now.minute() * 60L + now.second();
}