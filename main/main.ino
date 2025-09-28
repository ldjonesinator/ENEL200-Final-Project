#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "lcd.h"
#include "button.h"
#include "led.h"

#define LED 4
#define LEFT_BUTTON_PIN 2 // change the pin to whatever the schematic is
#define RIGHT_BUTTON_PIN 3 // change the pin to whatever the schematic is

// sensor pins
#define LIGHT A0
#define MOISTURE A1
#define TEMP A2

hd44780_I2Cexp lcd;
Button leftBut;
Button rightBut;

typedef enum {
  SETUP,
  IDLE,
  ERROR
} State;

typedef enum {
  LOWER,
  MEDIUM,
  HIGHER
} Level;

bool daytime;

const char* levelNames[] = {"Low", "Medium", "High"};
const int numLevels = sizeof(levelNames) / sizeof(levelNames[0]);;

const float moistureRange[] = {0, 1, 2, 3}; // update according to testing/datasheet of components
const float lightRange[] = {0, 1, 2, 3};
const float tempRange[] = {0, 1, 2, 3};

State currentState;

Level moistureLevel;
Level lightLevel;
Level tempLevel;

const unsigned long SENSOR_CHECK_INTERVAL = 60000; // how often to check the sensors (60000 milliseconds = 1 minute)
const unsigned long ERROR_CHECK_INTERVAL = 1800000; // how often to take the average sensor values (1800000 milliseconds = 30 minutes)

const unsigned long NUM_MEASUREMENTS = ERROR_CHECK_INTERVAL / SENSOR_CHECK_INTERVAL; // number of measurements made before checking for an error

unsigned long lastSensorCheckTime;
unsigned long lastErrorCheckTime;
unsigned long errorStartTime;

float moisture = 0;
float light = 0;
float temp = 0;

// tracks what errors have occurred
bool moistureLowError = true;
bool moistureHighError = false;
bool lightLowError = false;
bool lightHighError = false;
bool tempLowError = false;
bool tempHighError = false;

bool lcdOn = false;
bool ledOn = false;

// resets the entire system
void resetSensorValues()
{
  moisture = 0;
  light = 0;
  temp = 0;
}

Level getLevel()
{
  char choice;
  Level currentLevel = LOWER;

  lcd_write(&lcd, levelNames[currentLevel], 1);

  while (1) {
    if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
      return currentLevel;
    }
    if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
      currentLevel = (Level)(currentLevel + 1);
      if (currentLevel >= numLevels) {
        currentLevel = LOWER;
      }
      lcd_write(&lcd, levelNames[currentLevel], 1);
    }

    // cycling through options
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

void checkSensors()
{
  moisture += analogRead(MOISTURE);
  light += analogRead(LIGHT);
  temp += analogRead(TEMP);
}

void checkForError() {
  float avgMoisture = moisture / NUM_MEASUREMENTS;
  float avgLight = light / NUM_MEASUREMENTS;
  float avgTemp = temp / NUM_MEASUREMENTS;

  if (avgMoisture < moistureRange[moistureLevel]) {
        moistureLowError = true;
        currentState = ERROR;
  } else if (avgMoisture > moistureRange[moistureLevel + 1]) {
      moistureHighError = true;
      currentState = ERROR;
  }

  if (avgLight < lightRange[lightLevel]) {
      lightLowError = true;
      currentState = ERROR;
  } else if (avgLight > lightRange[lightLevel + 1]) {
      lightHighError = true;
      currentState = ERROR;
  }

  if (avgTemp < tempRange[tempLevel]) {
      tempLowError = true;
      currentState = ERROR;
  } else if (avgTemp > tempRange[tempLevel + 1]) {
      tempHighError = true;
      currentState = ERROR;
  }

  resetSensorValues();
  errorStartTime = millis();
}

String write_error(int line) {
  String error = "";
  int numErrors = 0;
  if (moistureLowError) {
    error += "Low Moisture";
    numErrors ++;
  }
  
  if (moistureHighError) {
    error += "High Moisture";
    numErrors++;
  }

  if (lightLowError) {
    if (numErrors > 0) {
      error += ", ";
    }
    error += "Low Light";
    numErrors ++;
  }

  if (lightHighError) {
    if (numErrors > 0) {
      error += ", ";
    }
    error += "High Light";
    numErrors++;
  }

  if (tempLowError) {
    if (numErrors > 0) {
      error += ", ";
    }
    error += "Low Temp";
    numErrors ++;
  }
  
  if (tempHighError) {
    if (numErrors > 0) {
      error += ", ";
    }
    error += "High Temp";
    numErrors++;
  }

  if (lcd_write(&lcd, error, line)) {
    return error;
  }
  return "";
}

bool hasError() {
  return moistureLowError || moistureHighError || lightLowError || lightHighError || tempLowError || tempHighError;
}

void setup() {
  Serial.begin(9600);

  lcd_setup(&lcd, true);
  pinMode(LED, OUTPUT);

  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  initialise_button(&leftBut, "L");
  initialise_button(&rightBut, "R");

  currentState = ERROR;
  errorStartTime = millis(); // DELETE THIS LATER
}

void loop() {
  // TO DO RIGHT HERE AT THE START OF LOOP()!
  // if it's day, switch current time to DAY
  // if the reset button is pressed, call reset()
  daytime = true;
  switch (currentState) {
    case SETUP:
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
      lcd.clear();
      lastSensorCheckTime = millis();
      lastErrorCheckTime = millis();
      break;

    case IDLE:
      // unsigned long currentTime = millis();
      // if (currentTime - startTime >= SENSOR_CHECK_INTERVAL) {
      //   checkSensors();
      //   lastSensorCheckTime = millis();
      // } else if (currentTime - lastErrorCheckTime >= ERROR_CHECK_INTERVAL) {
      //   checkForError();
      //   lastErrorCheckTime = millis();
      // }
      // break;

    case ERROR:
      if (hasError()) {
        static String error;
        static unsigned long shiftTime;
        static bool firstRun = true;

        if (firstRun) {
          error = write_error(0);
          lcd.backlight();
          lcdOn = true;
          errorStartTime = millis();
          shiftTime = millis();
          firstRun = false;
        }

        digitalWrite(LED, ledOn ? LOW : HIGH);
        ledOn = !ledOn;

        if (millis() - errorStartTime >= 5000 && lcdOn) {
          lcd.noBacklight();
          lcdOn = false;
        }

        if (millis() - shiftTime >= 750 && error.length() > 0) {
          shift_text(&lcd, error, 0);
          shiftTime = millis();
        }

        if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) {
          if (!lcdOn) {
            lcd.backlight();
            lcdOn = true;
            errorStartTime = millis();
          } else {
            resetSensorValues();
            currentState = SETUP; // CHANGE TO IDLE
            firstRun = true;
            lcd.clear();
          }
        }

        if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) {
          tempLowError = true; // THIS IS HARDCODED FOR TESTING
          lcd.backlight();
          lcdOn = true;
          firstRun = true;
          checkForError();
          lcd.clear();
        }
      } else {
        currentState = SETUP; // CHANGE TO IDLE
      }
      break;
  }
}
