#include "sensors.h"

const int VDD = 5;
const int ADC_RESOLUTION = 1024;
const int RESISTOR = 10000; // 10kOhms

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

