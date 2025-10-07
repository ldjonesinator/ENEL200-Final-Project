#ifndef STATE_H
#define STATE_H

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

#endif // STATE_H