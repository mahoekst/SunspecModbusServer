# Configuration Guide

This guide explains how to configure the SunSpec Modbus Server for your setup.

## Basic Configuration

### ESPHome YAML File

The main configuration is in `esphome/sunspec_server.yaml`. Key settings:

#### Substitutions

```yaml
substitutions:
  device_name: sunspec-modbus      # Device hostname
  friendly_name: "SunSpec Modbus Server"  # Display name
  modbus_port: "502"               # Modbus TCP port
```

#### WiFi Configuration

Edit `esphome/secrets.yaml` with your network details:

```yaml
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
```

#### Ethernet Configuration (Optional)

For stable operation, use Ethernet instead of WiFi:

```yaml
ethernet:
  type: LAN8720
  mdc_pin: GPIO23
  mdio_pin: GPIO18
  clk_mode: GPIO17_OUT
  phy_addr: 0
  power_pin: GPIO16
```

### ESP32 Board Selection

Supported boards in `esphome/sunspec_server.yaml`:

- `esp32-devkit-c` - Standard ESP32 DevKit
- `esp32-poe` - Olimex with PoE support
- `lilygo-t-internet-poe` - LilyGO with built-in Ethernet

## Advanced Configuration

### Modbus Server Settings

In `esphome/packages/modbus_server.yaml`:

```yaml
# Port configuration
modbus_port: 502          # Standard Modbus TCP port
# Note: Ports < 1024 require special permissions on Linux/Mac
# Use 5020 or higher if running on a system that restricts port access

# Update interval for simulated values
update_interval: 5000     # Milliseconds
```

### Simulated Values

Customize the simulated inverter parameters:

#### AC Power Output

Range: 1000W - 5000W (default: 3500W)

Modify in `esphome/packages/sunspec_registers.yaml`:

```yaml
number:
  - platform: template
    name: "AC Power Output Setpoint"
    min_value: 0
    max_value: 10000      # Change max power
    set_action:
      lambda: |-
        id(ac_power_value) = x;
```

#### DC Voltage Input

Range: 200V - 500V (default: 400V)

```yaml
number:
  - platform: template
    name: "DC Voltage Input Setpoint"
    min_value: 100
    max_value: 600        # Change max voltage
```

### Logging Configuration

Adjust logging verbosity in `sunspec_server.yaml`:

```yaml
logger:
  level: DEBUG            # Options: NONE, ERROR, WARN, INFO, DEBUG, VERBOSE
  logs:
    modbus: DEBUG         # Modbus-specific logging
    sunspec_modbus: DEBUG
```

## Security Configuration

### API Encryption

Generate a new encryption key:

```bash
esphome dashboard
# Select your device, then "Generate Encryption Key"
# Copy the key to secrets.yaml
```

### OTA Password

Change the over-the-air update password in `secrets.yaml`:

```yaml
ota_password: "your-secure-password"
```

### Network Security

- Change WiFi SSID and password
- Use strong AP (fallback) password
- Consider using static IP addresses
- Restrict Modbus port access with firewall rules

## Testing Configuration

### Validate YAML

```bash
cd esphome
esphome config sunspec_server.yaml
```

### Compile Without Flashing

```bash
esphome compile sunspec_server.yaml
```

### Dry Run (View Changes)

```bash
esphome diff sunspec_server.yaml
```

## Performance Tuning

### Update Interval

Faster updates = more CPU usage:

```yaml
update_interval: 1000     # 1 second (more frequent)
update_interval: 10000    # 10 seconds (less frequent)
```

### WiFi Power Saving

```yaml
wifi:
  power_save_mode: LIGHT  # Options: NONE, LIGHT, HIGH
```

### Memory Optimization

Disable features not needed:

```yaml
# Reduce log buffer
logger:
  level: WARN             # Less logging = less memory

# Disable web server if not needed
# web_server:
#   port: 80
```

## Network Configuration

### Static IP

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  manual_ip:
    static_ip: 192.168.1.100
    gateway: 192.168.1.1
    subnet: 255.255.255.0
```

### mDNS Service Name

```yaml
esphome:
  name: sunspec-modbus  # Accessible at sunspec-modbus.local
```

## Troubleshooting

### Port Already in Use

If port 502 is in use:

```bash
# Linux/Mac - Find process using port 502
lsof -i :502

# Windows
netstat -ano | findstr :502
```

Change to a different port in config:

```yaml
modbus_port: "5020"
```

### Connection Issues

Check logs:

```bash
esphome logs sunspec_server.yaml
```

Enable verbose logging:

```yaml
logger:
  level: VERBOSE
```

### Modbus Timeout

Increase update interval or reduce WiFi interference if experiencing timeouts.

## Configuration Examples

### Minimal Configuration

```yaml
# For quick testing without Ethernet
substitutions:
  device_name: sunspec-modbus
  friendly_name: "SunSpec Modbus"

# Keep other defaults from sunspec_server.yaml
```

### High-Reliability Configuration

```yaml
ethernet:
  type: LAN8720
  # ...ethernet settings...

logger:
  level: INFO

wifi:
  power_save_mode: NONE   # Disable power saving for WiFi

update_interval: 1000     # Frequent updates for responsiveness
```

### Low-Power Configuration

```yaml
logger:
  level: WARN

wifi:
  power_save_mode: HIGH   # Maximum power saving

update_interval: 30000    # Less frequent updates
```
