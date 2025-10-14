#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "RTClib.h"

#include "lcd.h"
#include "button.h"
#include "led.h"
#include "error.h"
#include "time.h"
#include "state.h"

State currentState;
State previousState = SETUP; // Previous system state
ErrorSubState errorSubState; // Current sub-state in the ERROR state

bool firstErrorScrollRun = true;
bool firstErrorInstantRun = true;
bool ifShouldShift;

// Allow user to select a level
Level getLevel()
{
    char choice;
    Level currentLevel = LOWER;

    lcd_write(&lcd, levelNames[currentLevel], 1);

    while (1) {
        // left button confirms level
        if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
            return currentLevel;
        }

        // right button cycles through levels
        if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
            currentLevel = (Level)(currentLevel + 1);
            if (currentLevel >= numLevels) {
                currentLevel = LOWER;
            }
            lcd_write(&lcd, levelNames[currentLevel], 1);
        }

        if (digitalRead(RIGHT_BUTTON_PIN) == HIGH && !rightBut.pressed) {
            update_button("R", HIGH);
        } else if (digitalRead(RIGHT_BUTTON_PIN) == LOW && rightBut.pressed) {
            update_button("R", LOW);
            currentLevel = (Level)(currentLevel + 1);
            if (currentLevel >= numLevels) {
                currentLevel = LOWER;
            }
            lcd_write(&lcd, levelNames[currentLevel], 1);
        }
    }
}

// Allow user to select an hour
int getHour(int startHour24, int endHour24)
{
    int currentHour24 = startHour24;

    lcd_write(&lcd, hourToString(currentHour24), 1);

    while (1) {
        // Left button confirms selection
        if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
            return currentHour24;
        }

        // Right button cycles through hours
        if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
            currentHour24++;
            if (currentHour24 > endHour24) {
                currentHour24 = startHour24;
            }
            lcd_write(&lcd, hourToString(currentHour24), 1);
        }

        if (digitalRead(RIGHT_BUTTON_PIN) == HIGH && !rightBut.pressed) {
            update_button("R", HIGH);
        } else if (digitalRead(RIGHT_BUTTON_PIN) == LOW && rightBut.pressed) {
            update_button("R", LOW);
            currentHour24++;
            if (currentHour24 > endHour24) {
                currentHour24 = startHour24;
            }
            lcd_write(&lcd, hourToString(currentHour24), 1);
        }
    }
}

void setup()
{
    Serial.begin(9600);
    Wire.begin();

    while (!rtc.begin()) {
        continue;
    }

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set RTC date and time to current system date and time

    lcd_setup(&lcd, true);
    pinMode(LED, OUTPUT);

    pinMode(LEFT_BUTTON_PIN, INPUT);
    pinMode(RIGHT_BUTTON_PIN, INPUT);
    initialise_button(&leftBut, "L");
    initialise_button(&rightBut, "R");

    currentState = SETUP; // Start monitor in setup state
}

void loop()
{
    now = rtc.now(); // Get the current time

    // Compute whether it's currently daytime
    if (now.hour() >= dayStartHour && now.hour() <= nightStartHour) {
        daytime = true;
    } else {
        daytime = false;
    }

    updateLED(daytime, currentState);

    switch (currentState) {
        case SETUP:
        lcd.backlight();

        lcd_write(&lcd, "Soil moisture?", 0);
        moistureLevel = getLevel();
        Serial.print("Soil moisture set to: ");
        Serial.println(levelNames[moistureLevel]);

        lcd_write(&lcd, "Light level?", 0);
        lightLevel = getLevel();
        Serial.print("Light level set to: ");
        Serial.println(levelNames[lightLevel]);

        lcd_write(&lcd, "Warmth?", 0);
        tempLevel = getLevel();
        Serial.print("Warmth set to: ");
        Serial.println(levelNames[tempLevel]);

        lcd_write(&lcd, "Set day start", 0);
        dayStartHour = getHour(1, 12);
        Serial.print("Day start hour set to ");
        Serial.println(dayStartHour);

        lcd_write(&lcd, "Set night start", 0);
        nightStartHour = getHour(13, 24);
        Serial.print("Night start hour set to ");
        Serial.println(nightStartHour);

        currentState = IDLE;
        break;

        case IDLE:
            checkSensorsAndErrors();

            if (!hasError()) {
                // turn off lcd after 5 seconds of inactivity
                if (lcdOn && millis() - idleStartTime >= 5000) {
                lcd.noBacklight();
                lcdOn = false;
                }

                // pressing left button wakes lcd for another 5 seconds
                if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
                lcd.backlight();
                idleStartTime = millis();
                lcdOn = true;
                }

                if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
                    resetSensorValues();
                    clearErrorFlags();
                    currentState = SETUP;
                }
            } else {
                errorSubState = ERROR_SCROLL;
                firstErrorScrollRun = true;
                firstErrorInstantRun = true;
                currentState = ERROR;
            }
            break;

        case ERROR:
            checkSensorsAndErrors();

            // Get out of the error state if there's no error
            if (!hasError() && errorSubState == ERROR_SCROLL) {
                currentState = IDLE;
                break;
            }

            static String errorMsg;
            static unsigned long shiftTime;

            static int instantMeasurementType = 0;
            static int lastDisplayedType = -1;
            static float instantMoisture = 0;
            static float instantLight = 0;
            static float instantTemp = 0;

            // Monitor starts in ERROR_SCROLL state by default
            switch (errorSubState) {
                case ERROR_SCROLL:
                    if (firstErrorScrollRun) {
                        if (daytime) {
                            lcd.backlight();
                        }
                        lcd.clear();
                        errorMsg = build_error();
                        lcdOn = true;
                        errorStartTime = millis();
                        shiftTime = millis();
                        firstErrorScrollRun = false;
                        ifShouldShift = lcd_write(&lcd, errorMsg, 0);
                    }

                    String newMsg = build_error();
                    if (newMsg != errorMsg) {
                        errorMsg = newMsg;
                        lcd.clear();
                        ifShouldShift = lcd_write(&lcd, errorMsg, 0);
                        shiftTime = millis();
                    }

                    // Only scroll if still needed
                    if (ifShouldShift && millis() - shiftTime >= 700) {
                        shift_text(&lcd, errorMsg, 0);
                        shiftTime = millis();
                    }

                    // Turn LCD backlight off after 5 seconds
                    if (lcdOn && millis() - errorStartTime >= 5000) {
                        lcd.noBacklight();
                        lcdOn = false;
                    }

                    if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
                        if (!lcdOn) { // Turn display on if LCD display is off
                            lcd.backlight();
                            lcdOn = true;
                            errorStartTime = millis();

                        } else { // Dismiss error if the LCD display is on
                            resetSensorValues();
                            clearErrorFlags();
                            currentState = IDLE;
                            idleStartTime = millis();
                            lcd.backlight();
                            firstErrorScrollRun = true;
                            firstErrorInstantRun = true;
                        }
                    }

                    if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) { // Take instant measurement
                        instantMeasurementType = 0;
                        lastDisplayedType = -1;
                        lcd.backlight();
                        lcdOn = true;
                        errorSubState = ERROR_INSTANT_MEASUREMENT;
                        firstErrorInstantRun = true;
                    }
                    break;

                case ERROR_INSTANT_MEASUREMENT:
                    if (firstErrorInstantRun) {
                        lcd.clear();
                        lastDisplayedType = -1;
                        instantMeasurementType = 0;
                        instantMoisture = analogRead(MOISTURE);
                        instantLight = analogRead(LIGHT);
                        instantTemp = adcToResistance(analogRead(TEMP));
                        lcd.backlight();
                        lcdOn = true;
                        firstErrorInstantRun = false;
                    }

                    if (instantMeasurementType != lastDisplayedType) {
                        lcd.clear();
                        firstErrorInstantRun = false;
                        String measurementType;
                        float currentValue;
                        float idealValue;

                        switch (instantMeasurementType) {
                            case 0: // Moisture
                                measurementType = "Moisture";
                                currentValue = instantMoisture;
                                idealValue = (moistureBounds[moistureLevel] + moistureBounds[moistureLevel + 1]) / 2;
                                break;
                            
                            case 1: // Light
                                measurementType = "Light";
                                currentValue = instantLight;
                                idealValue = (lightBounds[lightLevel] + lightBounds[lightLevel + 1]) / 2;
                                break;
                            
                            case 2: // Temperature
                                measurementType = "Temperature";
                                currentValue = instantTemp;
                                idealValue = (tempBounds[tempLevel + 1] + tempBounds[tempLevel]) / 2;
                                break;
                        }

                        lcd_write(&lcd, measurementType, 0);
                        float signedPercentageDifference;

                        switch (instantMeasurementType) {
                            case 0: // Moisture
                                signedPercentageDifference = ((idealValue - currentValue) / idealValue) * 100;
                                break;
                            
                            case 1: // Light
                                signedPercentageDifference = ((idealValue - currentValue) / idealValue) * 100;
                                break;
                            
                            case 2: // Temperature
                                signedPercentageDifference = ((currentValue - idealValue) / idealValue) * 100;
                                break;
                        }

                        lcd_write(&lcd, String(signedPercentageDifference, 1) + "% from ideal", 1);
                        lastDisplayedType = instantMeasurementType;
                    }
                    
                    // Right button moves to the next measurement type
                    if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
                        instantMeasurementType = (instantMeasurementType + 1) % 3;
                    }

                    // Left button goes back to the regular (scrolling) error state
                    if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
                        lastDisplayedType = -1;
                        errorSubState = ERROR_SCROLL;
                        firstErrorScrollRun = true;
                    }
                    break;
            }
            break;
        }

        if (currentState != previousState) {
            if (currentState == IDLE) {
                lcd.backlight();
                lcd.clear();
                lcd_write(&lcd, "I'm OK! (^_^)", 0);
                idleStartTime = millis();
                lcdOn = true;
            }
            previousState = currentState;
        }
    }