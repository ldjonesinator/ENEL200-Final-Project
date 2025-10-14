#include "button.h"

const uint8_t MAX_BUTTONS = 10;
static Button* allButtons[MAX_BUTTONS];
static uint8_t numButtons = 0;

Button leftBut;
Button rightBut;

static void register_button(Button* button)
{
    if (numButtons < MAX_BUTTONS) { // Don't have to worry about going over this amount
        allButtons[numButtons] = button;
        numButtons ++;
    }
}

static void set_timer_start(String label)
{
    // Updates the start time if the button is pressed
    size_t i = 0;
    bool isFound = false;
    while (i < numButtons && !isFound) { // Assumes this label is unique and picks the first one found
        if (allButtons[i]->label == label) {
            allButtons[i]->startTime = millis();
            isFound = true;
        }
        i ++;
    }
}

void initialise_button(Button* button, String label)
{
    // Sets the button label, puts button in default state and registers it
    button->label = label;
    button->pressed = false;
    button->longPress = false;
    register_button(button);
}

void update_button(String label, bool isPressed)
{
    // Update the button state and start time depending on if it's pressed
    size_t i = 0;
    bool isFound = false;
    while (i < numButtons && !isFound) { // Assumes this label is unique and picks the first one found
        if (allButtons[i]->label == label) {
            set_timer_start(label);
            allButtons[i]->pressed = isPressed;
            isFound = true;
        }
        i ++;
    }
}

bool isButClicked(Button* button, int buttonPin)
{
    if (millis() - button->startTime > 100) { // 100 ms to account for bouncing
        if (digitalRead(buttonPin) == HIGH && !button->pressed) {
            update_button(button->label, HIGH);
        } else if (digitalRead(buttonPin) == LOW && button->pressed) {
            update_button(button->label, LOW);
            return true;
        }
    }
    return false;
}