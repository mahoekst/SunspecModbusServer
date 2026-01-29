# SunSpec Modbus Register Mapping

This document describes the SunSpec register layout implemented in this project.

## Register Structure

SunSpec uses Modbus holding registers (address range 40000+) to store device information and parameters.

### Common Model (Model 65535)

Registers 40000-40068 contain common information about the device.

| Address | Name | Type | Description |
|---------|------|------|-------------|
| 40000-40003 | ID | String | "SunSpec" in ASCII |
| 40004 | DID | uint16 | Device Model ID (1 = Inverter) |
| 40005 | Length | uint16 | Length of model in 16-bit registers |
| 40006 | Version | uint16 | Model version |
| 40007 | Addr | uint16 | Address of next model or 0xFFFF if last |

### Model 1 - Inverter (Registers 40080+)

Single-phase or three-phase inverter model.

#### AC Measurements

| Address | Name | Unit | Type | Description |
|---------|------|------|------|-------------|
| 40080 | AC_Current | A | int16 | Output current (AC Current) |
| 40081 | AC_Current_A | A | int16 | Phase A current |
| 40082 | AC_Power | W | int16 | Total output power |
| 40083-40084 | AC_Power_A | W | int32 | Phase A power |
| 40088 | AC_Frequency | Hz | int16 | AC frequency (0.01 Hz resolution) |
| 40089 | AC_Apparent_Power | VA | int16 | Apparent power |
| 40090 | AC_Reactive_Power | var | int16 | Reactive power |
| 40091 | AC_Power_Factor | Pct | int16 | Power factor (0.01 resolution) |

#### DC Measurements

| Address | Name | Unit | Type | Description |
|---------|------|------|------|-------------|
| 40094 | DC_Voltage | V | int16 | DC voltage input (0.1 V resolution) |
| 40095 | DC_Current | A | int16 | DC input current (0.1 A resolution) |
| 40096-40097 | DC_Power | W | int32 | DC input power |

#### Temperature and Status

| Address | Name | Unit | Type | Description |
|---------|------|------|------|-------------|
| 40099 | Temp_Sink | C | int16 | Heat sink temperature |
| 40100 | Status | - | uint16 | Operational state (0=Off, 4=Running, etc.) |
| 40101 | Status_Vendor | - | uint16 | Vendor-specific status |

## Simulated Values

The device automatically generates realistic inverter data:

- **AC Power**: Fluctuates between 1000-5000W
- **DC Voltage**: Ranges from 350-450V
- **Efficiency**: ~97.5% (calculated from AC/DC power ratio)

## Register Access

### Read Operations (Function Code 3 & 4)

```
Request:  Function Code | Starting Address (2 bytes) | Quantity (2 bytes)
Response: Function Code | Byte Count | Register Values (2 bytes each)
```

### Write Operations (Function Code 6 & 16)

```
Request:  Function Code | Address | Value(s)
Response: Echo of request
```

## Example: Reading AC Power

To read the AC power output (register 40082):

```
Request:  0x03 | 0x9C 0x92 | 0x00 0x01
Response: 0x03 | 0x02 | <2 bytes AC power value>
```

## Model Extensions

- **Model 121**: Inverter extension (additional measurements)
- **Model 160**: Settings/configuration model

(More models can be added as needed)
