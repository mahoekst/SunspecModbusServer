# Quick Start Guide

Get your SunSpec Modbus Server running in 5 minutes!

## What You Need

- ESP32 microcontroller (DevKit-C recommended)
- USB cable
- Computer with Python 3.9+
- WiFi network (optional - Ethernet recommended)

## Installation Steps

### 1. Install ESPHome

```bash
pip install esphome
```

### 2. Clone and Configure

```bash
cd SunspecModbusServer
cp esphome/secrets.example.yaml esphome/secrets.yaml
```

Edit `esphome/secrets.yaml`:

```yaml
wifi_ssid: "YOUR_SSID"
wifi_password: "YOUR_PASSWORD"
api_encryption_key: "generate-a-key"  # See below
ota_password: "esphome"
ap_password: "esphomeap"
```

### 3. Generate Encryption Key

```bash
esphome logs sunspec_server.yaml
# Or generate manually via ESPHome dashboard
```

### 4. Flash Device

```bash
cd esphome
esphome run sunspec_server.yaml
```

Select your USB port when prompted.

### 5. Verify Connection

```bash
esphome logs sunspec_server.yaml
```

Look for: `SunSpec Modbus Server started successfully`

## Testing the Server

### Find Device IP

From logs or check your router. Device advertises as `sunspec-modbus.local`

### Test with Modbus Client

```python
from pymodbus.client import ModbusTcpClient

client = ModbusTcpClient(host='192.168.1.100', port=502)
result = client.read_holding_registers(40082, 1)
print(f"AC Power: {result.registers[0]}W")
```

### Test with modbus-cli

```bash
# Read AC power (register 40082)
modbus read 192.168.1.100 40082

# Write AC power setpoint
modbus write 192.168.1.100 40082 4000
```

## Web Dashboard

Access at: `http://sunspec-modbus.local:80`

Control simulated values:
- Adjust AC Power Output
- Modify DC Voltage Input
- View device status

## Next Steps

- Read [CONFIGURATION.md](CONFIGURATION.md) for advanced setup
- Check [SUNSPEC_REGISTERS.md](SUNSPEC_REGISTERS.md) for register info
- See [DEVELOPMENT.md](DEVELOPMENT.md) for extending functionality

## Troubleshooting

**Device not appearing?**
- Check USB cable
- Try different USB port
- Restart ESPHome

**Can't connect to device?**
- Verify WiFi credentials
- Check IP address in logs
- Ensure device is on same network

**Modbus timeout?**
- Check device logs with `esphome logs sunspec_server.yaml`
- Verify port 502 is accessible
- Try connecting from same network initially

**Port permission denied?**
- Linux/Mac: Use `sudo` for ports < 1024
- Or change port to 5020+ in configuration

For more help, see [CONFIGURATION.md](CONFIGURATION.md#troubleshooting).
