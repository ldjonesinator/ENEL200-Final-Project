#include "lcd.h"
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>



const int LCD_COLS = 16;
const int LCD_ROWS = 2;
static int stringIndexes[LCD_ROWS];



void lcd_setup(const hd44780_I2Cexp* lcd, bool backlight) {
  // returns an lcd variable that is set up for use
    int status = lcd->begin(LCD_COLS, LCD_ROWS);
    if (status != 0) {
        Serial.println("There was an error with the LCD");
        hd44780::fatalError(status);
    }
    lcd->clear();
    if (backlight) {
        lcd->backlight();
    } else {
        lcd->noBacklight();
    }
}

bool lcd_write(const hd44780_I2Cexp* lcd, String text, int line) {
  // returns if the line is too long and puts up to the first 16 characters of a string on an LCD
    bool islong;
    lcd->setCursor(0, line);
    lcd->print("                ");
    if (text.length() > LCD_COLS) { // line too long
        lcd->setCursor(0, line);
        lcd->print(text.substring(0, 16));
        islong = true;
    } else {
        lcd->setCursor((LCD_COLS - text.length()) / 2, line); // spacing to center the text
        lcd->print(text);
        islong = false;
    }
    return islong;
}

bool shift_text(const hd44780_I2Cexp* lcd, String text, int line) {
    // shifts text on a line in the LCD
    bool isDoneShift = false;
    if (stringIndexes[line] == text.length() - 17) {
        isDoneShift = true;
    }

    if (stringIndexes[line] >= text.length() - 16) {
        lcd_write(lcd, text, line);
        stringIndexes[line] = 0;
    } else {
        lcd_write(lcd, text.substring(stringIndexes[line] + 1, text.length()), line);
    }

    stringIndexes[line] ++;
    return isDoneShift;
}
