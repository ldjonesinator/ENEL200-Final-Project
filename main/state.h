#ifndef MAIN_H
#define MAIN_H

typedef enum {
    SETUP,
    IDLE,
    ERROR
} State;

typedef enum {
    ERROR_SCROLL,
    ERROR_INSTANT_MEASUREMENT
} ErrorSubState;

extern State currentState; // current system state

#endif // MAIN_H