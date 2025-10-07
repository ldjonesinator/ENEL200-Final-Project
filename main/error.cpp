#include "error.h"
#include "time.h"

const int VDD = 5;
const int ADC_RESOLUTION = 1024;
const int RESISTOR = 10000; // 10kOhms

// user selected levels (moisture, light, temp)
Level plantLevels[3];

// measurement tracking
int numMeasurements = 0; // counts the number of measurements (used for averaging)
float moisture = 0; // running sums for sensor readings
float light = 0;
float temp = 0;

// error flags
bool moistureLowError = false;
bool moistureHighError = false;
bool lightLowError = false;
bool lightHighError = false;
bool tempLowError = false;
bool tempHighError = false;

unsigned long adcToResistance(int adc_value) {
    float voltage = VDD * (float)adc_value / ADC_RESOLUTION;
    return (long)(voltage * RESISTOR / (VDD - voltage));
}

float takeResistanceSamples(const int tempPin, const int sample_size) {
    static float total = 0;
    static int sample_num = 0;
    total += adcToResistance(analogRead(tempPin));
    sample_num ++;
    if (sample_num == sample_size) {
        return total / sample_size;
    } else {
        return 0;
    }
}

bool hasError()
{
    return moistureLowError || moistureHighError || lightLowError || lightHighError || tempLowError || tempHighError;
}

void checkSensors()
{
    moisture += analogRead(MOISTURE);
    light += analogRead(LIGHT);
    temp += analogRead(TEMP);
    numMeasurements++;
}

void resetSensorValues()
{
    moisture = 0;
    light = 0;
    temp = 0;
    numMeasurements = 0;
}

void clearErrorFlags()
{
    moistureLowError = false;
    moistureHighError = false;
    lightLowError = false;
    lightHighError = false;
    tempLowError = false;
    tempHighError = false;
}

void checkForError()
{
    clearErrorFlags();
    float avgMoisture = moisture / numMeasurements;
    float avgLight = light / numMeasurements;
    float avgTemp = temp / numMeasurements;

    // moisture bounds check
    if (avgMoisture > moistureBounds[plantLevels[0]]) {
            moistureLowError = true;
            currentState = ERROR;
    } else if (avgMoisture < moistureBounds[plantLevels[0] + 1]) {
        moistureHighError = true;
        currentState = ERROR;
    }

    // light bounds check
    if (avgLight > lightBounds[plantLevels[1]]) {
        lightLowError = true;
        currentState = ERROR;
    } else if (avgLight < lightBounds[plantLevels[1] + 1]) {
        lightHighError = true;
        currentState = ERROR;
    }

    // temp bounds check
    if (avgTemp > tempBounds[plantLevels[2]]) {
        tempLowError = true;
        currentState = ERROR;
    } else if (avgTemp < tempBounds[plantLevels[2] + 1]) {
        tempHighError = true;
        currentState = ERROR;
    }

    resetSensorValues();
}

void checkSensorsAndErrors()
{
    long currentSec = currentTimeInSeconds();

    if ((currentSec % SENSOR_CHECK_INTERVAL_SEC) == 0 && currentSec != lastSensorCheckSec) {
        checkSensors();
        lastSensorCheckSec = currentSec;
        Serial.println("Checking sensors at "+ String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    }

    if ((currentSec % ERROR_CHECK_INTERVAL_SEC) == 0 && currentSec != lastErrorCheckSec) {
        checkForError();
        lastErrorCheckSec = currentSec;
        Serial.println("Checking for error at "+ String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    }
}