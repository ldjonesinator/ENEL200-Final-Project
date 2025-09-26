#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "lcd.h"
#include "button.h"
#include "led.h"

#define LED 4
#define LEFT_BUTTON_PIN 3 // change the pin to whatever the schematic is
#define RIGHT_BUTTON_PIN 2 // change the pin to whatever the schematic is

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

// tracks whether there are any errors
bool moistureError = false;
bool lightError = false;
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
    if (digitalRead(LEFT_BUTTON_PIN) == HIGH && !leftBut.pressed) { // when you select the option
      update_button("L", HIGH);
    } else if (digitalRead(LEFT_BUTTON_PIN) == LOW && leftBut.pressed) {
      update_button("L", LOW);
      return currentLevel;
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

  bool checkMoisture = avgMoisture > moistureRange[moistureLevel] && avgMoisture < moistureRange[moistureLevel + 1];
  bool checkLight = avgLight > lightRange[lightLevel] && avgLight < lightRange[lightLevel + 1];
  bool checkTemperature = avgTemperature > temperatureRange[temperatureLevel] && avgTemperature < temperatureRange[temperatureLevel + 1];

  if (!checkMoisture) {
    moistureError = true;
    currentState = ERROR;
  }

  if (!checkLight) {
    lightError = true;
    currentState = ERROR;
  }

  if (!checkTemperature) {
    temperatureError = true;
    currentState = ERROR;
  }
}

void setup() {
  Serial.begin(9600);

  lcd_setup(&lcd, true);
  pinMode(LED, OUTPUT);

  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  initialise_button(&leftBut, "L");
  initialise_button(&rightBut, "R");

  currentState = SETUP;
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

      int row = 0;
      if (moistureError) {
        lcd_write(&lcd, "Moisture Error!", row);
        row++;
      }

      if (lightError) {
        lcd_write(&lcd, "Light Error!", row);
        row++;
      }

      if (temperatureError) {
        lcd_write(&lcd, "Temperature Error!", row);
        row++;
      }
      break;
  }
}
