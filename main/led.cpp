#include "led.h"

void updateLED(uint8_t ledPin, bool timeDay, bool isError)          // updates the LED depending on the time or if there is an error 
{
    if (timeDay) {              // a function that returns true if it is daytime
        if (isError) {    // turns LED on
            digitalWrite(ledPin, HIGH);
        } else {                // turns LED off
            digitalWrite(ledPin, LOW);
        }
    } else {                    // turns LED off
        digitalWrite(ledPin, LOW);
    }
}

