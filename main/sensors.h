#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

unsigned long adcToResistance(int adc_value);
float takeResistanceSamples(const int tempPin, const int sample_size);


#endif // SENSORS_H