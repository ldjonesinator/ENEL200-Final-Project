#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <stdint.h>

// Button pins
#define LEFT_BUTTON_PIN 2
#define RIGHT_BUTTON_PIN 3

// Button struct
typedef struct {
    String label;
    bool pressed;
    bool longPress;
    uint64_t startTime;
} Button;

// Objects for left and right buttons
extern Button leftBut;
extern Button rightBut;

void initialise_button(Button* button, String label); // Initialises a button in the default state
void update_button(String label, bool isPressed); // Updates the members of the button struct
bool isButClicked(Button* button, int buttonPin); // Returns whether a button has been pressed

#endif // BUTTON_H