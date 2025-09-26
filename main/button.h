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
    uint64_t startTime;
} Button;


void initialise_button(Button* button, String label);
void update_button(String label, bool isPressed);

#endif //BUTTON_H
