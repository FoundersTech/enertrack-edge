#pragma once
#include <Arduino.h>
#include <Preferences.h>   // Arduino wrapper for ESP32 NVS
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// NVS Manager (Non-Volatile Storage)
//
// Provides a simple abstraction layer over ESP32 NVS using Preferences.
//
// Responsibilities:
// - Store and retrieve Wi-Fi credentials
// - Handle factory reset (namespace clear)
// - Generate and persist a unique device identifier
//
// Notes:
// - Data is stored under a dedicated namespace (NVS_NAMESPACE)
// - Preferences automatically handles flash persistence
// ─────────────────────────────────────────────────────────────────────────────
class NvsManager {
public:

    // Stores Wi-Fi credentials in NVS
    static bool saveWifi(const String& ssid, const String& password) {
        Preferences prefs;

        if (!prefs.begin(NVS_NAMESPACE, false)) {
            return false;
        }

        prefs.putString(NVS_KEY_SSID, ssid);
        prefs.putString(NVS_KEY_PASS, password);
        prefs.end();

        Serial.printf("[NVS] Wi-Fi credentials saved — SSID: %s\n", ssid.c_str());
        return true;
    }

    // Loads Wi-Fi credentials from NVS
    //
    // Returns:
    // - true  → valid credentials found
    // - false → no credentials stored or read failure
    static bool loadWifi(String& ssid, String& password) {
        Preferences prefs;

        if (!prefs.begin(NVS_NAMESPACE, true)) {  // read-only mode
            return false;
        }

        ssid     = prefs.getString(NVS_KEY_SSID, "");
        password = prefs.getString(NVS_KEY_PASS, "");

        prefs.end();

        return ssid.length() > 0;
    }

    // Clears all stored data in the namespace (factory reset)
    static void clearAll() {
        Preferences prefs;

        if (prefs.begin(NVS_NAMESPACE, false)) {
            prefs.clear();
            prefs.end();
        }

        Serial.println("[NVS] Namespace cleared — factory reset");
    }

    // Retrieves or generates a persistent unique device ID
    //
    // Strategy:
    // - If ID exists in NVS → return it
    // - Otherwise → generate from device MAC address and store it
    //
    // Format:
    // - 12-character hexadecimal string (e.g., "A1B2C3D4E5F6")
    static String getOrCreateDeviceId() {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, false);

        String id = prefs.getString(NVS_KEY_DEV_ID, "");

        if (id.isEmpty()) {
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_WIFI_STA);

            char buf[13];
            snprintf(
                buf, sizeof(buf),
                "%02X%02X%02X%02X%02X%02X",
                mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]
            );

            id = String(buf);
            prefs.putString(NVS_KEY_DEV_ID, id);
        }

        prefs.end();
        return id;
    }
};