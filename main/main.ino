#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "RTClib.h"

#include "lcd.h"
#include "button.h"
#include "led.h"

#define LED 4 // pin for error led
#define LEFT_BUTTON_PIN 2 // pin for left button
#define RIGHT_BUTTON_PIN 3 // pin for right button

// sensor pins
#define LIGHT A0 // light sensor
#define MOISTURE A1 // moisture sensor
#define TEMP A2 // temperature sensor

hd44780_I2Cexp lcd; // lcd object
Button leftBut; // left button object
Button rightBut; // right button object
RTC_DS3231 rtc; // real time clock object

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

bool daytime; // track if it is day

const char* levelNames[] = {"Low", "Medium", "High"}; // level names
const int numLevels = sizeof(levelNames) / sizeof(levelNames[0]); // number of levels

// bounds for error checking
const float moistureBounds[] = {0, 1, 2, 3}; 
const float lightBounds[] = {0, 1, 2, 3};
const float tempBounds[] = {0, 1, 2, 3};

State currentState; // current system state
Level moistureLevel; // user selected moisture level
Level lightLevel; // user selected light level
Level tempLevel; // user selected temp level

// timing for sensors and error checks
const unsigned long SENSOR_CHECK_INTERVAL = 60000; // 1 min interval
const unsigned long ERROR_CHECK_INTERVAL = 1800000; // 30 min interval

const unsigned long NUM_MEASUREMENTS = ERROR_CHECK_INTERVAL / SENSOR_CHECK_INTERVAL; // measurements per error check

// timing trackers
unsigned long lastSensorCheckTime;
unsigned long lastErrorCheckTime;
unsigned long idleStartTime;
unsigned long errorStartTime;

// running sums for sensor readings
float moisture = 0;
float light = 0;
float temp = 0;

// track which errors are active
bool moistureLowError = false;
bool moistureHighError = false;
bool lightLowError = false;
bool lightHighError = false;
bool tempLowError = false;
bool tempHighError = false;

// lcd and led state tracking
bool lcdOn = false;
bool ledOn = false;

void resetSensorValues()
{
  moisture = 0;
  light = 0;
  temp = 0;
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

// add current sensor readings to running totals
void checkSensors()
{
  moisture += analogRead(MOISTURE);
  light += analogRead(LIGHT);
  temp += analogRead(TEMP);
}

// check averages and set error flags
void checkForError()
{
  float avgMoisture = moisture / NUM_MEASUREMENTS;
  float avgLight = light / NUM_MEASUREMENTS;
  float avgTemp = temp / NUM_MEASUREMENTS;

  // moisture bounds check
  if (avgMoisture < moistureBounds[moistureLevel]) {
        moistureLowError = true;
        currentState = ERROR;
  } else if (avgMoisture > moistureBounds[moistureLevel + 1]) {
      moistureHighError = true;
      currentState = ERROR;
  }

  // light bounds check
  if (avgLight < lightBounds[lightLevel]) {
      lightLowError = true;
      currentState = ERROR;
  } else if (avgLight > lightBounds[lightLevel + 1]) {
      lightHighError = true;
      currentState = ERROR;
  }

  // temp bounds check
  if (avgTemp < tempBounds[tempLevel]) {
      tempLowError = true;
      currentState = ERROR;
  } else if (avgTemp > tempBounds[tempLevel + 1]) {
      tempHighError = true;
      currentState = ERROR;
  }

  resetSensorValues();
  errorStartTime = millis();
}

// build error message and write to lcd
String write_error(int line)
{
  String error = "";
  int numErrors = 0;

  if (moistureLowError) {
    error += "Low Moisture";
    numErrors ++;
  }
  
  if (moistureHighError) {
    if (numErrors > 0) error += ", ";
    error += "High Moisture";
    numErrors++;
  }

  if (lightLowError) {
    if (numErrors > 0) error += ", ";
    error += "Low Light";
    numErrors ++;
  }

  if (lightHighError) {
    if (numErrors > 0) error += ", ";
    error += "High Light";
    numErrors++;
  }

  if (tempLowError) {
    if (numErrors > 0) error += ", ";
    error += "Low Temp";
    numErrors ++;
  }
  
  if (tempHighError) {
    if (numErrors > 0) error += ", ";
    error += "High Temp";
    numErrors++;
  }

  if (lcd_write(&lcd, error, line)) {
    return error;
  }
  return "";
}

// return true if any error flags set
bool hasError()
{
  return moistureLowError || moistureHighError || lightLowError || lightHighError || tempLowError || tempHighError;
}

// periodically update sensors and errors
void updateSensorsAndErrors()
{
  if (millis() - lastSensorCheckTime >= SENSOR_CHECK_INTERVAL) {
    checkSensors();
    lastSensorCheckTime = millis();
  }

  if (millis() - lastErrorCheckTime >= ERROR_CHECK_INTERVAL) {
    checkForError();
    lastErrorCheckTime = millis();
  }
}

void showBacklight() {
  if (daytime) {
    lcd.backlight();
  }
}

void setup()
{
  Serial.begin(9600);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // set rtc to current time

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
  DateTime now = rtc.now();
  
  if (now.hour() >= 9 && now.hour() <= 21) { // 9am to 9pm is day
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

      currentState = IDLE;
      lastSensorCheckTime = millis();
      lastErrorCheckTime = millis();
      break;

    case IDLE:
      updateSensorsAndErrors();

      if (!hasError()) {
        static bool firstIdleRun = true;

        if (firstIdleRun) {
          showBacklight();
          lcd.clear();
          lcd_write(&lcd, "No Error!", 0);
          idleStartTime = millis();
          lcdOn = true;
          firstIdleRun = false;
        }

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
        currentState = ERROR;
      }
      break;

    case ERROR:
      updateSensorsAndErrors();

      if (hasError()) {
        static String error;
        static unsigned long shiftTime;
        static bool firstErrorRun = true;

        if (firstErrorRun) {
          showBacklight();
          lcd.clear();
          error = write_error(0);
          lcdOn = true;
          errorStartTime = millis();
          shiftTime = millis();
          firstErrorRun = false;
        }

        if (ledOn) {
          digitalWrite(LED, LOW);
        } else {
          digitalWrite(LED, HIGH);
        }
        
        ledOn = !ledOn;

        // turn off lcd after 5 sec
        if (millis() - errorStartTime >= 5000 && lcdOn) {
          lcd.noBacklight();
          lcdOn = false;
        }

        // scroll error text
        if (millis() - shiftTime >= 750 && error.length() > 0) {
          shift_text(&lcd, error, 0);
          shiftTime = millis();
        }

        // left button clears errors or wakes lcd
        if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
          if (!lcdOn) {
            lcd.backlight();
            lcdOn = true;
            errorStartTime = millis();
          } else {
            resetSensorValues();
            currentState = IDLE;
            firstErrorRun = true;
          }
        }

        // right button refreshes error display
        if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
          lcd.backlight();
          lcdOn = true;
          errorStartTime = millis();
          firstErrorRun = true;
          // checkForError(); uncomment when sensors are added
        }
      } else {
        currentState = IDLE;
      }
      break;
  }
}