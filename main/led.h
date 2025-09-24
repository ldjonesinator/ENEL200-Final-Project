#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <stdint.h>
//#include ---

void updateLED(uint8_t ledPin, bool timeDay, bool isError);

#endif //LED_H
