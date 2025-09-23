#include "button.h"

Button initButton(name)                                                     // initalises button
{
    Button button {
        name,
        false,
        0                                                     // ????
    };
    return button;
}

void updateButton()
{
    // update buton when it is pressed (goes HIGH)
    // maybe call buttonsPressed?
}

Clicked buttonsPressed(leftButt, rightButt)                                 // checks what button/s are being clicked
{
    // due to coding, theres 100ms 'delay' between button clicked and anything updating (LCD)
    // button will be either HIGH or LOW
    // if button is just pressed - start timer

    Clicked buttCicked;

    static uint64_t newTime = millis();                                     // gets the time - does this have to be static?
    static bool leftPressed;                                                // do these have to be static?
    static bool rightPressed;
    
    if ((leftButt.pressed || rightButt.pressed) && !isButtonPressed) {
        static uint64_t startTime = newTime;                                // starts the timer - does this have to be static?
        isButtonPressed = true;
        if (leftButt.pressed) {
            leftPressed = true;
            rightPressed = false;
        } else {
            leftPressed = false;
            rightPressed = true;
        }
    }

    timepassed = newTime - startTime
    if (timepassed < 100) {
        // check if the other button has been pressed
        if (leftPressed) {
            if (rightButt.pressed) {
                rightPressed = true;
            }
        } else if (rightPressed) {
            if (leftButt.pressed) {
                leftPressed = true;
            }
        }
    }



    if (timepassed > 100) {
        // update what button/s have been clicked
        isButtonPressed = false;
        if (leftPressed && rightPressed) {
            buttClicked = BOTH;
        } else if (leftPressed && !rightPressed) {
            buttClicked = LEFT;
        } else if (rightPressed && !leftPressed) {
            buttClicked = RIGHT;
        }
    }

    // not sure if i should include these next 2 lines....
    leftButt.pressed = false;
    rightButt.pressed = false;

    return buttClicked;
}


void doClick(buttClicked)                                                   // enum thing from buttonPressed
// use buttonPressed to update the LCD
{
    bool isError = checkForError();                                         // should be a global variable? - multiple places/modules will call it
    switch (buttClicked) {
        case 0:                                                             // NONE
            break;                                                          // nothing is changed
        case 1:                                                             // LEFT
            if (LCD == OFF) {
                LCD = ON;
            } else {
                isError = False;                                            // update so there is no error
                updateLED(ledPin);
            }
            break;
        case 2:                                                             // RIGHT
            isError = checkForError();                                      // rechecks if there is an error
            updateLED(ledPin);
            // call other error function maybe???           // ????
        case 3:                                                             // BOTH
            reset();                                                        // calls function to reset the system
    }
}

