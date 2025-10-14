#include "lcd.h"
#include "error.h"

const int LCD_COLS = 16;
const int LCD_ROWS = 2;
static int stringIndexes[LCD_ROWS];

hd44780_I2Cexp lcd;
bool lcdOn = false;

void lcd_setup(const hd44780_I2Cexp* lcd, bool backlight) {
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
    bool islong;
    lcd->setCursor(0, line);
    lcd->print("                ");
    if (text.length() > LCD_COLS) { // Line too long
        lcd->setCursor(0, line);
        lcd->print(text.substring(0, 16));
        islong = true;
    } else {
        lcd->setCursor((LCD_COLS - text.length()) / 2, line); // Spacing to center the text
        lcd->print(text);
        islong = false;
    }
    return islong;
}

bool shift_text(const hd44780_I2Cexp* lcd, String text, int line) {
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

String build_error()
{
    String error = "";
    int numErrors = 0;

    if (moistureLowError) {
        error += "Water me!";
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

String write_error(int line)
{
    String error = build_error();

    if (lcd_write(&lcd, error, line)) {
        return error;
    }
    return "";
}