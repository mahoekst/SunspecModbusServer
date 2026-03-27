# SunSpec Modbus TCP Server for ESP32 (ESPHome)

## Project Overview
This is an ESPHome custom component that implements a SunSpec-compliant Modbus TCP server. It reads real inverter data from a Growatt 9000 TL3-S three-phase inverter via RS485/Modbus RTU, then exposes it via SunSpec Modbus TCP for integration with Victron GX systems. It also publishes all values to Home Assistant.

**Hardware:** LilyGo T-CAN485 ESP32 board

## Quick Start

### Validate config
```bash
cd esphome
esphome config growatt-sunspec-dev.yaml
```

### Build (local dev)
```bash
cd esphome
esphome compile growatt-sunspec-dev.yaml > /tmp/build.log 2>&1 &
tail -f /tmp/build.log
```

### Upload to ESP32
```bash
cd esphome
esphome upload growatt-sunspec-dev.yaml
```

### Monitor Serial Output
```bash
cd esphome
esphome logs growatt-sunspec-dev.yaml
```

## File Structure
```
esphome/
├── growatt-sunspec.yaml      # Production config (used in Home Assistant, pulls component from GitHub)
├── growatt-sunspec-dev.yaml  # Local dev config (uses local component source)
├── secrets.yaml              # WiFi/API credentials (not committed)
├── secrets.yaml.example      # Template for secrets.yaml
└── components/
    └── sunspec_modbus_server/
        ├── __init__.py       # ESPHome component config schema & codegen
        ├── sunspec_server.h  # C++ class definition
        └── sunspec_server.cpp# C++ implementation
```

## Component Configuration

The `sunspec_modbus_server` component accepts the following options:

| Option | Default | Description |
|--------|---------|-------------|
| `port` | 502 | Modbus TCP port |
| `unit_id` | 1 | Modbus unit/slave ID |
| `manufacturer` | "Growatt" | SunSpec Model 1 manufacturer string |
| `model` | "9000 TL3-S" | SunSpec Model 1 model string |
| `serial` | "EMULATED001" | SunSpec Model 1 serial string |
| `update_interval` | 1s | How often registers are refreshed |
| `source_*` | — | Input sensor IDs from modbus_controller/growatt_solar |
| `target_power_limit` | — | Number entity ID to receive power limit commands from Victron |

## SunSpec Register Layout (Base: 40000)
| Offset | Content |
|--------|---------|
| 0-1 | SunSpec ID "SunS" |
| 2-3 | Model 1 header |
| 4-68 | Model 1 (Common) data |
| 69-70 | Model 103 header |
| 71-120 | Model 103 (Three-Phase Inverter) data |
| 121-122 | Model 123 header |
| 123-146 | Model 123 (Immediate Controls) — power limit via Victron |
| 147-148 | End marker |

### Model 103 — Three-Phase Inverter
AC power, current (A/B/C), voltage (A/B/C), frequency, power factor, total energy, DC voltage/current/power, temperature, operating state.

### Model 123 — Immediate Controls
Allows Victron GX to write a power limit (`WMaxLimPct` + `WMaxLim_Ena`). The component forwards this to the configured `target_power_limit` number entity, which writes it to the Growatt inverter via Modbus RTU register 3.

## Development Workflow

- Edit component files in `esphome/components/sunspec_modbus_server/`
- Test locally using `growatt-sunspec-dev.yaml` (local path source)
- When ready, commit and push — `growatt-sunspec.yaml` (used in HA) pulls from GitHub `main`

## Development Guidelines
- Always check that suggested APIs/methods are not deprecated before using them
- Verify compatibility with the current ESPHome version (currently 2026.1.0)
- Avoid using deprecated functions — use recommended replacements
- Run `esphome compile` in the background with log redirection; it takes several minutes
