#include "sunspec_model.h"
#include <cstring>

SunSpecModel::SunSpecModel() {
    memset(_registers, 0, sizeof(_registers));
}

void SunSpecModel::begin() {
    // SunSpec ID "SunS" = 0x53756E53
    _registers[SUNSPEC_ID_OFFSET] = 0x5375;     // "Su"
    _registers[SUNSPEC_ID_OFFSET + 1] = 0x6E53; // "nS"

    // Model 1 (Common) Header
    _registers[MODEL1_ID_OFFSET] = 1;           // Model ID
    _registers[MODEL1_LENGTH_OFFSET] = MODEL1_LENGTH;  // Length

    // Model 1 Data - Manufacturer (16 registers = 32 chars)
    writeString(MODEL1_DATA_OFFSET, "Growatt", 32);

    // Model 1 Data - Model (16 registers = 32 chars)
    writeString(MODEL1_DATA_OFFSET + 16, "9000 TL3-S", 32);

    // Model 1 Data - Options (8 registers = 16 chars)
    writeString(MODEL1_DATA_OFFSET + 32, "", 16);

    // Model 1 Data - Version (8 registers = 16 chars)
    writeString(MODEL1_DATA_OFFSET + 40, "1.0.0", 16);

    // Model 1 Data - Serial Number (16 registers = 32 chars)
    writeString(MODEL1_DATA_OFFSET + 48, "EMULATED001", 32);

    // Model 1 Data - Device Address
    _registers[MODEL1_DATA_OFFSET + 64] = MODBUS_UNIT_ID;

    // Model 103 (Three-Phase Inverter) Header
    _registers[MODEL103_ID_OFFSET] = 103;       // Model ID
    _registers[MODEL103_LENGTH_OFFSET] = MODEL103_LENGTH;  // Length

    // Model 103 Scale Factors (set once, used for all readings)
    // Current scale factor: -2 means divide by 100 (0.01A resolution)
    _registers[MODEL103_DATA_OFFSET + Model103::A_SF] = (uint16_t)(int16_t)-2;

    // Voltage scale factor: -1 means divide by 10 (0.1V resolution)
    _registers[MODEL103_DATA_OFFSET + Model103::V_SF] = (uint16_t)(int16_t)-1;

    // Power scale factor: 0 means multiply by 1 (1W resolution)
    _registers[MODEL103_DATA_OFFSET + Model103::W_SF] = 0;

    // Frequency scale factor: -2 means divide by 100 (0.01Hz resolution)
    _registers[MODEL103_DATA_OFFSET + Model103::Hz_SF] = (uint16_t)(int16_t)-2;

    // VA scale factor: 0
    _registers[MODEL103_DATA_OFFSET + Model103::VA_SF] = 0;

    // VAr scale factor: 0
    _registers[MODEL103_DATA_OFFSET + Model103::VAr_SF] = 0;

    // PF scale factor: -2 means divide by 100 (0.01 resolution, 100 = 1.00)
    _registers[MODEL103_DATA_OFFSET + Model103::PF_SF] = (uint16_t)(int16_t)-2;

    // Energy scale factor: 0 means 1 Wh resolution
    _registers[MODEL103_DATA_OFFSET + Model103::WH_SF] = 0;

    // DC Current scale factor: -2
    _registers[MODEL103_DATA_OFFSET + Model103::DCA_SF] = (uint16_t)(int16_t)-2;

    // DC Voltage scale factor: -1
    _registers[MODEL103_DATA_OFFSET + Model103::DCV_SF] = (uint16_t)(int16_t)-1;

    // DC Power scale factor: 0
    _registers[MODEL103_DATA_OFFSET + Model103::DCW_SF] = 0;

    // Initialize DC values (PV string voltage present even at 0 power)
    _registers[MODEL103_DATA_OFFSET + Model103::DCV] = 4500;  // 450.0V with SF=-1
    _registers[MODEL103_DATA_OFFSET + Model103::DCA] = 0;     // 0A
    _registers[MODEL103_DATA_OFFSET + Model103::DCW] = 0;     // 0W

    // Temperature scale factor: 0 (1°C resolution)
    _registers[MODEL103_DATA_OFFSET + Model103::Tmp_SF] = 0;

    // Initial operating state: OFF
    _registers[MODEL103_DATA_OFFSET + Model103::St] = (uint16_t)InverterState::OFF;
    _registers[MODEL103_DATA_OFFSET + Model103::StVnd] = 0;

    // End Model Marker
    _registers[END_MODEL_OFFSET] = 0xFFFF;
    _registers[END_MODEL_OFFSET + 1] = 0;

    DEBUG_SERIAL.println("SunSpec: Model initialized");
    DEBUG_SERIAL.print("  - Registers: ");
    DEBUG_SERIAL.println(TOTAL_REGISTERS);
}

uint16_t* SunSpecModel::getRegisters() {
    return _registers;
}

uint16_t SunSpecModel::getRegisterCount() {
    return TOTAL_REGISTERS;
}

void SunSpecModel::writeString(uint16_t offset, const char* str, uint16_t maxLen) {
    uint16_t strLen = strlen(str);
    uint16_t regCount = maxLen / 2;

    for (uint16_t i = 0; i < regCount; i++) {
        uint16_t charIdx = i * 2;
        char hi = (charIdx < strLen) ? str[charIdx] : ' ';
        char lo = (charIdx + 1 < strLen) ? str[charIdx + 1] : ' ';
        _registers[offset + i] = ((uint16_t)hi << 8) | (uint8_t)lo;
    }
}

void SunSpecModel::writeUint32(uint16_t offset, uint32_t value) {
    _registers[offset] = (uint16_t)(value >> 16);     // High word
    _registers[offset + 1] = (uint16_t)(value & 0xFFFF);  // Low word
}

void SunSpecModel::setACPower(int16_t watts) {
    _registers[MODEL103_DATA_OFFSET + Model103::W] = (uint16_t)watts;
}

void SunSpecModel::setACCurrent(float totalAmps, float phaseA, float phaseB, float phaseC) {
    // Scale factor is -2, so multiply by 100
    _registers[MODEL103_DATA_OFFSET + Model103::A] = (uint16_t)(totalAmps * 100);
    _registers[MODEL103_DATA_OFFSET + Model103::AphA] = (uint16_t)(phaseA * 100);
    _registers[MODEL103_DATA_OFFSET + Model103::AphB] = (uint16_t)(phaseB * 100);
    _registers[MODEL103_DATA_OFFSET + Model103::AphC] = (uint16_t)(phaseC * 100);
}

void SunSpecModel::setACVoltage(float phaseA, float phaseB, float phaseC) {
    // Scale factor is -1, so multiply by 10
    _registers[MODEL103_DATA_OFFSET + Model103::PhVphA] = (uint16_t)(phaseA * 10);
    _registers[MODEL103_DATA_OFFSET + Model103::PhVphB] = (uint16_t)(phaseB * 10);
    _registers[MODEL103_DATA_OFFSET + Model103::PhVphC] = (uint16_t)(phaseC * 10);
}

void SunSpecModel::setLineVoltage(float ab, float bc, float ca) {
    // Scale factor is -1, so multiply by 10
    _registers[MODEL103_DATA_OFFSET + Model103::PPVphAB] = (uint16_t)(ab * 10);
    _registers[MODEL103_DATA_OFFSET + Model103::PPVphBC] = (uint16_t)(bc * 10);
    _registers[MODEL103_DATA_OFFSET + Model103::PPVphCA] = (uint16_t)(ca * 10);
}

void SunSpecModel::setFrequency(float hz) {
    // Scale factor is -2, so multiply by 100
    _registers[MODEL103_DATA_OFFSET + Model103::Hz] = (uint16_t)(hz * 100);
}

void SunSpecModel::setPowerFactor(float pf) {
    // Scale factor is -2, so multiply by 100
    // PF is typically -1.0 to 1.0, stored as -100 to 100
    _registers[MODEL103_DATA_OFFSET + Model103::PF] = (uint16_t)(int16_t)(pf * 100);
}

void SunSpecModel::setApparentPower(int16_t va) {
    _registers[MODEL103_DATA_OFFSET + Model103::VA] = (uint16_t)va;
}

void SunSpecModel::setReactivePower(int16_t var) {
    _registers[MODEL103_DATA_OFFSET + Model103::VAr] = (uint16_t)var;
}

void SunSpecModel::setEnergy(uint32_t wh) {
    writeUint32(MODEL103_DATA_OFFSET + Model103::WH_HI, wh);
}

void SunSpecModel::setDCValues(float voltage, float current, int16_t power) {
    // Voltage scale factor is -1
    _registers[MODEL103_DATA_OFFSET + Model103::DCV] = (uint16_t)(voltage * 10);
    // Current scale factor is -2
    _registers[MODEL103_DATA_OFFSET + Model103::DCA] = (uint16_t)(current * 100);
    // Power scale factor is 0
    _registers[MODEL103_DATA_OFFSET + Model103::DCW] = (uint16_t)power;
}

void SunSpecModel::setTemperature(int16_t cabinetTemp) {
    _registers[MODEL103_DATA_OFFSET + Model103::TmpCab] = (uint16_t)cabinetTemp;
    _registers[MODEL103_DATA_OFFSET + Model103::TmpSnk] = (uint16_t)cabinetTemp;
    _registers[MODEL103_DATA_OFFSET + Model103::TmpTrns] = 0xFFFF;  // Not implemented
    _registers[MODEL103_DATA_OFFSET + Model103::TmpOt] = 0xFFFF;    // Not implemented
}

void SunSpecModel::setOperatingState(InverterState state) {
    _registers[MODEL103_DATA_OFFSET + Model103::St] = (uint16_t)state;
}
