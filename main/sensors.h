#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "state.h"

// sensor pins
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
constexpr float lightBounds[] = {1000, 275, 60, 0}; // low -> medium -> high
constexpr float tempBounds[] = {1000, 187, 175, 0}; // low (10-20 degrees) -> medium (20-25 degrees) -> high (25-30 degrees)

extern Level plantLevels[3];
// extern Level moistureLevel;
// extern Level lightLevel;
// extern Level tempLevel;

extern bool moistureLowError;
extern bool moistureHighError;
extern bool lightLowError;
extern bool lightHighError;
extern bool tempLowError;
extern bool tempHighError;

unsigned long adcToResistance(int adc_value);
float takeResistanceSamples(const int tempPin, const int sample_size);
bool hasError(); // return true if there's any error
void checkSensors(); // add current sensor readings to running totals
void resetSensorValues(); // reset all sensor data
void clearErrorFlags(); // reset all error flags
void checkForError(); // check averages and set error flags
void checkSensorsAndErrors(); // check sensors and check for error if it's time to do so

#endif // SENSORS_H