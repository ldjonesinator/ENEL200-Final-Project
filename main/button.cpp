#include <stdint.h>

#include "button.h"

const uint8_t MAX_BUTTONS = 10;
static Button* allButtons[MAX_BUTTONS];
static uint8_t numButtons = 0;


static void register_button(Button* button) {
    if (numButtons < MAX_BUTTONS) { // don't have to worry about going over this amount
        allButtons[numButtons] = button;
        numButtons ++;
    }
}

static void set_timer_start(String label) {
    // updates the start time if the button is pressed
    size_t i = 0;
    bool isFound = false;
    while (i < numButtons && !isFound) { // assumes this label is unique and picks the first one found
        if (allButtons[i]->label == label) {
            allButtons[i]->startTime = millis();
            isFound = true;
        }
        i ++;
    }
}

void initialise_button(Button* button, String label) {
    // sets the button label, puts button in default state and registers it
    button->label = label;
    button->pressed = false;
    register_button(button);
}

void update_button(String label, bool isPressed) {
    // it will update the button's state and start time depending on if it's pressed
    size_t i = 0;
    bool isFound = false;
    while (i < numButtons && !isFound) { // assumes this label is unique and picks the first one found
        if (allButtons[i]->label == label) {
            if (isPressed) {
                set_timer_start(label);
            }
            allButtons[i]->pressed = isPressed;
            isFound = true;
        }
        i ++;
    }
}
