#ifndef ERROR_H
#define ERROR_H

#include <Arduino.h>
#include "state.h"

#define LIGHT A0 // light sensor
#define MOISTURE A1 // moisture sensor
#define TEMP A2 // temperature sensor

enum Level {
    LOWER,
    MEDIUM,
    HIGHER
};

// user level names
constexpr char* levelNames[] = {"Low", "Medium", "High"}; // level names
constexpr int numLevels = sizeof(levelNames) / sizeof(levelNames[0]); // number of levels

// bounds for error checking
constexpr float moistureBounds[] = {525, 490, 370, 273}; // air -> moist -> water
constexpr float lightBounds[] = {1000, 170, 60, 0}; // low -> medium -> high
constexpr float tempBounds[] = {8500, 9491, 10205, 11000}; // low (10-18 degrees) -> medium (18-22 degrees) -> high (22-30 degrees)

extern Level moistureLevel;
extern Level lightLevel;
extern Level tempLevel;

extern bool moistureLowError;
extern bool moistureHighError;
extern bool lightLowError;
extern bool lightHighError;
extern bool tempLowError;
extern bool tempHighError;
extern int numErrors;

unsigned long adcToResistance(int adc_value);
float takeResistanceSamples(const int tempPin, const int sample_size);
bool hasError(); // return true if there's any error
void checkSensors(); // add current sensor readings to running totals
void resetSensorValues(); // reset all sensor data
void clearErrorFlags(); // reset all error flags
void checkForError(); // check averages and set error flags
void checkSensorsAndErrors(); // check sensors and check for error if it's time to do so

#endif // ERROR_H