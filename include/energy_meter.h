#pragma once
#include <Arduino.h>
#include <EmonLib.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Energy Measurement (SCT-013 Current Transformer)
//
// This module measures AC current using a non-invasive SCT-013 sensor
// and computes apparent power based on a fixed mains voltage.
//
// Required analog front-end on PIN_SCT013:
//   - Burden resistor: 33Ω (for SCT-013-030, 30A / 1V)
//   - Voltage divider: 2x 10kΩ (bias at VCC/2 ≈ 1.65V)
//   - Bypass capacitor: 10µF at bias node (noise stabilization)
//
// Notes:
// - The ESP32 ADC requires input signals within 0–3.3V
// - The bias ensures proper sampling of the AC waveform (centered signal)
// ─────────────────────────────────────────────────────────────────────────────
class EnergyMeter {
public:

    // Initializes the current measurement channel
    void begin() {
        _emon.current(PIN_SCT013, EMON_CALIBRATION);

        Serial.printf(
            "[ENERGY] SCT-013 initialized — pin %d, calibration %.1f\n",
            PIN_SCT013, EMON_CALIBRATION
        );
    }

    // Performs RMS current measurement and computes apparent power
    //
    // Execution:
    // - Call periodically (see ENERGY_READ_INTERVAL_MS)
    // - Uses EmonLib RMS calculation over EMON_SAMPLES samples
    void read() {
        _irms  = _emon.calcIrms(EMON_SAMPLES);
        _watts = _irms * MAINS_VOLTAGE;  // Apparent power (VA ≈ W for resistive loads)

        // Noise filtering:
        // Values below threshold are treated as no-load condition
        if (_irms < 0.3f) {
            _irms  = 0.0f;
            _watts = 0.0f;
        }

        Serial.printf(
            "[ENERGY] Irms: %.2f A | Power: %.1f W\n",
            _irms, _watts
        );
    }

    // Returns RMS current (Amperes)
    float getIrms() const {
        return _irms;
    }

    // Returns computed apparent power (Watts)
    float getWatts() const {
        return _watts;
    }

private:
    EnergyMonitor _emon;

    float _irms  = 0.0f;  // RMS current (A)
    float _watts = 0.0f;  // Apparent power (W)
};