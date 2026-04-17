#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Device State Machine
//
// Defines the lifecycle states of the device, inspired by typical IoT
// provisioning flows (e.g., Mibo / Intelbras pattern).
//
// State transitions are driven by:
// - NVS availability (stored credentials)
// - BLE client interaction
// - Wi-Fi connection results
// ─────────────────────────────────────────────────────────────────────────────
enum class DeviceState : uint8_t {
    BOOT,              // System initialization; loading configuration from NVS

    BLE_ADVERTISING,   // No Wi-Fi credentials available; advertising via BLE
                       // awaiting provisioning from mobile application

    BLE_CONNECTED,     // BLE client connected; receiving provisioning data

    WIFI_CONNECTING,   // Attempting to connect to Wi-Fi using provided credentials

    ONLINE,            // Successfully connected to Wi-Fi; normal operation
                       // (energy monitoring and data transmission)

    ERROR              // Critical failure state; requires reset or intervention
};

// ─────────────────────────────────────────────────────────────────────────────
// Utility: State to String Conversion
//
// Converts a DeviceState enum value to a human-readable string.
// Useful for logging, debugging, and telemetry.
// ─────────────────────────────────────────────────────────────────────────────
inline const char* stateToStr(DeviceState s) {
    switch (s) {
        case DeviceState::BOOT:            return "BOOT";
        case DeviceState::BLE_ADVERTISING: return "BLE_ADVERTISING";
        case DeviceState::BLE_CONNECTED:   return "BLE_CONNECTED";
        case DeviceState::WIFI_CONNECTING: return "WIFI_CONNECTING";
        case DeviceState::ONLINE:          return "ONLINE";
        case DeviceState::ERROR:           return "ERROR";
        default:                           return "UNKNOWN";
    }
}