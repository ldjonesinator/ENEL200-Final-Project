#include "led.h"

bool ledOn = false;

void updateLED(bool daytime, State currentState)
{
    if (daytime && currentState == ERROR && !ledOn) {
        digitalWrite(LED, HIGH);
        ledOn = true;
    } else if (currentState == SETUP || currentState == IDLE && ledOn) {
        digitalWrite(LED, LOW);
        ledOn = false;
    }
}