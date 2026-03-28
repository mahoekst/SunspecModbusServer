# SunSpec Register Map

All registers are Modbus holding registers. Base address: **40000**.

The layout follows the SunSpec discovery chain: each model block starts with a 2-register header (Model ID + length), followed by data registers, followed by the next model header.

## Memory map overview

| Address range | Offset range | Content |
|---------------|-------------|---------|
| 40000–40001 | 0–1 | SunSpec identifier `"SunS"` |
| 40002–40003 | 2–3 | Model 1 header (ID=1, L=65) |
| 40004–40068 | 4–68 | Model 1 data (Common) |
| 40069–40070 | 69–70 | Model 120 header (ID=120, L=26) |
| 40071–40096 | 71–96 | Model 120 data (Nameplate) |
| 40097–40098 | 97–98 | Model 103 header (ID=103, L=50) |
| 40099–40148 | 99–148 | Model 103 data (Three-Phase Inverter) |
| 40149–40150 | 149–150 | Model 160 header (ID=160, L=48) |
| 40151–40198 | 151–198 | Model 160 data (Multiple MPPT) |
| 40199–40200 | 199–200 | Model 123 header (ID=123, L=24) |
| 40201–40224 | 201–224 | Model 123 data (Immediate Controls) |
| 40225–40226 | 225–226 | End marker (0xFFFF, 0x0000) |

---

## Model 1 — Common (40004–40068)

| Address | Offset | Name | Type | Value |
|---------|--------|------|------|-------|
| 40004–40019 | 0–15 | Mn (Manufacturer) | String[32] | e.g. `"Growatt"` |
| 40020–40035 | 16–31 | Md (Model) | String[32] | e.g. `"9000 TL3-S"` |
| 40036–40043 | 32–39 | Opt (Options) | String[16] | empty |
| 40044–40051 | 40–47 | Vr (Version) | String[16] | from `version:` config |
| 40052–40067 | 48–63 | SN (Serial Number) | String[32] | from `serial:` config |
| 40068 | 64 | DA (Device Address) | uint16 | 1 |

---

## Model 120 — Nameplate Ratings (40071–40096)

| Address | Offset | Name | Type | Description |
|---------|--------|------|------|-------------|
| 40071 | 0 | DERTyp | uint16 | DER type: 4 = PV |
| 40072 | 1 | WRtg | uint16 | Rated continuous power (W), from `max_power:` |
| 40073 | 2 | WRtg_SF | int16 | Scale factor: 0 |
| 40074 | 3 | VARtg | uint16 | Rated apparent power (VA) |
| 40075 | 4 | VARtg_SF | int16 | Scale factor: 0 |
| 40076–40079 | 5–8 | VArRtgQ1–Q4 | int16 | VAr capability (0 for PV) |
| 40080 | 9 | VArRtg_SF | int16 | Scale factor: 0 |
| 40081 | 10 | ARtg | uint16 | Max RMS AC current (A) |
| 40082 | 11 | ARtg_SF | int16 | Scale factor: 0 |
| 40083–40086 | 12–15 | PFRtgQ1–Q4 | int16 | Min power factor per quadrant |
| 40087 | 16 | PFRtg_SF | int16 | Scale factor: −2 (values × 0.01) |
| 40088–40096 | 17–25 | — | — | Reserved / not used |

---

## Model 103 — Three-Phase Inverter (40099–40148)

Scale factors are fixed at init and do not change at runtime.

### AC measurements

| Address | Offset | Name | SF | Unit | Description |
|---------|--------|------|----|------|-------------|
| 40099 | 0 | A | A_SF | A | Total AC current |
| 40100 | 1 | AphA | A_SF | A | Phase A current |
| 40101 | 2 | AphB | A_SF | A | Phase B current |
| 40102 | 3 | AphC | A_SF | A | Phase C current |
| 40103 | 4 | A_SF | — | — | −2 (0.01 A resolution) |
| 40104 | 5 | PPVphAB | V_SF | V | Line voltage A–B |
| 40105 | 6 | PPVphBC | V_SF | V | Line voltage B–C |
| 40106 | 7 | PPVphCA | V_SF | V | Line voltage C–A |
| 40107 | 8 | PhVphA | V_SF | V | Phase A voltage (phase-to-neutral) |
| 40108 | 9 | PhVphB | V_SF | V | Phase B voltage |
| 40109 | 10 | PhVphC | V_SF | V | Phase C voltage |
| 40110 | 11 | V_SF | — | — | −1 (0.1 V resolution) |
| 40111 | 12 | W | W_SF | W | AC power output |
| 40112 | 13 | W_SF | — | — | 0 (1 W resolution) |
| 40113 | 14 | Hz | Hz_SF | Hz | AC frequency |
| 40114 | 15 | Hz_SF | — | — | −2 (0.01 Hz resolution) |
| 40115 | 16 | VA | VA_SF | VA | Apparent power |
| 40116 | 17 | VA_SF | — | — | 0 |
| 40117 | 18 | VAr | VAr_SF | var | Reactive power |
| 40118 | 19 | VAr_SF | — | — | 0 |
| 40119 | 20 | PF | PF_SF | % | Power factor |
| 40120 | 21 | PF_SF | — | — | −2 (0.01 resolution) |
| 40121–40122 | 22–23 | WH | WH_SF | Wh | Total energy (32-bit, hi+lo) |
| 40123 | 24 | WH_SF | — | — | 0 (1 Wh resolution) |

### DC measurements

| Address | Offset | Name | SF | Unit | Description |
|---------|--------|------|----|------|-------------|
| 40124 | 25 | DCA | DCA_SF | A | DC input current (PV1) |
| 40125 | 26 | DCA_SF | — | — | −2 (0.01 A resolution) |
| 40126 | 27 | DCV | DCV_SF | V | DC input voltage (PV1) |
| 40127 | 28 | DCV_SF | — | — | −1 (0.1 V resolution) |
| 40128 | 29 | DCW | DCW_SF | W | DC input power (PV1) |
| 40129 | 30 | DCW_SF | — | — | 0 (1 W resolution) |

### Temperature and state

| Address | Offset | Name | SF | Unit | Description |
|---------|--------|------|----|------|-------------|
| 40130 | 31 | TmpCab | Tmp_SF | °C | Cabinet temperature |
| 40131 | 32 | TmpSnk | Tmp_SF | °C | Heat sink temperature |
| 40132 | 33 | TmpTrns | Tmp_SF | °C | Transformer temperature |
| 40133 | 34 | TmpOt | Tmp_SF | °C | Other temperature |
| 40134 | 35 | Tmp_SF | — | — | 0 (1 °C resolution) |
| 40135 | 36 | St | — | — | Operating state (see below) |
| 40136 | 37 | StVnd | — | — | Vendor state (0) |

**Operating states (St):**

| Value | State | When |
|-------|-------|------|
| 1 | OFF | AC power = 0 and DC power = 0 |
| 4 | MPPT | Normal production |
| 5 | THROTTLED | Power limit active (WMaxLim_Ena = 1) |

---

## Model 160 — Multiple MPPT (40151–40198)

### Global registers (40151–40158)

| Address | Offset | Name | Description |
|---------|--------|------|-------------|
| 40151 | 0 | DCA_SF | −2 (0.01 A) |
| 40152 | 1 | DCV_SF | −1 (0.1 V) |
| 40153 | 2 | DCW_SF | 0 (1 W) |
| 40154 | 3 | DCWH_SF | 0 |
| 40155–40156 | 4–5 | Evt1, Evt2 | Global events (0) |
| 40157 | 6 | N | Number of trackers: 2 |
| 40158 | 7 | TmsPer | Timestamp period (0) |

### Tracker 0 — PV1 (40159–40178)

| Address | Offset | Name | Description |
|---------|--------|------|-------------|
| 40159 | 0 | T_ID | Tracker ID: 1 |
| 40160–40167 | 1–8 | T_IDStr | `"PV1"` (8 registers) |
| 40168 | 9 | T_DCA | DC current (applies DCA_SF) |
| 40169 | 10 | T_DCV | DC voltage (applies DCV_SF) |
| 40170 | 11 | T_DCW | DC power (applies DCW_SF) |
| 40171–40178 | 12–19 | — | Reserved |

### Tracker 1 — PV2 (40179–40198)

| Address | Offset | Name | Description |
|---------|--------|------|-------------|
| 40179 | 0 | T_ID | Tracker ID: 2 |
| 40180–40187 | 1–8 | T_IDStr | `"PV2"` (8 registers) |
| 40188 | 9 | T_DCA | DC current (applies DCA_SF) |
| 40189 | 10 | T_DCV | DC voltage (applies DCV_SF) |
| 40190 | 11 | T_DCW | DC power (applies DCW_SF) |
| 40191–40198 | 12–19 | — | Reserved |

---

## Model 123 — Immediate Controls (40201–40224)

Victron writes power limit commands here. Only `WMaxLimPct` and `WMaxLim_Ena` are acted upon; all other fields are accepted but ignored.

| Address | Offset | Name | Type | Description |
|---------|--------|------|------|-------------|
| 40201 | 0 | Conn_WinTms | uint16 | Time window to connect (s) |
| 40202 | 1 | Conn_RvrtTms | uint16 | Revert connection timeout (s) |
| 40203 | 2 | Conn | uint16 | Connect/disconnect |
| 40204 | 3 | WMaxLimPct | uint16 | **Power limit (0–100%)** |
| 40205 | 4 | WMaxLimPct_WinTms | uint16 | Ramp window (s) |
| 40206 | 5 | WMaxLimPct_RvrtTms | uint16 | **Revert timeout (s)** — watchdog |
| 40207 | 6 | WMaxLimPct_RmpTms | uint16 | Ramp time (s) |
| 40208 | 7 | WMaxLim_Ena | uint16 | **1 = limit active, 0 = disabled** |
| 40209 | 8 | OutPFSet | int16 | Power factor setpoint |
| 40210–40213 | 9–12 | OutPFSet_* | — | PF window/revert/ramp/enable |
| 40214 | 13 | VArPct_Mod | uint16 | VAr percent mode |
| 40215–40218 | 14–17 | VArPct_* | — | VAr window/revert/ramp/setpoint |
| 40219 | 18 | VArSetPct_Ena | uint16 | VAr enable |
| 40220 | 19 | WMaxLimPct_SF | int16 | Scale factor: 0 (direct %) |
| 40221 | 20 | OutPFSet_SF | int16 | Scale factor |
| 40222–40224 | 21–23 | — | — | Padding |

### Power limit logic

When Victron writes `WMaxLim_Ena = 1`:
- `WMaxLimPct` value (scaled by `WMaxLimPct_SF`) is forwarded to the Growatt inverter as an active power rate percentage (0–100) via Modbus RTU holding register 3
- If `WMaxLimPct_RvrtTms > 0`, a watchdog timer is armed — if no further write arrives before it expires, power is restored to 100%

When `WMaxLim_Ena = 0`: power is immediately restored to 100%.
