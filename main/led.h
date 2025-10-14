#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <stdint.h>
#include "state.h"

#define LED 4

// Tracks whether the LED is on
extern bool ledOn;

void updateLED(bool daytime, State currentState); // Updates the LED depending on the time and if there is an error 

#endif // LED_H