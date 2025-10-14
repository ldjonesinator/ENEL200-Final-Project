#ifndef ERROR_H
#define ERROR_H

#include <Arduino.h>
#include "state.h"

// Pins for each sensor
#define LIGHT A0 // Light sensor
#define MOISTURE A1 // Moisture sensor
#define TEMP A2 // Temperature sensor

// Levels for sensors
typedef enum {
    LOWER,
    MEDIUM,
    HIGHER
} Level;

// Level names and the number of levels
constexpr char* levelNames[] = {"Low", "Medium", "High"}; // Level names
constexpr int numLevels = sizeof(levelNames) / sizeof(levelNames[0]); // Number of levels

// Sensor value bounds/ranges
constexpr float moistureBounds[] = {525, 490, 370, 273}; // Air -> Moist -> Water
constexpr float lightBounds[] = {1000, 170, 60, 0}; // Low -> Medium -> High
constexpr float tempBounds[] = {8500, 9491, 10205, 11000}; // Low (10-18 degrees) -> Medium (18-22 degrees) -> High (22-30 degrees)

// User chosen level for each sensor
extern Level moistureLevel;
extern Level lightLevel;
extern Level tempLevel;

// Error flags
extern bool moistureLowError;
extern bool moistureHighError;
extern bool lightLowError;
extern bool lightHighError;
extern bool tempLowError;
extern bool tempHighError;

// Number of error flags
extern int numErrors;

unsigned long adcToResistance(int adc_value); // Return resistance given ADC voltage
bool hasError(); // Return true if there's any error
void checkSensors(); // Add current sensor readings to running totals
void resetSensorValues(); // Reset all sensor data
void clearErrorFlags(); // Reset all error flags
void checkForError(); // Check averages and set error flags
void checkSensorsAndErrors(); // Check sensors and check for error if it's time to do so

#endif // ERROR_H