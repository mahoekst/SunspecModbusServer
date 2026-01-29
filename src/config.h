#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_AP_NAME "SunSpec-Setup"
#define WIFI_CONFIG_TIMEOUT 180  // seconds

// Modbus Configuration
#define MODBUS_TCP_PORT 502
#define MODBUS_UNIT_ID 1

// SunSpec Configuration
// SunSpec registers start at Modbus address 40001 (0-indexed: 40000)
#define SUNSPEC_BASE_ADDRESS 40000

// SunSpec Model Offsets (relative to base)
#define SUNSPEC_ID_OFFSET 0        // "SunS" identifier (2 registers)
#define MODEL1_ID_OFFSET 2         // Model 1 header
#define MODEL1_LENGTH_OFFSET 3     // Model 1 length
#define MODEL1_DATA_OFFSET 4       // Model 1 data start
#define MODEL1_LENGTH 65           // Model 1 data length

#define MODEL103_ID_OFFSET 69      // Model 103 header (4 + MODEL1_LENGTH = 69)
#define MODEL103_LENGTH_OFFSET 70  // Model 103 length
#define MODEL103_DATA_OFFSET 71    // Model 103 data start
#define MODEL103_LENGTH 50         // Model 103 data length

#define END_MODEL_OFFSET 121       // End model marker (71 + 50 = 121)
#define TOTAL_REGISTERS 123        // Total SunSpec registers (0-122 inclusive)

// Inverter Simulation Parameters
#define INVERTER_MAX_POWER 10      // Watts (minimal for testing without affecting system)
#define GRID_FREQUENCY 50.0f       // Hz
#define GRID_VOLTAGE 230.0f        // V (phase-to-neutral)
#define POWER_FACTOR 0.99f

// Simulation Update Interval
#define SIMULATION_UPDATE_MS 1000  // Update every second

// Serial Debug
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

#endif // CONFIG_H
