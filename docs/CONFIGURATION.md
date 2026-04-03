# Configuration Reference

## Component options

All options go under the `sunspec_modbus_server:` key in your ESPHome YAML.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `port` | int | 502 | Modbus TCP listen port |
| `unit_id` | int | 1 | Modbus unit/slave ID (Victron expects 126) |
| `manufacturer` | string | `"Growatt"` | Model 1 `Mn` field — shown on Cerbo product page |
| `model` | string | `"9000 TL3-S"` | Model 1 `Md` field |
| `serial` | string | `"EMULATED001"` | Model 1 `SN` field — serial number shown on Cerbo |
| `version` | string | `"1.0.0"` | Model 1 `Vr` field — firmware version shown on Cerbo |
| `max_power` | int | 9000 | Rated power in watts — used in Model 120 `WRtg` |
| `update_interval` | duration | `1s` | How often registers are refreshed from source sensors |

## Source sensors (input from Growatt)

These wire ESPHome sensor IDs (from `growatt_solar` or `modbus_controller`) into the SunSpec registers.

| Option | SunSpec destination |
|--------|-------------------|
| `source_ac_power` | Model 103 `W` |
| `source_voltage_a` | Model 103 `PhVphA` |
| `source_voltage_b` | Model 103 `PhVphB` |
| `source_voltage_c` | Model 103 `PhVphC` |
| `source_current_a` | Model 103 `AphA` |
| `source_current_b` | Model 103 `AphB` |
| `source_current_c` | Model 103 `AphC` |
| `source_frequency` | Model 103 `Hz` |
| `source_power_factor` | Model 103 `PF` |
| `source_total_energy` | Model 103 `WH` (32-bit, Wh) |
| `source_dc_voltage` | Model 103 `DCV` + Model 160 PV1 `T_DCV` |
| `source_dc_current` | Model 103 `DCA` + Model 160 PV1 `T_DCA` |
| `source_dc_power` | Model 103 `DCW` + Model 160 PV1 `T_DCW` |
| `source_temperature` | Model 103 `TmpCab` |
| `source_pv2_voltage` | Model 160 PV2 `T_DCV` |
| `source_pv2_current` | Model 160 PV2 `T_DCA` |
| `source_pv2_power` | Model 160 PV2 `T_DCW` |
| `source_inverter_status` | Model 103 `St` (operating state) — see below |

## Operating state logic

The SunSpec Model 103 `St` register is set using the following logic:

**When `source_inverter_status` is wired** (recommended), the Growatt's own status code is used:

| Growatt status | Value | SunSpec state |
|---|---|---|
| Normal (producing) | 1 | `MPPT (4)` or `THROTTLED (5)` if power limit active |
| Waiting (sun present, not yet producing) | 0 | `STANDBY (8)` |
| Fault | 3 | `FAULT (7)` |
| No state (inverter off, not responding to Modbus) | — | `SLEEPING (2)` |

**When `source_inverter_status` is not wired**, the state is derived from measurements:

| Condition | SunSpec state |
|---|---|
| `ac_power > 0` | `MPPT (4)` or `THROTTLED (5)` |
| `dc_voltage > 0` but `ac_power == 0` | `STANDBY (8)` |
| `dc_voltage == 0` | `SLEEPING (2)` |

The `SLEEPING` state covers the case where the inverter is fully powered down at night and not responding to Modbus queries — the sensor has no state, so `has_state()` returns false and `SLEEPING` is reported to Victron.

## Power limit target

| Option | Description |
|--------|-------------|
| `target_power_limit` | ID of a `number` entity that receives the power limit percentage written by Victron via Model 123. Typically a `modbus_controller` number writing to Growatt holding register 3. |

## Output sensors (publish to Home Assistant)

Optional — expose derived/scaled values as HA sensors. Each accepts a full `sensor.sensor_schema` (name, icon, etc.).

| Option | Unit |
|--------|------|
| `ac_power` | W |
| `ac_voltage_a/b/c` | V |
| `ac_current_a/b/c` | A |
| `ac_current_total` | A |
| `frequency` | Hz |
| `power_factor` | — |
| `total_energy` | Wh |
| `dc_voltage` | V |
| `dc_current` | A |
| `dc_power` | W |
| `temperature` | °C |

## Minimal example

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [ sunspec_modbus_server ]

sunspec_modbus_server:
  port: 502
  unit_id: 126
  manufacturer: "Growatt"
  model: "9000 TL3-S"
  serial: "ABC123XYZ"
  version: "dhaa0/dhaa1413"
  max_power: 9000
  update_interval: 1s

  source_ac_power: my_ac_power_sensor
  source_voltage_a: my_voltage_a_sensor
  # ... other source sensors ...

  target_power_limit: my_power_limit_number
```

## Substitutions pattern (recommended)

```yaml
substitutions:
  manufacturer: "Growatt"
  model: "9000 TL3-S"
  serial: "ABC123XYZ"       # from label on the inverter
  version: "dhaa0/dhaa1413" # firmware shown on inverter display
  max_power: "9000"

sunspec_modbus_server:
  manufacturer: ${manufacturer}
  model: ${model}
  serial: ${serial}
  version: ${version}
  max_power: ${max_power}
  # ...
```

## Notes

- `unit_id: 126` is the standard SunSpec unit ID expected by Victron GX devices
- `total_energy` source sensor must be in **Wh** (the `growatt_solar` platform reports kWh — multiply by 1000 in a filter)
- Line voltages from `growatt_solar` are line-to-line (~400 V); apply `multiply: 0.57735` to convert to phase-to-neutral (~230 V) before passing to `source_voltage_a/b/c`
