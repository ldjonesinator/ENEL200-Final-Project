#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

void lcd_setup(const hd44780_I2Cexp* lcd, bool backlight);
bool lcd_write(const hd44780_I2Cexp* lcd, String text, int line);
bool shift_text(const hd44780_I2Cexp* lcd, String text, int line);

#endif // LCD_H
