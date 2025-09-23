#ifndef BUTTON_H
#define BUTTON_H

//#include ----

typedef enum {
    NONE,
    LEFT,
    RIGHT,
    BOTH
} Clicked;

typedef struct {
    String name;                                                            // Left or right
    bool pressed;
    uint64_t time;                                                          // time button is pressed for
} Button;


Button initButton(String name);                                             // initalises button

void updateButton();                                            // ????

Clicked buttonsPressed(Button leftButt, Button rightButt)                   // checks what button/s are being clicked

void doClick(Clicked buttClicked);                                          // enum thing from buttonPressed

#endif //BUTTON_H
