# Development Guide

This guide provides information for developers wanting to contribute to or modify the SunSpec Modbus Server project.

## Project Architecture

### Components

1. **Custom ESPHome Component** (`esphome/custom_components/sunspec_modbus/`)
   - C++ implementation of Modbus TCP server
   - SunSpec register mapping
   - Simulated inverter data generation

2. **ESPHome Configuration** (`esphome/`)
   - YAML configuration files
   - Device setup and network settings
   - Home Assistant integration

3. **Documentation** (`docs/`)
   - Configuration guides
   - Register mapping reference
   - Development guides

## Custom Component Structure

### Header File: `sunspec_modbus.h`

Defines the `SunSpecModbus` class with:

- **Socket Management**: TCP server socket creation and handling
- **Register Management**: Read/write methods for Modbus registers
- **Request Handling**: PDU processing and response generation

### Implementation File: `sunspec_modbus.cpp`

Implements:

- **Modbus Functions**: FC3, FC4, FC6, FC16 support
- **SunSpec Registers**: Common model and inverter model initialization
- **Simulated Data**: Realistic value updates with fluctuation

### Python Configuration: `__init__.py`

Bridges C++ component with ESPHome:

- Codegen configuration
- Parameter validation
- Integration with ESPHome framework

## Building and Testing

### Prerequisites

```bash
pip install esphome>=2024.10.0
```

### Compile Configuration

```bash
cd esphome
esphome compile sunspec_server.yaml
```

### Flash to Device

```bash
esphome run sunspec_server.yaml
```

### Monitor Logs

```bash
esphome logs sunspec_server.yaml
```

## Modifying Register Values

### Adding New Registers

Edit `sunspec_modbus.cpp` in `init_sunspec_registers()`:

```cpp
void SunSpecModbus::init_sunspec_registers() {
  // Example: Add temperature register
  this->write_register(40099, 45);  // 45°C
}
```

### Dynamic Value Updates

Modify `update_simulated_values_()` for custom simulation logic:

```cpp
void SunSpecModbus::update_simulated_values_() {
  // Your simulation code here
  ac_power += random_variation;
  this->write_register(40082, (int16_t)ac_power);
}
```

## Extending Modbus Functionality

### Adding New Modbus Functions

Currently supported:
- FC3: Read Holding Registers
- FC4: Read Input Registers
- FC6: Write Single Register
- FC16: Write Multiple Registers

### Implementing Additional Functions

1. Add handler in `sunspec_modbus.cpp`:

```cpp
void SunSpecModbus::handle_read_coils_(const uint8_t* request,
                                       uint8_t* response,
                                       size_t& response_size) {
  // Implementation
}
```

2. Add case in `process_modbus_pdu_()`:

```cpp
case 1:  // Read Coils
  this->handle_read_coils_(request, response, response_size);
  break;
```

## Adding New SunSpec Models

### Model 121: Inverter Extension

Create new register initialization:

```cpp
void SunSpecModbus::init_model_121_() {
  // Model 121 starts at 40200
  this->write_register(40200, 121);    // Model ID
  this->write_register(40201, 50);     // Length
  // Add model-specific registers
}
```

### Model 160: Settings

```cpp
void SunSpecModbus::init_model_160_() {
  // Model 160 settings
  this->write_register(40350, 1);      // Model ID
  this->write_register(40351, 20);     // Length
}
```

## Integration with Home Assistant

### Automatic Discovery

Add to Home Assistant configuration to automatically discover registers:

```yaml
template:
  - sensor:
      - name: "AC Power"
        unique_id: sunspec_ac_power
        unit_of_measurement: "W"
        device_class: power
        state_topic: "home/sunspec/ac_power"
```

### Custom Integration

For deeper integration, create a Home Assistant custom component using `pymodbus`.

## Testing

### Unit Testing

Create test cases in a new `tests/` directory:

```python
import unittest
from unittest.mock import Mock, patch

class TestModbusServer(unittest.TestCase):
    def test_read_register(self):
        # Test implementation
        pass
```

### Integration Testing

Test with a Modbus client:

```bash
pip install pymodbus
python tests/test_client.py
```

### Client Example

```python
from pymodbus.client import ModbusTcpClient

client = ModbusTcpClient(host='192.168.1.100', port=502)
result = client.read_holding_registers(40082, 1)
print(f"AC Power: {result.registers[0]}W")
```

## Code Standards

### C++ Style

- Use consistent indentation (2 spaces)
- Follow ESP-IDF naming conventions
- Use `ESP_LOG*` macros for logging
- Add comments for complex logic

### Python Style

- Follow PEP 8
- Use type hints where applicable
- Add docstrings to functions

## Performance Optimization

### Memory Usage

- Use `std::map` efficiently for registers
- Consider `std::array` for fixed-size register blocks
- Profile memory with ESPHome tools

### CPU Usage

- Optimize socket handling for non-blocking I/O
- Reduce update frequency if not needed
- Minimize logging in production

## Debugging

### Enable Verbose Logging

```yaml
logger:
  level: VERBOSE
  logs:
    sunspec_modbus: VERBOSE
    modbus: VERBOSE
```

### GDB Debugging

For advanced debugging:

```bash
esphome run sunspec_server.yaml --gdb
```

### Serial Monitor

```bash
esphome monitor sunspec_server.yaml
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/new-feature`
3. Make changes and test thoroughly
4. Submit a pull request with clear description

## Useful Resources

- [ESPHome Documentation](https://esphome.io/)
- [Modbus TCP Specification](https://www.modbus.org/)
- [SunSpec Alliance](https://sunspec.org/)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)

## Common Issues

### Compilation Errors

Check ESPHome version:
```bash
esphome version
```

Ensure minimum version 2024.10.0 is installed.

### Socket Binding Failures

- Check port permissions
- Verify no other service uses the port
- Try a port > 1024

### Modbus Timeouts

- Increase WiFi signal strength
- Reduce update interval
- Check network congestion

## Future Enhancements

- [ ] Three-phase inverter support
- [ ] Battery management system (BMS) model
- [ ] Firmware update mechanism
- [ ] Web dashboard for real-time monitoring
- [ ] MQTT integration
- [ ] Modbus RTU support (serial)
