#pragma once

// ─── Device ───────────────────────────────────────────────────────────────────
#define DEVICE_NAME_PREFIX   "EnerTrack"
#define FW_VERSION           "1.0.0"

// ─── Pinos ────────────────────────────────────────────────────────────────────
#define PIN_SCT013           34

// ─── SCT-013 / EmonLib ────────────────────────────────────────────────────────
#define EMON_CALIBRATION     60.6
#define EMON_SAMPLES         1480
#define MAINS_VOLTAGE        127.0

// ─── BLE GATT UUIDs ───────────────────────────────────────────────────────────
#define BLE_SERVICE_UUID          "12345678-1234-1234-1234-123456789abc"
#define BLE_CHAR_WIFI_SSID_UUID   "12345678-1234-1234-1234-123456789ab1"
#define BLE_CHAR_WIFI_PASS_UUID   "12345678-1234-1234-1234-123456789ab2"
#define BLE_CHAR_STATUS_UUID      "12345678-1234-1234-1234-123456789ab3"
#define BLE_CHAR_ENERGY_UUID      "12345678-1234-1234-1234-123456789ab4"
#define BLE_CHAR_WIFI_SCAN_UUID   "12345678-1234-1234-1234-123456789ab5"  // NOVO

// ─── NVS ──────────────────────────────────────────────────────────────────────
#define NVS_NAMESPACE    "enertrack"
#define NVS_KEY_SSID     "wifi_ssid"
#define NVS_KEY_PASS     "wifi_pass"
#define NVS_KEY_DEV_ID   "device_id"

// ─── Wi-Fi ────────────────────────────────────────────────────────────────────
#define WIFI_CONNECT_TIMEOUT_MS  15000
#define WIFI_MAX_RETRIES         3

// ─── Timers ───────────────────────────────────────────────────────────────────
#define ENERGY_READ_INTERVAL_MS  5000
#define BLE_ADV_TIMEOUT_SEC      120

// ─── API Cloud ────────────────────────────────────────────────────────────────
#define API_READINGS_URL  "https://enertrack-web.giihvieiratwo.workers.dev/api/readings"