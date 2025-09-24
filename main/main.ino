#include "lcd.h"
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;

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

const char* levelNames[] = {"Low", "Medium", "High"};
const int numLevels = 3;

State currentState;
Level moistureLevel;
Level lightLevel;
Level temperatureLevel;

Level getLevel() {
  char choice;
  Level currentLevel = LOWER;

  //Serial.print(levelNames[currentLevel]);
  lcd_write(&lcd, levelNames[currentLevel], 1);

  while (1) {
    if (Serial.available()) {
      choice = Serial.read(); // this will eventually be a button that the user presses
      if (choice == 'y') { // 'y' means YES
        return currentLevel;
      } else if (choice == 'n') {
        currentLevel = (Level)(currentLevel + 1);
        if (currentLevel >= numLevels) {
          currentLevel = LOWER;
        }
        //Serial.print(levelNames[currentLevel]);
        lcd_write(&lcd, levelNames[currentLevel], 1);
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  lcd_setup(&lcd, true);
  currentState = SETUP;
}

void loop() {
  switch (currentState) {
    case SETUP:
      //Serial.println("How much soil moisture does your plant need?"); // these messages will eventually be displayed on the LCD
      lcd_write(&lcd, "Soil moisture?", 0);
      moistureLevel = getLevel();
      Serial.print("Soil moisture set to: ");
      Serial.println(levelNames[moistureLevel]);

      //Serial.println("How much light does your plant need?");
      lcd_write(&lcd, "Light level?", 0);
      lightLevel = getLevel();
      Serial.print("Light level set to: ");
      Serial.println(levelNames[lightLevel]);

      //Serial.println("How much warmth does your plant need?");
      lcd_write(&lcd, "Warmth?", 0);
      temperatureLevel = getLevel();
      Serial.print("Warmth set to: ");
      Serial.println(levelNames[temperatureLevel]);

      currentState = IDLE;
      break;

    case IDLE:
      // check sensors every 1 min
      // check for error every 30 mins
      // also check if the reset button has been pressed
      break;

    case ERROR:
      // tell user their plant is struggling
      break;
  }
}