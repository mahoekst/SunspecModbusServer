#ifndef SUNSPEC_MODEL_H
#define SUNSPEC_MODEL_H

#include <Arduino.h>
#include "config.h"

// SunSpec Operating States (Model 103)
enum class InverterState : uint16_t {
    OFF = 1,
    SLEEPING = 2,
    STARTING = 3,
    MPPT = 4,
    THROTTLED = 5,
    SHUTTING_DOWN = 6,
    FAULT = 7,
    STANDBY = 8
};

// Model 103 register offsets (relative to Model 103 data start at offset 71)
namespace Model103 {
    const uint8_t A = 0;        // AC Total Current
    const uint8_t AphA = 1;     // Phase A Current
    const uint8_t AphB = 2;     // Phase B Current
    const uint8_t AphC = 3;     // Phase C Current
    const uint8_t A_SF = 4;     // Current Scale Factor
    const uint8_t PPVphAB = 5;  // Phase AB Voltage
    const uint8_t PPVphBC = 6;  // Phase BC Voltage
    const uint8_t PPVphCA = 7;  // Phase CA Voltage
    const uint8_t PhVphA = 8;   // Phase A Voltage
    const uint8_t PhVphB = 9;   // Phase B Voltage
    const uint8_t PhVphC = 10;  // Phase C Voltage
    const uint8_t V_SF = 11;    // Voltage Scale Factor
    const uint8_t W = 12;       // AC Power
    const uint8_t W_SF = 13;    // Power Scale Factor
    const uint8_t Hz = 14;      // Frequency
    const uint8_t Hz_SF = 15;   // Frequency Scale Factor
    const uint8_t VA = 16;      // Apparent Power
    const uint8_t VA_SF = 17;   // VA Scale Factor
    const uint8_t VAr = 18;     // Reactive Power
    const uint8_t VAr_SF = 19;  // VAr Scale Factor
    const uint8_t PF = 20;      // Power Factor
    const uint8_t PF_SF = 21;   // PF Scale Factor
    const uint8_t WH_HI = 22;   // Energy High Word
    const uint8_t WH_LO = 23;   // Energy Low Word
    const uint8_t WH_SF = 24;   // Energy Scale Factor
    const uint8_t DCA = 25;     // DC Current
    const uint8_t DCA_SF = 26;  // DC Current SF
    const uint8_t DCV = 27;     // DC Voltage
    const uint8_t DCV_SF = 28;  // DC Voltage SF
    const uint8_t DCW = 29;     // DC Power
    const uint8_t DCW_SF = 30;  // DC Power SF
    const uint8_t TmpCab = 31;  // Cabinet Temperature
    const uint8_t TmpSnk = 32;  // Heat Sink Temperature
    const uint8_t TmpTrns = 33; // Transformer Temperature
    const uint8_t TmpOt = 34;   // Other Temperature
    const uint8_t Tmp_SF = 35;  // Temperature Scale Factor
    const uint8_t St = 36;      // Operating State
    const uint8_t StVnd = 37;   // Vendor Operating State
}

class SunSpecModel {
public:
    SunSpecModel();

    // Initialize register map with static data
    void begin();

    // Get pointer to register array (for Modbus mapping)
    uint16_t* getRegisters();

    // Get total number of registers
    uint16_t getRegisterCount();

    // Update Model 103 dynamic values
    void setACPower(int16_t watts);
    void setACCurrent(float totalAmps, float phaseA, float phaseB, float phaseC);
    void setACVoltage(float phaseA, float phaseB, float phaseC);
    void setLineVoltage(float ab, float bc, float ca);
    void setFrequency(float hz);
    void setPowerFactor(float pf);
    void setApparentPower(int16_t va);
    void setReactivePower(int16_t var);
    void setEnergy(uint32_t wh);
    void setDCValues(float voltage, float current, int16_t power);
    void setTemperature(int16_t cabinetTemp);
    void setOperatingState(InverterState state);

private:
    uint16_t _registers[TOTAL_REGISTERS];

    // Helper to write string to register array (padded with spaces)
    void writeString(uint16_t offset, const char* str, uint16_t maxLen);

    // Helper to write 32-bit value (high word first)
    void writeUint32(uint16_t offset, uint32_t value);
};

#endif // SUNSPEC_MODEL_H
