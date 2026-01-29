#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "sunspec_model.h"
#include "simulator.h"
#include "modbus_server.h"

// Global instances
WiFiSetup wifiSetup;
SunSpecModel sunspec;
Simulator simulator;
ModbusServer modbusServer;

// Status tracking
unsigned long lastStatusPrint = 0;
const unsigned long STATUS_INTERVAL_MS = 10000;  // Print status every 10 seconds

void printStatus() {
    const SimulatedValues& values = simulator.getValues();

    DEBUG_SERIAL.println("--- Status ---");
    DEBUG_SERIAL.print("WiFi: ");
    DEBUG_SERIAL.println(wifiSetup.getStatusString());

    DEBUG_SERIAL.print("Power: ");
    DEBUG_SERIAL.print(values.acPower, 1);
    DEBUG_SERIAL.println(" W");

    DEBUG_SERIAL.print("Voltage: ");
    DEBUG_SERIAL.print(values.acVoltageA, 1);
    DEBUG_SERIAL.print(" / ");
    DEBUG_SERIAL.print(values.acVoltageB, 1);
    DEBUG_SERIAL.print(" / ");
    DEBUG_SERIAL.print(values.acVoltageC, 1);
    DEBUG_SERIAL.println(" V");

    DEBUG_SERIAL.print("Current: ");
    DEBUG_SERIAL.print(values.acCurrentTotal, 2);
    DEBUG_SERIAL.println(" A");

    DEBUG_SERIAL.print("Frequency: ");
    DEBUG_SERIAL.print(values.frequency, 2);
    DEBUG_SERIAL.println(" Hz");

    DEBUG_SERIAL.print("Energy: ");
    DEBUG_SERIAL.print(values.totalEnergy);
    DEBUG_SERIAL.println(" Wh");

    DEBUG_SERIAL.print("DC: ");
    DEBUG_SERIAL.print(values.dcVoltage, 1);
    DEBUG_SERIAL.print("V / ");
    DEBUG_SERIAL.print(values.dcCurrent, 2);
    DEBUG_SERIAL.print("A / ");
    DEBUG_SERIAL.print(values.dcPower, 1);
    DEBUG_SERIAL.println("W");

    DEBUG_SERIAL.print("Temp: ");
    DEBUG_SERIAL.print(values.temperature);
    DEBUG_SERIAL.println(" C");

    DEBUG_SERIAL.println("--------------");
}

void updateSunSpecFromSimulator() {
    const SimulatedValues& values = simulator.getValues();

    // Update SunSpec Model 103 registers
    sunspec.setACPower((int16_t)values.acPower);
    sunspec.setACCurrent(values.acCurrentTotal, values.acCurrentA, values.acCurrentB, values.acCurrentC);
    sunspec.setACVoltage(values.acVoltageA, values.acVoltageB, values.acVoltageC);
    sunspec.setLineVoltage(values.lineVoltageAB, values.lineVoltageBC, values.lineVoltageCA);
    sunspec.setFrequency(values.frequency);
    sunspec.setPowerFactor(values.powerFactor);
    sunspec.setApparentPower((int16_t)values.apparentPower);
    sunspec.setReactivePower((int16_t)values.reactivePower);
    sunspec.setEnergy(values.totalEnergy);
    sunspec.setDCValues(values.dcVoltage, values.dcCurrent, (int16_t)values.dcPower);
    sunspec.setTemperature(values.temperature);

    // Set operating state based on power production
    if (values.isProducing) {
        sunspec.setOperatingState(InverterState::MPPT);
    } else {
        sunspec.setOperatingState(InverterState::SLEEPING);
    }

    // Update Modbus registers
    modbusServer.updateRegisters(sunspec.getRegisters(), sunspec.getRegisterCount());
}

void setup() {
    // Initialize serial
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    delay(1000);  // Wait for serial to stabilize

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.println("SunSpec Modbus TCP Server");
    DEBUG_SERIAL.println("Phase 1: Simulation Mode");
    DEBUG_SERIAL.println("================================");
    DEBUG_SERIAL.println();

    // Initialize WiFi
    DEBUG_SERIAL.println("Starting WiFi setup...");
    if (!wifiSetup.begin(WIFI_AP_NAME, WIFI_CONFIG_TIMEOUT)) {
        DEBUG_SERIAL.println("WiFi connection failed!");
        DEBUG_SERIAL.println("Restarting in 5 seconds...");
        delay(5000);
        ESP.restart();
    }

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.print("Connected! IP: ");
    DEBUG_SERIAL.println(wifiSetup.getIP());
    DEBUG_SERIAL.println();

    // Initialize SunSpec model
    DEBUG_SERIAL.println("Initializing SunSpec model...");
    sunspec.begin();

    // Initialize Modbus server
    DEBUG_SERIAL.println("Starting Modbus TCP server...");
    modbusServer.begin();
    modbusServer.mapRegisters(sunspec.getRegisters(), sunspec.getRegisterCount());

    // Initialize simulator
    DEBUG_SERIAL.println("Starting simulator...");
    simulator.begin();

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("=== Server Ready ===");
    DEBUG_SERIAL.print("Modbus TCP: ");
    DEBUG_SERIAL.print(wifiSetup.getIP());
    DEBUG_SERIAL.print(":");
    DEBUG_SERIAL.println(MODBUS_TCP_PORT);
    DEBUG_SERIAL.print("Unit ID: ");
    DEBUG_SERIAL.println(MODBUS_UNIT_ID);
    DEBUG_SERIAL.print("SunSpec Base: ");
    DEBUG_SERIAL.println(SUNSPEC_BASE_ADDRESS);
    DEBUG_SERIAL.println("====================");
    DEBUG_SERIAL.println();
}

void loop() {
    // Check WiFi connection
    if (!wifiSetup.isConnected()) {
        DEBUG_SERIAL.println("WiFi disconnected! Attempting reconnect...");
        delay(5000);
        if (!wifiSetup.isConnected()) {
            DEBUG_SERIAL.println("Reconnect failed, restarting...");
            ESP.restart();
        }
    }

    // Update simulator
    simulator.update();

    // Copy simulated values to SunSpec registers
    updateSunSpecFromSimulator();

    // Process Modbus requests
    modbusServer.task();

    // Print status periodically
    unsigned long now = millis();
    if (now - lastStatusPrint >= STATUS_INTERVAL_MS) {
        lastStatusPrint = now;
        printStatus();
    }

    // Small delay to prevent watchdog issues
    delay(10);
}
