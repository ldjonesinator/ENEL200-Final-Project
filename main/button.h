#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <stdint.h>

#define LEFT_BUTTON_PIN 2 // pin for left button
#define RIGHT_BUTTON_PIN 3 // pin for right button

typedef struct {
    String label;
    bool pressed;
    bool longPress;
    uint64_t startTime;
} Button;

extern Button leftBut; // left button object
extern Button rightBut; // right button object

void initialise_button(Button* button, String label, int pin);
void update_button(String label, bool isPressed);
void update_long_presses();
bool isButClicked(Button* button, int buttonPin);

#endif // BUTTON_H