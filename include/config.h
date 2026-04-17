#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Device Configuration
// ─────────────────────────────────────────────────────────────────────────────
#define DEVICE_NAME_PREFIX   "EnerTrack"   // BLE advertising name prefix
#define FW_VERSION           "1.0.0"       // Firmware version identifier

// ─────────────────────────────────────────────────────────────────────────────
// Hardware Pins
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_SCT013           34   // ADC1_CH6 — analog input (SCT-013 current sensor)
// IMPORTANT:
// - Use a proper voltage divider with mid-point bias (≈1.65V)
// - Ensure the signal stays within the 0–3.3V ADC input range

// ─────────────────────────────────────────────────────────────────────────────
// SCT-013 / Energy Measurement (EmonLib)
// ─────────────────────────────────────────────────────────────────────────────
#define EMON_CALIBRATION     60.6  // Calibration constant: (CT turns / burden resistor)
// Example:
// SCT-013-030 (30A / 1V): 30 / 0.495 ≈ 60.6

#define EMON_SAMPLES         1480  // Number of ADC samples per RMS calculation

#define MAINS_VOLTAGE        127.0 // Nominal mains voltage (Volts)
// Typical values in Brazil: 127V or 220V

// ─────────────────────────────────────────────────────────────────────────────
// BLE GATT UUIDs
// ─────────────────────────────────────────────────────────────────────────────
// NOTE:
// - Replace with production UUIDs (e.g., https://uuidgenerator.net)
#define BLE_SERVICE_UUID          "12345678-1234-1234-1234-123456789abc"
#define BLE_CHAR_WIFI_SSID_UUID   "12345678-1234-1234-1234-123456789ab1"
#define BLE_CHAR_WIFI_PASS_UUID   "12345678-1234-1234-1234-123456789ab2"
#define BLE_CHAR_STATUS_UUID      "12345678-1234-1234-1234-123456789ab3"
#define BLE_CHAR_ENERGY_UUID      "12345678-1234-1234-1234-123456789ab4"

// ─────────────────────────────────────────────────────────────────────────────
// Non-Volatile Storage (NVS)
// ─────────────────────────────────────────────────────────────────────────────
#define NVS_NAMESPACE    "enertrack"   // NVS namespace
#define NVS_KEY_SSID     "wifi_ssid"   // Stored Wi-Fi SSID
#define NVS_KEY_PASS     "wifi_pass"   // Stored Wi-Fi password
#define NVS_KEY_DEV_ID   "device_id"   // Unique device identifier

// ─────────────────────────────────────────────────────────────────────────────
// Wi-Fi Configuration
// ─────────────────────────────────────────────────────────────────────────────
#define WIFI_CONNECT_TIMEOUT_MS  15000   // Connection timeout (milliseconds)
#define WIFI_MAX_RETRIES         3       // Maximum retry attempts

// ─────────────────────────────────────────────────────────────────────────────
// Timing Configuration
// ─────────────────────────────────────────────────────────────────────────────
#define ENERGY_READ_INTERVAL_MS  5000    // Energy sampling interval (ms)
#define BLE_ADV_TIMEOUT_SEC      120     // Advertising timeout (seconds)