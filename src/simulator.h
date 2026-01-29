#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <Arduino.h>
#include "config.h"

struct SimulatedValues {
    // AC Output
    float acPower;          // Watts
    float acCurrentTotal;   // Amps
    float acCurrentA;       // Phase A current
    float acCurrentB;       // Phase B current
    float acCurrentC;       // Phase C current
    float acVoltageA;       // Phase A voltage (phase-neutral)
    float acVoltageB;       // Phase B voltage
    float acVoltageC;       // Phase C voltage
    float lineVoltageAB;    // Line voltage AB
    float lineVoltageBC;    // Line voltage BC
    float lineVoltageCA;    // Line voltage CA
    float frequency;        // Hz
    float powerFactor;      // 0-1
    float apparentPower;    // VA
    float reactivePower;    // VAr
    uint32_t totalEnergy;   // Wh (accumulated)

    // DC Input
    float dcVoltage;        // V
    float dcCurrent;        // A
    float dcPower;          // W

    // Other
    int16_t temperature;    // Cabinet temp in °C
    bool isProducing;       // True if generating power
};

class Simulator {
public:
    Simulator();

    // Initialize simulator
    void begin();

    // Update simulated values (call periodically)
    void update();

    // Get current simulated values
    const SimulatedValues& getValues() const;

private:
    SimulatedValues _values;
    unsigned long _startTime;
    unsigned long _lastUpdate;
    uint32_t _accumulatedEnergy;  // Wh accumulated since start

    // Calculate power based on simulated solar curve
    float calculatePower();

    // Add small random variation
    float addNoise(float value, float maxNoise);
};

#endif // SIMULATOR_H
