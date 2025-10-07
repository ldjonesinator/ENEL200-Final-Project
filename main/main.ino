#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "RTClib.h"

#include "lcd.h"
#include "button.h"
#include "led.h"
#include "error.h"
#include "time.h"

State currentState;
State previousState = SETUP; // previous system state
ErrorSubState errorSubState; // current sub-state in the ERROR state

bool firstErrorScrollRun = true;
bool firstErrorInstantRun = true;

// allow user to select a level (LOW, MED or HIGH)
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
        }
        if (currentLevel >= numLevels) {
            currentLevel = LOWER;
        }
        lcd_write(&lcd, levelNames[currentLevel], 1);
    }
}

// allow user to select an hour
int getHour(int startHour24, int endHour24)
{
    int currentHour24 = startHour24;

    lcd_write(&lcd, hourToString(currentHour24), 1);

    while (1) {
        // left button confirms selection
        if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
            return currentHour24;
        }

        // right button cycles through hours
        if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
            currentHour24++;
        }
        if (currentHour24 > endHour24) {
            currentHour24 = startHour24;
            lcd_write(&lcd, hourToString(currentHour24), 1);
        }
    }
}

// prompts for the plant levels
void plantPrompts(Level* levels, int line) {
    const String prompts[3] = {"Soil moisture", "Light level", "Warmth"};
    for (size_t i = 0; i < sizeof(prompts); i ++) {
        lcd_write(&lcd, prompts[i] + "?", line);
        levels[i] = getLevel();
        Serial.print(prompts[i] + " set to: ");
        Serial.println(levelNames[plantLevels[0]]);
    }

    lcd_write(&lcd, "Set day start", line);
    dayStartHour = getHour(1, 12);
    Serial.print("Day start hour set to ");
    Serial.println(dayStartHour);

    lcd_write(&lcd, "Set night start", line);
    nightStartHour = getHour(13, 24);
    Serial.print("Night start hour set to ");
    Serial.println(nightStartHour);
}

void setup()
{
    Serial.begin(9600);
    Wire.begin();

    while (!rtc.begin()) {
        continue;
    }

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    lcd_setup(&lcd, true);
    pinMode(LED, OUTPUT);

    initialise_button(&leftBut, "L", LEFT_BUTTON_PIN);
    initialise_button(&rightBut, "R", RIGHT_BUTTON_PIN);

    currentState = SETUP; // start system in setup
}

void loop()
{
    now = rtc.now(); // current time

    // compute whether it's currently daytime
    if (now.hour() >= dayStartHour && now.hour() <= nightStartHour) {
        daytime = true;
    } else {
        daytime = false;
    }

    switch (currentState) {
        case SETUP:
            lcd.backlight();
            plantPrompts(plantLevels, 0);

            currentState = IDLE;
            break;

        case IDLE:
            checkSensorsAndErrors();

            if (!hasError()) {
                // turn off lcd after 5 seconds of inactivity
                if (lcdOn && (millis() - idleStartTime) >= 5000) {
                    lcd.noBacklight();
                    lcdOn = false;
                }

                // pressing left button wakes lcd for another 5 seconds
                if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
                    lcd.backlight();
                    idleStartTime = millis();
                    lcdOn = true;
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

            // get out of the error state if there's no error
            if (!hasError()) {
                digitalWrite(LED, LOW);
                ledOn = false;
                currentState = IDLE;
                break;
            }

            static String errorMsg;
            static unsigned long shiftTime;

            static int instantMeasurementType = 0;
            static int lastDisplayedType = -1;
            float instantMoisture = 0;
            float instantLight = 0;
            float instantTemp = 0;

            // turn led on if it's daytime
            if (daytime && !ledOn) {
                digitalWrite(LED, HIGH);
                ledOn = true;
            }

            // system starts in ERROR_SCROLL by default
            switch (errorSubState) {
                case ERROR_INSTANT_MEASUREMENT:
                    if (firstErrorInstantRun) {
                        lcd.clear();
                        lastDisplayedType = -1;
                        instantMeasurementType = 0;
                        instantMoisture = analogRead(MOISTURE);
                        instantLight = analogRead(LIGHT);
                        instantTemp = analogRead(TEMP);
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
                            case 0:
                                measurementType = "Moisture";
                                currentValue = instantMoisture;
                                idealValue = (moistureBounds[plantLevels[0]] + moistureBounds[plantLevels[0] + 1]) / 2;
                                break;
                            case 1:
                                measurementType = "Light";
                                currentValue = instantLight;
                                idealValue = (lightBounds[plantLevels[1]] + lightBounds[plantLevels[1] + 1]) / 2;
                                break;
                            case 2:
                                measurementType = "Temperature";
                                currentValue = instantTemp;
                                idealValue = (tempBounds[plantLevels[2]] + tempBounds[plantLevels[2] + 1]) / 2;
                                break;
                        }

                        lcd_write(&lcd, measurementType, 0);
                        float signedPercentageDifference = ((currentValue - idealValue) / idealValue) * 100;
                        lcd_write(&lcd, String(signedPercentageDifference, 1) + "% from ideal", 1);
                        lastDisplayedType = instantMeasurementType;
                    }
                    
                    // right button moves to the next measurement type
                    if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
                        instantMeasurementType = (instantMeasurementType + 1) % 3;
                    }

                    // left button click goes back to the regular (scrolling) error state
                    if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
                        lastDisplayedType = -1;
                        errorSubState = ERROR_SCROLL;
                        firstErrorScrollRun = true;
                    }
                    break;

                case ERROR_SCROLL:
                    if (firstErrorScrollRun) {
                        if (daytime) {
                            lcd.backlight();
                        }
                        lcd.clear();
                        errorMsg = write_error(0);
                        lcdOn = true;
                        errorStartTime = millis();
                        shiftTime = millis();
                        firstErrorScrollRun = false;
                    }
                    
                    if (millis() - shiftTime >= 750 && errorMsg.length() > 0) {
                        errorMsg = build_error();
                        shift_text(&lcd, errorMsg, 0);
                        shiftTime = millis();
                    }

                    if (lcdOn && millis() - errorStartTime >= 5000) {
                        lcd.noBacklight();
                        lcdOn = false;
                    }

                    if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
                        if (!lcdOn) {
                            lcd.backlight();
                            lcdOn = true;
                            errorStartTime = millis();

                        } else {
                            resetSensorValues();
                            clearErrorFlags();
                            currentState = IDLE;
                            idleStartTime = millis();
                            lcd.backlight();
                            digitalWrite(LED, LOW);
                            ledOn = false;
                            firstErrorScrollRun = true;
                            firstErrorInstantRun = true;
                        }
                    }

                    if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
                        instantMoisture = analogRead(MOISTURE);
                        instantLight = analogRead(LIGHT);
                        instantTemp = analogRead(TEMP);
                        instantMeasurementType = 0;
                        lastDisplayedType = -1;
                        lcd.backlight();
                        lcdOn = true;
                        errorSubState = ERROR_INSTANT_MEASUREMENT;
                        firstErrorInstantRun = true;
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