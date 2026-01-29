#include "simulator.h"
#include <math.h>

Simulator::Simulator() : _startTime(0), _lastUpdate(0), _accumulatedEnergy(0) {
    memset(&_values, 0, sizeof(_values));
}

void Simulator::begin() {
    _startTime = millis();
    _lastUpdate = _startTime;
    _accumulatedEnergy = 0;

    // Initialize static values
    _values.acVoltageA = GRID_VOLTAGE;
    _values.acVoltageB = GRID_VOLTAGE;
    _values.acVoltageC = GRID_VOLTAGE;
    _values.lineVoltageAB = GRID_VOLTAGE * 1.732f;  // sqrt(3)
    _values.lineVoltageBC = GRID_VOLTAGE * 1.732f;
    _values.lineVoltageCA = GRID_VOLTAGE * 1.732f;
    _values.frequency = GRID_FREQUENCY;
    _values.powerFactor = POWER_FACTOR;
    _values.temperature = 35;  // Typical operating temp

    DEBUG_SERIAL.println("Simulator: Initialized");
}

void Simulator::update() {
    unsigned long now = millis();
    unsigned long deltaMs = now - _lastUpdate;

    if (deltaMs < SIMULATION_UPDATE_MS) {
        return;  // Not time to update yet
    }

    _lastUpdate = now;

    // Calculate power based on time (simulated solar curve)
    _values.acPower = calculatePower();
    _values.isProducing = (_values.acPower > 0);

    // Add noise to voltage readings
    _values.acVoltageA = addNoise(GRID_VOLTAGE, 5.0f);
    _values.acVoltageB = addNoise(GRID_VOLTAGE, 5.0f);
    _values.acVoltageC = addNoise(GRID_VOLTAGE, 5.0f);

    // Line voltages (phase-to-phase) = phase voltage * sqrt(3)
    _values.lineVoltageAB = addNoise(GRID_VOLTAGE * 1.732f, 8.0f);
    _values.lineVoltageBC = addNoise(GRID_VOLTAGE * 1.732f, 8.0f);
    _values.lineVoltageCA = addNoise(GRID_VOLTAGE * 1.732f, 8.0f);

    // Frequency with small variation
    _values.frequency = addNoise(GRID_FREQUENCY, 0.1f);

    // Calculate currents from power (balanced three-phase)
    // P = sqrt(3) * V_line * I * PF  =>  I = P / (sqrt(3) * V_line * PF)
    if (_values.acPower > 0) {
        float avgLineVoltage = (_values.lineVoltageAB + _values.lineVoltageBC + _values.lineVoltageCA) / 3.0f;
        _values.acCurrentTotal = _values.acPower / (1.732f * avgLineVoltage * _values.powerFactor);

        // Phase currents (slightly unbalanced for realism)
        float phaseCurrent = _values.acCurrentTotal / 3.0f;
        _values.acCurrentA = addNoise(phaseCurrent, phaseCurrent * 0.02f);
        _values.acCurrentB = addNoise(phaseCurrent, phaseCurrent * 0.02f);
        _values.acCurrentC = addNoise(phaseCurrent, phaseCurrent * 0.02f);
        _values.acCurrentTotal = _values.acCurrentA + _values.acCurrentB + _values.acCurrentC;
    } else {
        _values.acCurrentTotal = 0;
        _values.acCurrentA = 0;
        _values.acCurrentB = 0;
        _values.acCurrentC = 0;
    }

    // Power factor with small variation
    _values.powerFactor = addNoise(POWER_FACTOR, 0.01f);
    if (_values.powerFactor > 1.0f) _values.powerFactor = 1.0f;
    if (_values.powerFactor < 0.95f) _values.powerFactor = 0.95f;

    // Apparent power (VA) = P / PF
    _values.apparentPower = _values.acPower / _values.powerFactor;

    // Reactive power (VAr) = sqrt(VA^2 - W^2)
    float vaSquared = _values.apparentPower * _values.apparentPower;
    float wSquared = _values.acPower * _values.acPower;
    _values.reactivePower = sqrt(vaSquared - wSquared);

    // DC side calculations
    // Typical inverter efficiency ~97%
    float inverterEfficiency = 0.97f;
    _values.dcPower = _values.acPower / inverterEfficiency;

    // Typical string voltage for 9kW inverter: 300-600V DC
    // Simulate mid-range voltage
    _values.dcVoltage = addNoise(450.0f, 20.0f);

    // DC current from power and voltage
    if (_values.dcVoltage > 0 && _values.dcPower > 0) {
        _values.dcCurrent = _values.dcPower / _values.dcVoltage;
    } else {
        _values.dcCurrent = 0;
    }

    // Temperature varies slightly with power
    float tempRise = (_values.acPower / INVERTER_MAX_POWER) * 15.0f;  // Up to 15°C rise
    _values.temperature = (int16_t)(25 + tempRise + addNoise(0, 2.0f));

    // Accumulate energy (Wh)
    // deltaMs is in milliseconds, power is in watts
    // Energy (Wh) = Power (W) * time (h) = Power * deltaMs / 3600000
    float energyIncrement = _values.acPower * (float)deltaMs / 3600000.0f;
    _accumulatedEnergy += (uint32_t)energyIncrement;
    _values.totalEnergy = _accumulatedEnergy;
}

const SimulatedValues& Simulator::getValues() const {
    return _values;
}

float Simulator::calculatePower() {
    // Simulate a solar curve using time since start
    // Use a sine wave that completes one cycle every 60 seconds for testing
    // In real deployment, this would be based on actual time of day

    unsigned long elapsedMs = millis() - _startTime;
    float elapsedSeconds = elapsedMs / 1000.0f;

    // Complete one "day" cycle every 60 seconds for easy testing
    float cycleSeconds = 60.0f;
    float phase = (elapsedSeconds / cycleSeconds) * 2.0f * PI;

    // Sine wave from 0 to 1 (shifted to only positive values)
    float solarFactor = (sin(phase - PI/2) + 1.0f) / 2.0f;

    // Scale to max power with some noise
    float power = solarFactor * INVERTER_MAX_POWER;
    power = addNoise(power, power * 0.02f);  // 2% noise

    // Clamp to valid range
    if (power < 50) power = 0;  // Below threshold, inverter would be off
    if (power > INVERTER_MAX_POWER) power = INVERTER_MAX_POWER;

    return power;
}

float Simulator::addNoise(float value, float maxNoise) {
    if (maxNoise <= 0) return value;

    // Simple random noise
    float noise = ((float)random(-1000, 1001) / 1000.0f) * maxNoise;
    return value + noise;
}
