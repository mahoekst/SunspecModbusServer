# ESPHome SunSpec Modbus Server

A comprehensive ESPHome project to simulate a SunSpec-compliant Modbus server, perfect for testing Modbus clients and solar monitoring applications.

## Overview

This project implements a Modbus TCP server on an ESP32 microcontroller that simulates SunSpec register mappings commonly found in solar inverters. It's ideal for:

- Testing Modbus clients without requiring actual solar equipment
- Development and debugging of Modbus applications
- Educational purposes to understand SunSpec standards
- Integration testing for energy management systems

## Hardware Requirements

- **ESP32** microcontroller (e.g., ESP32-DevKit-C, Lilygo T-Internet-POE)
- **Ethernet connection** (for reliable Modbus TCP communication)
- **Power supply** (5V/USB or dedicated power)

## Quick Start

### 1. Prerequisites

- ESPHome installed (`pip install esphome`)
- Python 3.9+
- USB cable for initial flashing

### 2. Configuration

Edit `esphome/sunspec_server.yaml` to match your device:

```yaml
substitutions:
  device_name: sunspec-modbus
  friendly_name: "SunSpec Modbus Server"
```

### 3. Build and Flash

```bash
cd esphome
esphome run sunspec_server.yaml
```

### 4. Access the Server

- Web Dashboard: `http://<device-ip>:80`
- Modbus TCP: `<device-ip>:502`

## Supported SunSpec Models

- **Model 1 (Inverter)** - Three-phase and single-phase configurations
- **Model 121** - Inverter extension for additional parameters
- **Model 160** - Settings model

## Configuration Guide

See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for detailed configuration options.

## API Reference

See [docs/SUNSPEC_REGISTERS.md](docs/SUNSPEC_REGISTERS.md) for complete register mapping.

## Development

For contributing to this project, see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## License

MIT License - see LICENSE file for details

## Support

For issues and questions, please open an issue on GitHub.
