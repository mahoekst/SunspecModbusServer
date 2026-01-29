#ifndef MODBUS_SERVER_H
#define MODBUS_SERVER_H

#include <Arduino.h>
#include <ModbusIP_ESP8266.h>
#include "config.h"

class ModbusServer {
public:
    ModbusServer();

    // Initialize Modbus TCP server
    bool begin();

    // Map SunSpec registers to Modbus holding registers
    void mapRegisters(uint16_t* registers, uint16_t count);

    // Update Modbus registers from source array
    void updateRegisters(uint16_t* registers, uint16_t count);

    // Process Modbus requests (call in loop)
    void task();

    // Check if any clients are connected
    bool hasClients();

private:
    ModbusIP _modbus;
    uint16_t _registerCount;
};

#endif // MODBUS_SERVER_H
