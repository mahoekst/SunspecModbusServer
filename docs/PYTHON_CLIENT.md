# SunSpec Modbus Python Client Example

This example demonstrates how to read and write SunSpec Modbus registers using Python.

## Installation

```bash
pip install pymodbus
```

## Basic Usage

### Connect to Server

```python
from pymodbus.client import ModbusTcpClient

# Connect to the SunSpec Modbus Server
client = ModbusTcpClient(host='192.168.1.100', port=502)
client.connect()

# Read AC Power (register 40082)
result = client.read_holding_registers(40082, 1)
if result.isError():
    print(f"Error: {result}")
else:
    ac_power = result.registers[0]
    print(f"AC Power: {ac_power}W")

client.close()
```

### Read Multiple Registers

```python
# Read common model info (registers 40000-40005)
result = client.read_holding_registers(40000, 6)

if not result.isError():
    for i, value in enumerate(result.registers):
        print(f"Register {40000 + i}: {value}")
```

### Write Register

```python
# Write AC Power setpoint to 3000W (register 40082)
result = client.write_register(40082, 3000)

if not result.isError():
    print("Successfully wrote AC power setpoint")
else:
    print(f"Write error: {result}")
```

## Example: Monitor Inverter

```python
import time
from pymodbus.client import ModbusTcpClient

def monitor_inverter(host='192.168.1.100', interval=5):
    """Monitor inverter values periodically"""
    
    client = ModbusTcpClient(host=host, port=502)
    if not client.connect():
        print(f"Failed to connect to {host}")
        return
    
    try:
        while True:
            # Read AC Power, DC Voltage, Temperature
            result = client.read_holding_registers(40082, 3)
            
            if not result.isError():
                ac_power = result.registers[0]
                dc_voltage = result.registers[1]
                temperature = result.registers[2]
                
                print(f"AC Power: {ac_power}W | DC Voltage: {dc_voltage}V | Temp: {temperature}°C")
            else:
                print(f"Error reading: {result}")
            
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print("Stopped")
    finally:
        client.close()

if __name__ == '__main__':
    monitor_inverter()
```

## Example: Simulate Load Profile

```python
def simulate_load_profile(host='192.168.1.100'):
    """Simulate a typical solar inverter power curve"""
    
    client = ModbusTcpClient(host=host, port=502)
    client.connect()
    
    # Simulated power levels throughout the day
    power_profile = [
        100,    # 6 AM - Sunrise
        500,    # 7 AM
        1500,   # 9 AM
        3500,   # 12 PM - Peak
        4000,   # 1 PM - Peak
        3000,   # 3 PM
        1000,   # 5 PM
        200,    # 6 PM - Sunset
    ]
    
    for power in power_profile:
        result = client.write_register(40082, power)
        print(f"Set power to {power}W")
        time.sleep(60)  # Wait 1 minute between updates
    
    client.close()

if __name__ == '__main__':
    simulate_load_profile()
```

## Register Reference

Common registers to read/write:

| Register | Name | Description |
|----------|------|-------------|
| 40082 | AC_Power | AC output power (W) |
| 40094 | DC_Voltage | DC input voltage (V) |
| 40095 | DC_Current | DC input current (A) |
| 40099 | Temperature | Heat sink temperature (°C) |
| 40100 | Status | Operating status |

## Error Handling

```python
def read_safe(client, address, count):
    """Read with error handling"""
    try:
        result = client.read_holding_registers(address, count)
        if result.isError():
            print(f"Modbus error: {result}")
            return None
        return result.registers
    except Exception as e:
        print(f"Exception: {e}")
        return None

# Usage
values = read_safe(client, 40082, 1)
if values:
    print(f"AC Power: {values[0]}W")
```

## Async Example

```python
import asyncio
from pymodbus.client import AsyncModbusTcpClient

async def async_monitor():
    """Asynchronous monitoring example"""
    
    client = AsyncModbusTcpClient(host='192.168.1.100', port=502)
    await client.connect()
    
    for _ in range(10):
        result = await client.read_holding_registers(40082, 1)
        if not result.isError():
            print(f"AC Power: {result.registers[0]}W")
        await asyncio.sleep(5)
    
    client.close()

# Run async example
asyncio.run(async_monitor())
```

## Testing Script

```bash
#!/bin/bash
# test_modbus.sh

IP="192.168.1.100"
PORT="502"

echo "Testing SunSpec Modbus Server at $IP:$PORT"

python3 << 'EOF'
from pymodbus.client import ModbusTcpClient
import sys

client = ModbusTcpClient(host='$IP', port=$PORT)

if client.connect():
    print("✓ Connected successfully")
    
    # Test read
    result = client.read_holding_registers(40000, 1)
    if not result.isError():
        print(f"✓ Read successful: {result.registers}")
    else:
        print(f"✗ Read failed: {result}")
    
    # Test write
    result = client.write_register(40082, 3500)
    if not result.isError():
        print("✓ Write successful")
    else:
        print(f"✗ Write failed: {result}")
    
    client.close()
else:
    print("✗ Connection failed")
    sys.exit(1)
EOF
```

## More Information

- [Pymodbus Documentation](https://pymodbus.readthedocs.io/)
- [Modbus TCP Specification](https://www.modbus.org/)
- [SunSpec Register Mapping](SUNSPEC_REGISTERS.md)
