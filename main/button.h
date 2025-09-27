#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <stdint.h>

// typedef enum {
//     NONE,
//     LEFT,
//     RIGHT,
//     BOTH
// } Clicked;

typedef struct {
    String label;
    bool pressed;
    bool longPress;
    uint64_t startTime;
} Button;


void initialise_button(Button* button, String label);
void update_button(String label, bool isPressed);
void update_long_presses();
bool isButClicked(Button* button, int buttonPin);

#endif //BUTTON_H
