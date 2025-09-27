#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "lcd.h"
#include "button.h"
#include "led.h"

#define LED 4
#define LEFT_BUTTON_PIN 3 // change the pin to whatever the schematic is
#define RIGHT_BUTTON_PIN 2 // change the pin to whatever the schematic is

// sensor pins
#define LIGHT A0
#define MOISTURE A1
#define TEMPERATURE A2

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

typedef enum {
  DAY,
  NIGHT
} Mode;

const char* levelNames[] = {"Low", "Medium", "High"};
const int numLevels = sizeof(levelNames) / sizeof(levelNames[0]);;

const float moistureRange[] = {0, 1, 2, 3}; // update according to testing/datasheet of components
const float lightRange[] = {0, 1, 2, 3};
const float temperatureRange[] = {0, 1, 2, 3};

State currentState;

Level moistureLevel;
Level lightLevel;
Level temperatureLevel;

Mode currentMode;

const unsigned long SENSOR_CHECK_INTERVAL = 60000; // how often to check the sensors (60000 milliseconds = 1 minute)
const unsigned long ERROR_CHECK_INTERVAL = 1800000; // how often to take the average sensor values (1800000 milliseconds = 30 minutes)

const unsigned long NUM_MEASUREMENTS = ERROR_CHECK_INTERVAL / SENSOR_CHECK_INTERVAL; // number of measurements made before checking for an error

unsigned long lastSensorCheckTime;
unsigned long lastErrorCheckTime;

float moisture = 0;
float light = 0;
float temperature = 0;

// tracks what errors have occurred
bool moistureLowError = true;
bool moistureHighError = false;
bool lightLowError = true;
bool lightHighError = false;
bool temperatureLowError = true;
bool temperatureError = false;

// resets the entire system
void reset()
{
  currentState = SETUP;
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
  temperature += analogRead(TEMPERATURE);
}

void checkForError() {
  float avgMoisture = moisture / NUM_MEASUREMENTS;
  float avgLight = light / NUM_MEASUREMENTS;
  float avgTemperature = temperature / NUM_MEASUREMENTS;

  bool checkMoistureLow = avgMoisture < moistureRange[moistureLevel];
  bool checkMoistureHigh = avgMoisture > moistureRange[moistureLevel + 1];
  bool checkLightLow = avgLight < lightRange[lightLevel];
  bool checkLightHigh = avgLight > lightRange[lightLevel + 1];
  bool checkTemperatureLow = avgTemperature < temperatureRange[temperatureLevel];
  bool checkTemperatureHigh = avgTemperature > temperatureRange[temperatureLevel + 1];

  if (checkMoistureLow) {
        moistureLowError = true;
        currentState = ERROR;
  } else if (checkMoistureHigh) {
      moistureHighError = true;
      currentState = ERROR;
  }

  if (checkLightLow) {
      lightLowError = true;
      currentState = ERROR;
  } else if (checkLightHigh) {
      lightHighError = true;
      currentState = ERROR;
  }

  if (checkTempLow) {
      temperatureLowError = true;
      currentState = ERROR;
  } else if (checkTempHigh) {
      temperatureHighError = true;
      currentState = ERROR;
  }
}

String write_error(int line) {
  String error = "";
  int numErrors = 0;
  if (moistureLowError) {
    error += "Low Moisture";
    numErrors ++;
  } else if (moistureHighError) {
    error += "High Moisture";
    numErrors++;
  }

  if (lightLowError) {
    if (numErrors > 0) {
      error += ", ";
    }
    error += "Low Light";
    numErrors ++;
  } else if (lightHighError) {
    error += "High Light";
    numErrors++;
  }

  if (temperatureLowError) {
    if (numErrors > 0) {
      error += ", ";
    }
    error += "Low Temperature";
    numErrors ++;
  } else if (temperatureHighError) {
    error += "High Temperature";
    numErrors++;
  }

  if (numErrors > 1) {
    error += " Errors!";
  } else {
    error += " Error!";
  }
  if (lcd_write(&lcd, error, line)) {
    return error;
  }
  return "";
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
}

void loop() {
  // TO DO RIGHT HERE AT THE START OF LOOP()!
  // if it's day, switch current time to DAY
  // if the reset button is pressed, call reset()
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
      temperatureLevel = getLevel();
      Serial.print("Warmth set to: ");
      Serial.println(levelNames[temperatureLevel]);

      currentState = IDLE;
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
      digitalWrite(LED, HIGH);
      String error = write_error(0);
      bool isDone = false;
      if (error.length() > 0) { // only happens if the line is too long
        unsigned long shiftTime = millis();
        while (!isDone) {
          if (millis() - shiftTime >= 750) {
            shift_text(&lcd, error, 0);
            shiftTime = millis();
          }
          if (isButClicked(&leftBut, LEFT_BUTTON_PIN)) { // CHANGE THESE LATER TO WHAT THEY ARE SUPPOSED TO DO
            currentState = SETUP;
            lcd.clear();
            isDone = true;
          }
          if (isButClicked(&rightBut, RIGHT_BUTTON_PIN)) { // CHANGE THESE LATER TO WHAT THEY ARE SUPPOSED TO DO
            currentState = SETUP;
            lcd.clear();
            isDone = true;
          }
        }
      }
      break;
  }
}
