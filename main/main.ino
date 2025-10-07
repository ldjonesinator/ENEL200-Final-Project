#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "RTClib.h"

#include "lcd.h"
#include "button.h"
#include "led.h"

// pin definitions
#define LED 4 // pin for error led
#define LEFT_BUTTON_PIN 2 // pin for left button
#define RIGHT_BUTTON_PIN 3 // pin for right button

// sensor pins
#define LIGHT A0 // light sensor
#define MOISTURE A1 // moisture sensor
#define TEMP A2 // temperature sensor

// lcd, button, and rtc objects
hd44780_I2Cexp lcd; // lcd object
Button leftBut; // left button object
Button rightBut; // right button object
RTC_DS3231 rtc; // real time clock object
DateTime now; // real time clock time

// system states
typedef enum {
    SETUP,
    IDLE,
    ERROR
} State; // system states

typedef enum {
    LOWER,
    MEDIUM,
    HIGHER
} Level; // level options

typedef enum {
    ERROR_SCROLL,
    ERROR_INSTANT_MEASUREMENT
} ErrorSubState;

bool daytime; // track if it is day

// user level names
const char* levelNames[] = {"Low", "Medium", "High"}; // level names
const int numLevels = sizeof(levelNames) / sizeof(levelNames[0]); // number of levels

// bounds for error checking
const float moistureBounds[] = {525, 490, 370, 273}; // air -> moist -> water
const float lightBounds[] = {1000, 275, 60, 0}; // low -> medium -> high
const float tempBounds[] = {1000, 187, 175, 0}; // low (10-20 degrees) -> medium (20-25 degrees) -> high (25-30 degrees)

// timing intervals
const unsigned long SENSOR_CHECK_INTERVAL_SEC = 10;
const unsigned long ERROR_CHECK_INTERVAL_SEC = 60;

// state tracking
State currentState; // current system state
ErrorSubState errorSubState; // current sub-state in the ERROR state
Level moistureLevel; // user selected moisture level
Level lightLevel; // user selected light level
Level tempLevel; // user selected temp level

// timing trackers
unsigned long idleStartTime;
unsigned long errorStartTime;
long lastSensorCheckSec = -1;
long lastErrorCheckSec  = -1;

// measurement tracking
int numMeasurements = 0; // counts the number of measurements (used for averaging)
float moisture = 0; // running sums for sensor readings
float light = 0;
float temp = 0;

// error flags
bool moistureLowError = false;
bool moistureHighError = false;
bool lightLowError = false;
bool lightHighError = false;
bool tempLowError = false;
bool tempHighError = false;

// lcd and led state tracking
bool lcdOn = false;
bool ledOn = false;

// user-chosen hour for start of day and night
int dayStartHour;
int nightStartHour;

bool firstScrollRun = true;
bool firstInstantRun = true;

State previousState = SETUP;

// reset all sensor data
void resetSensorValues()
{
    moisture = 0;
    light = 0;
    temp = 0;
    numMeasurements = 0;
}

// reset all error flags
void clearErrorFlags()
{
    moistureLowError = false;
    moistureHighError = false;
    lightLowError = false;
    lightHighError = false;
    tempLowError = false;
    tempHighError = false;
}

// allow user to select a level
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

// convert 24-hour hour to 12-hour string with AM/PM
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
        if (currentHour24 > endHour24) currentHour24 = startHour24;
        lcd_write(&lcd, hourToString(currentHour24), 1);
        }

        if (digitalRead(RIGHT_BUTTON_PIN) == HIGH && !rightBut.pressed) {
        update_button("R", HIGH);
        } else if (digitalRead(RIGHT_BUTTON_PIN) == LOW && rightBut.pressed) {
        update_button("R", LOW);
        currentHour24++;
        if (currentHour24 > endHour24) currentHour24 = startHour24;
        lcd_write(&lcd, hourToString(currentHour24), 1);
        }
    }
}

// add current sensor readings to running totals
void checkSensors()
{
    moisture += analogRead(MOISTURE);
    light += analogRead(LIGHT);
    temp += analogRead(TEMP);
    numMeasurements++;
}

// check averages and set error flags
void checkForError()
{
    clearErrorFlags();
    float avgMoisture = moisture / numMeasurements;
    float avgLight = light / numMeasurements;
    float avgTemp = temp / numMeasurements;

    // moisture bounds check
    if (avgMoisture > moistureBounds[moistureLevel]) {
            moistureLowError = true;
            currentState = ERROR;
    } else if (avgMoisture < moistureBounds[moistureLevel + 1]) {
        moistureHighError = true;
        currentState = ERROR;
    }

    // light bounds check
    if (avgLight > lightBounds[lightLevel]) {
        lightLowError = true;
        currentState = ERROR;
    } else if (avgLight < lightBounds[lightLevel + 1]) {
        lightHighError = true;
        currentState = ERROR;
    }

    // temp bounds check
    if (avgTemp > tempBounds[tempLevel]) {
        tempLowError = true;
        currentState = ERROR;
    } else if (avgTemp < tempBounds[tempLevel + 1]) {
        tempHighError = true;
        currentState = ERROR;
    }

    resetSensorValues();
}

// build error message string
String build_error()
{
    String error = "";
    int numErrors = 0;

    if (moistureLowError) {
        error += "Low Moisture";
        numErrors ++;
    }
    
    if (moistureHighError) {
        if (numErrors > 0) error += ", ";
        error += "Too Soggy!";
        numErrors++;
    }

    if (lightLowError) {
        if (numErrors > 0) error += ", ";
        error += "Let me sunbathe!";
        numErrors ++;
    }

    if (lightHighError) {
        if (numErrors > 0) error += ", ";
        error += "I'm Blinded!";
        numErrors++;
    }

    if (tempLowError) {
        if (numErrors > 0) error += ", ";
        error += "I'm freezing!";
        numErrors ++;
    }
    
    if (tempHighError) {
        if (numErrors > 0) error += ", ";
        error += "I'm burning!";
        numErrors++;
    }

    return error;
}

// build error message and write to lcd
String write_error(int line)
{
    String error = build_error();

    if (lcd_write(&lcd, error, line)) {
        return error;
    }
    return "";
}

// return true if there's any error
bool hasError()
{
    return moistureLowError || moistureHighError || lightLowError || lightHighError || tempLowError || tempHighError;
}

long currentTimeInSeconds()
{
    return now.hour() * 3600L + now.minute() * 60L + now.second();
}

// check sensors and check for error if it's time to do so
void checkSensorsAndErrors()
{
    long currentSec = currentTimeInSeconds();

    if ((currentSec % SENSOR_CHECK_INTERVAL_SEC) == 0 && currentSec != lastSensorCheckSec) {
        checkSensors();
        lastSensorCheckSec = currentSec;
        Serial.println("Checking sensors at "+ String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    }

    if ((currentSec % ERROR_CHECK_INTERVAL_SEC) == 0 && currentSec != lastErrorCheckSec) {
        checkForError();
        lastErrorCheckSec = currentSec;
        Serial.println("Checking for error at "+ String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    }
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

    pinMode(LEFT_BUTTON_PIN, INPUT);
    pinMode(RIGHT_BUTTON_PIN, INPUT);
    initialise_button(&leftBut, "L");
    initialise_button(&rightBut, "R");

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

        } else {
            errorSubState = ERROR_SCROLL;
            firstScrollRun = true;
            firstInstantRun = true;
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
        static float instantMoisture = 0;
        static float instantLight = 0;
        static float instantTemp = 0;

        // turn led on if it's daytime
        if (daytime && !ledOn) {
            digitalWrite(LED, HIGH);
            ledOn = true;
        }

        // system starts in ERROR_SCROLL by default
        switch (errorSubState) {
            case ERROR_INSTANT_MEASUREMENT:
                if (firstInstantRun) {
                    lcd.clear();
                    lastDisplayedType = -1;
                    instantMeasurementType = 0;
                    instantMoisture = analogRead(MOISTURE);
                    instantLight = analogRead(LIGHT);
                    instantTemp = analogRead(TEMP);
                    lcd.backlight();
                    lcdOn = true;
                    firstInstantRun = false;
                }

                if (instantMeasurementType != lastDisplayedType) {
                    lcd.clear();
                    firstInstantRun = false;
                    String measurementType;
                    float currentValue;
                    float idealValue;

                    switch (instantMeasurementType) {
                        case 0:
                            measurementType = "Moisture";
                            currentValue = instantMoisture;
                            idealValue = (moistureBounds[moistureLevel] + moistureBounds[moistureLevel + 1]) / 2;
                            break;
                        case 1:
                            measurementType = "Light";
                            currentValue = instantLight;
                            idealValue = (lightBounds[lightLevel] + lightBounds[lightLevel + 1]) / 2;
                            break;
                        case 2:
                            measurementType = "Temperature";
                            currentValue = instantTemp;
                            idealValue = (tempBounds[tempLevel] + tempBounds[tempLevel + 1]) / 2;
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
                    firstScrollRun = true;
                }
                break;

            case ERROR_SCROLL:
                if (firstScrollRun) {
                    if (daytime) {
                        lcd.backlight();
                    }
                    lcd.clear();
                    errorMsg = write_error(0);
                    lcdOn = true;
                    errorStartTime = millis();
                    shiftTime = millis();
                    firstScrollRun = false;
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
                        firstScrollRun = true;
                        firstInstantRun = true;
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
                    firstInstantRun = true;
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