# SunSpec Modbus TCP Server for ESP32

## Project Overview
This is a PlatformIO project for ESP32 that implements a SunSpec-compliant Modbus TCP server. It's designed to emulate a Growatt 9000 TL3-S three-phase inverter for integration testing with Victron GX systems.

### Project Phases
- **Phase 1 (current):** SunSpec Modbus TCP server with simulated inverter values
- **Phase 2 (future):** Add RS485/Modbus RTU client to read actual Growatt inverter data

## Quick Start

### Build
```bash
pio run
```

### Upload to ESP32
```bash
pio run -t upload
```

### Monitor Serial Output
```bash
pio device monitor
```

### WiFi Setup
1. On first boot, ESP32 creates AP "SunSpec-Setup"
2. Connect to this AP and configure your WiFi credentials
3. ESP32 will connect and display its IP address

## Architecture

### File Structure
```
src/
├── main.cpp           # Entry point, orchestrates components
├── config.h           # All configuration constants
├── wifi_manager.h/cpp # WiFiManager wrapper for captive portal
├── sunspec_model.h/cpp# SunSpec register definitions (Model 1 + 103)
├── simulator.h/cpp    # Generates realistic inverter data
└── modbus_server.h/cpp# Modbus TCP server wrapper
```

### Component Responsibilities
- **WiFiSetup**: Handles WiFi connection with captive portal fallback
- **SunSpecModel**: Manages SunSpec register layout and data mapping
- **Simulator**: Generates time-varying realistic inverter values
- **ModbusServer**: Wraps ModbusIP library, maps registers

## SunSpec Implementation

### Register Layout (Base: 40000)
| Offset | Content |
|--------|---------|
| 0-1 | SunSpec ID "SunS" |
| 2-3 | Model 1 header |
| 4-69 | Model 1 (Common) data |
| 70-71 | Model 103 header |
| 72-121 | Model 103 (Three-Phase Inverter) data |
| 122-123 | End marker |

### Model 1 (Common)
Contains manufacturer info: "Growatt", model: "9000 TL3-S", serial: "EMULATED001"

### Model 103 (Three-Phase Inverter)
Contains AC power, current, voltage, frequency, power factor, energy, DC values, temperature, and operating state.

## Simulation Mode
- Power follows a sine wave (0-9000W) cycling every 60 seconds
- Realistic noise on all measurements
- Balanced three-phase currents with slight imbalance
- Energy accumulation over time
- Temperature varies with power output

## Testing

### With Modbus Tools
Connect to `<ESP32_IP>:502` and read holding registers starting at 40000:
- Registers 0-1: Should show 0x5375 0x6E53 ("SunS")
- Register 2: Should show 1 (Model 1 ID)
- Register 70: Should show 103 (Model 103 ID)

### With Victron GX
1. Settings → Modbus TCP → Add device
2. IP: ESP32's IP, Port: 502, Unit ID: 1
3. Device should appear as "PV Inverter"

## Configuration (config.h)
| Constant | Default | Description |
|----------|---------|-------------|
| WIFI_AP_NAME | "SunSpec-Setup" | Captive portal AP name |
| MODBUS_TCP_PORT | 502 | Modbus TCP port |
| SUNSPEC_BASE_ADDRESS | 40000 | SunSpec register base |
| INVERTER_MAX_POWER | 9000 | Max power in watts |
| SIMULATION_UPDATE_MS | 1000 | Update interval |

## Dependencies
- `emelianov/modbus-esp8266` - Modbus TCP/RTU library
- `tzapu/WiFiManager` - WiFi captive portal
- ESP32 Arduino Core (via espressif32 platform)

## Future Work (Phase 2)
- Add Modbus RTU client for RS485 communication
- Read actual values from Growatt inverter
- Support for LilyGo ESP32 RS485 board
- Configurable inverter model parameters
