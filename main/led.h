#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <stdint.h>

#define LED 4 // pin for error led

extern bool ledOn;

void updateLED(uint8_t ledPin, bool timeDay, bool isError);

#endif // LED_H