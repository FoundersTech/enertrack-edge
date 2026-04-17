# EnerTrack Home — Firmware v1.0.0

ESP32-based embedded system for electrical energy consumption monitoring.
BLE onboarding flow inspired by Intelbras Mibo cameras.

---

## Project Structure

```
enertrack-home/
├── platformio.ini          # PlatformIO configuration
├── include/
│   ├── config.h            # Pins, BLE UUIDs, constants
│   ├── device_state.h      # Device state enum
│   ├── nvs_manager.h       # NVS storage (Wi-Fi credentials)
│   ├── ble_provisioning.h  # BLE GATT server (Wi-Fi onboarding)
│   └── energy_meter.h      # SCT-013 reading via EmonLib
└── src/
    └── main.cpp            # Main state machine
```

---

## Onboarding Flow

```
BOOT → NVS available?
         ├─ YES → connect to Wi-Fi → ONLINE
         └─ NO  → BLE_ADVERTISING
                      └─ app connects → BLE_CONNECTED
                             └─ receives SSID + password → WIFI_CONNECTING
                                    ├─ success → save to NVS → ONLINE
                                    └─ failure → notify app → BLE_CONNECTED
```

---

## SCT-013 Circuit (REQUIRED)

The ESP32 ADC operates within a **0–3.3V range**, while the SCT-013 outputs an AC signal.
A **signal conditioning circuit** is required:

```
         SCT-013
         (3.5mm jack)
            │ tip
            ├──── R_burden (33Ω) ──┬──── GND
            │                      │
            └──── R1 (10kΩ) ───────┤──── GPIO34 (ADC1_CH6)
                                   │
                  R2 (10kΩ) ───────┤
                  │                │
                 3.3V         C (10µF) ──── GND
```

### Component Roles

* **R_burden (33Ω)** → Defines sensor output voltage
* **R1 + R2 (10kΩ)** → Voltage divider for 1.65V bias (AC midpoint)
* **C (10µF)** → Bias stabilization (noise filtering)

### Important Notes

* Use only **GPIO34, 35, 36, or 39** → ADC1 channels, input-only, Wi-Fi safe
* ⚠️ **ADC2 pins cannot be used with Wi-Fi enabled**
  (GPIO 0, 2, 4, 12–15, 25–27)

---

## SCT-013 Calibration

Adjust `EMON_CALIBRATION` in `config.h`:

| SCT-013 Model           | Burden | Calibration Formula | Value  |
| ----------------------- | ------ | ------------------- | ------ |
| SCT-013-030 (30A/1V)    | 33Ω    | 30 / 0.495          | ~60.6  |
| SCT-013-060 (60A/1V)    | 33Ω    | 60 / 0.495          | ~121.2 |
| SCT-013-100 (100A/50mA) | 33Ω    | 100 / (0.05 × 33)   | ~60.6  |

### Calibration Procedure

1. Measure current using a clamp meter (reference device)
2. Compare with EnerTrack readings
3. Adjust `EMON_CALIBRATION` until values match

---

## Next Steps

* [ ] Define cloud protocol (MQTT / HTTP REST / Firebase)
* [ ] Implement data publishing in `ONLINE` state
* [ ] Develop mobile app for BLE onboarding
* [ ] Calculate accumulated energy (kWh) and estimated cost
* [ ] Implement OTA updates over Wi-Fi

---
