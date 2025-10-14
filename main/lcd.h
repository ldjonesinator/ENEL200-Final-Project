#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

extern hd44780_I2Cexp lcd; // LCD object

extern bool lcdOn; // Tracks whether the LCD is on

void lcd_setup(const hd44780_I2Cexp* lcd, bool backlight); // Returns an LCD variable that is set up for use
bool lcd_write(const hd44780_I2Cexp* lcd, String text, int line); // Returns if the line is too long and puts up to the first 16 characters of a string on an LCD
bool shift_text(const hd44780_I2Cexp* lcd, String text, int line); // Shifts text on a line in the LCD
String build_error(); // Build error message string
String write_error(int line); // Builds the error message and writes to lCD

#endif // LCD_H