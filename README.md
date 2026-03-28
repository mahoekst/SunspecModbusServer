# ESPHome SunSpec Modbus TCP Server

An ESPHome custom component that bridges a **Growatt 9000 TL3-S** three-phase inverter to **Victron GX** systems (Cerbo GX, Venus GX, etc.) using the SunSpec Modbus TCP protocol. Real inverter data is read via RS485/Modbus RTU and re-exposed as a SunSpec-compliant Modbus TCP server. All values are simultaneously published to **Home Assistant**.

## What it does

```
Growatt Inverter                  ESP32 (LilyGo T-CAN485)               Victron Cerbo GX
  RS485 / Modbus RTU  ──────────►  SunSpec Modbus TCP Server  ──────────►  VRM / DVCC
                                           │
                                           ▼
                                    Home Assistant
```

- Reads AC power, voltage, current, frequency, energy, DC input, temperature from the Growatt inverter every 10 seconds
- Exposes all data as a SunSpec-compliant Modbus TCP server on port 502
- Receives power limit commands from Victron (via Model 123 `WMaxLimPct`) and writes them back to the Growatt inverter over RS485
- Publishes all sensor values to Home Assistant via the ESPHome native API

## Hardware

| Component | Details |
|-----------|---------|
| ESP32 board | [LilyGo T-CAN485](https://github.com/Xinyuan-LilyGO/T-CAN485) |
| Connection to inverter | RS485 (built-in on T-CAN485) |
| Inverter | Growatt 9000 TL3-S (three-phase) |

## Supported SunSpec Models

| Model | Description | Purpose |
|-------|-------------|---------|
| **Model 1** | Common (Manufacturer/Model/Serial/Version) | Device identity shown on Cerbo |
| **Model 120** | Nameplate Ratings | Rated power, current, power factor |
| **Model 103** | Three-Phase Inverter | Live AC/DC measurements, state |
| **Model 160** | Multiple MPPT | Per-tracker (PV1 + PV2) voltage, current, power |
| **Model 123** | Immediate Controls | Power limit from Victron → Growatt (`WMaxLimPct`) |

See [docs/SUNSPEC_REGISTERS.md](docs/SUNSPEC_REGISTERS.md) for the full register map.

## Installation

### Option A — Pull component from GitHub (recommended)

Use `growatt-sunspec.yaml` as-is. The component is fetched automatically from this repository:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/mahoekst/SunspecModbusServer
      ref: main
      path: esphome/components
    components: [ sunspec_modbus_server ]
```

### Option B — Manual install (no GitHub dependency)

1. Copy the component folder into your ESPHome config directory:

   ```
   <your-esphome-config>/
   └── components/
       └── sunspec_modbus_server/
           ├── __init__.py
           ├── sunspec_server.h
           └── sunspec_server.cpp
   ```

2. In your YAML, reference the local copy:

   ```yaml
   external_components:
     - source:
         type: local
         path: components
       components: [ sunspec_modbus_server ]
   ```

3. Add the component configuration to your YAML (see [docs/CONFIGURATION.md](docs/CONFIGURATION.md)).

### Secrets

Copy `esphome/secrets.yaml.example` to `esphome/secrets.yaml` and fill in your credentials:

```yaml
wifi_ssid: "your-wifi"
wifi_password: "your-password"
esphome_api_key: "your-api-key"
esphome_ota_password: "your-ota-password"
esphome_ap_ssid: "Growatt Fallback"
esphome_ap_password: "your-ap-password"
```

### Build and flash

```bash
cd esphome
esphome run growatt-sunspec.yaml
```

Or in separate steps:

```bash
esphome compile growatt-sunspec.yaml
esphome upload growatt-sunspec.yaml
esphome logs growatt-sunspec.yaml
```

## Configuration

Edit the `substitutions` block at the top of your YAML to match your inverter:

```yaml
substitutions:
  manufacturer: "Growatt"
  model: "9000 TL3-S"
  serial: "ABC123XYZ"       # from label on the inverter
  version: "dhaa0/dhaa1413" # firmware version shown on inverter display
  max_power: "9000"         # rated power in watts
```

See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for all available options.

## Power Limiting (Victron DVCC)

The Victron Cerbo GX can write a power limit to Model 123 register `WMaxLimPct`. The component:

1. Receives the value over Modbus TCP
2. Applies a watchdog (revert timer): if Victron stops sending, power is restored to 100% after the timeout
3. Writes the percentage to the Growatt inverter via holding register 3 (`Active Power Rate`) over RS485

## Files

```
esphome/
├── growatt-sunspec.yaml          # Production config (used in Home Assistant)
├── growatt-sunspec-dev.yaml      # Local dev config (uses local component source)
├── secrets.yaml.example          # Template for secrets.yaml
└── components/
    └── sunspec_modbus_server/
        ├── __init__.py           # ESPHome component schema & code generation
        ├── sunspec_server.h      # C++ class definition
        └── sunspec_server.cpp    # C++ implementation
```
