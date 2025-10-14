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

extern State currentState; // Current montitor state

#endif // STATE_H