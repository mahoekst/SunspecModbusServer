#include "modbus_server.h"

ModbusServer::ModbusServer() : _registerCount(0) {
}

bool ModbusServer::begin() {
    // Start Modbus TCP server
    _modbus.server(MODBUS_TCP_PORT);

    DEBUG_SERIAL.print("Modbus: TCP server started on port ");
    DEBUG_SERIAL.println(MODBUS_TCP_PORT);

    return true;
}

void ModbusServer::mapRegisters(uint16_t* registers, uint16_t count) {
    _registerCount = count;

    // Add holding registers starting at SunSpec base address
    // Modbus addresses are 1-based in protocol, but library uses 0-based
    // SunSpec base is 40001 in Modbus terms, which is 40000 in 0-based
    _modbus.addHreg(SUNSPEC_BASE_ADDRESS, 0, count);

    // Copy initial values
    for (uint16_t i = 0; i < count; i++) {
        _modbus.Hreg(SUNSPEC_BASE_ADDRESS + i, registers[i]);
    }

    DEBUG_SERIAL.print("Modbus: Mapped ");
    DEBUG_SERIAL.print(count);
    DEBUG_SERIAL.print(" registers at address ");
    DEBUG_SERIAL.println(SUNSPEC_BASE_ADDRESS);
}

void ModbusServer::updateRegisters(uint16_t* registers, uint16_t count) {
    // Update all registers from source array
    for (uint16_t i = 0; i < count && i < _registerCount; i++) {
        _modbus.Hreg(SUNSPEC_BASE_ADDRESS + i, registers[i]);
    }
}

void ModbusServer::task() {
    _modbus.task();
}

bool ModbusServer::hasClients() {
    // The ModbusIP library doesn't expose client count directly
    // This would need library modification to implement properly
    return true;  // Assume we might have clients
}
