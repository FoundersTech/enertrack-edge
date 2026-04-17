#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "device_state.h"
#include "nvs_manager.h"
#include "ble_provisioning.h"
#include "energy_meter.h"

// ─────────────────────────────────────────────────────────────────────────────
// Global Instances
// ─────────────────────────────────────────────────────────────────────────────
static DeviceState     gState  = DeviceState::BOOT;
static BleProvisioning gBle;
static EnergyMeter     gEnergy;

// Credentials received via BLE (written from BLE callback context)
static volatile bool   gGotCredentials = false;
static String          gPendingSsid;
static String          gPendingPass;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Updates device state with logging
void setState(DeviceState next) {
    Serial.printf("[STATE] %s → %s\n", stateToStr(gState), stateToStr(next));
    gState = next;
}

// Builds a unique BLE device name (e.g., "EnerTrack-A1B2")
String buildDeviceName() {
    String id = NvsManager::getOrCreateDeviceId();
    return String(DEVICE_NAME_PREFIX) + "-" + id.substring(8);  // last 4 chars
}

// Attempts Wi-Fi connection with timeout
bool connectWifi(const String& ssid, const String& pass) {
    Serial.printf("[WIFI] Connecting to: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            WiFi.disconnect(true);
            return false;
        }
        delay(300);
    }

    Serial.printf(
        "[WIFI] Connected! IP: %s\n",
        WiFi.localIP().toString().c_str()
    );

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.printf("\n=== EnerTrack Home v%s ===\n", FW_VERSION);

    // 1. Load Wi-Fi credentials from NVS (if available)
    String ssid, pass;

    if (NvsManager::loadWifi(ssid, pass)) {
        Serial.println("[BOOT] Credentials found in NVS → attempting Wi-Fi");
        setState(DeviceState::WIFI_CONNECTING);

        if (connectWifi(ssid, pass)) {
            gEnergy.begin();
            setState(DeviceState::ONLINE);
            return;
        }

        // Invalid credentials → fallback to BLE provisioning
        Serial.println("[BOOT] Wi-Fi failed — clearing NVS and starting BLE");
        NvsManager::clearAll();
    }

    // 2. No valid credentials → start BLE provisioning
    gEnergy.begin();  // Initialize ADC early (stabilization / warm-up)

    ProvisioningCallbacks cbs;

    cbs.onClientConnected = []() {
        setState(DeviceState::BLE_CONNECTED);
    };

    cbs.onClientDisconnected = []() {
        if (gState == DeviceState::BLE_CONNECTED) {
            setState(DeviceState::BLE_ADVERTISING);
        }
    };

    cbs.onCredentialsReceived = [](const String& ssid, const String& pass) {
        // Defer processing to main loop (avoid heavy work in BLE callback)
        gPendingSsid    = ssid;
        gPendingPass    = pass;
        gGotCredentials = true;
    };

    String devName = buildDeviceName();
    gBle.begin(devName, cbs);

    setState(DeviceState::BLE_ADVERTISING);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main Loop
// ─────────────────────────────────────────────────────────────────────────────
static unsigned long lastEnergyRead = 0;

void loop() {
    switch (gState) {

        // ─────────────────────────────────────────────────────────────────────
        // BLE Provisioning States
        // ─────────────────────────────────────────────────────────────────────
        case DeviceState::BLE_ADVERTISING:
        case DeviceState::BLE_CONNECTED:

            if (gGotCredentials) {
                gGotCredentials = false;

                setState(DeviceState::WIFI_CONNECTING);
                gBle.notifyStatus("connecting");

                if (connectWifi(gPendingSsid, gPendingPass)) {
                    NvsManager::saveWifi(gPendingSsid, gPendingPass);

                    gBle.notifyStatus("wifi_ok");
                    delay(500);  // Allow app to receive notification

                    gBle.stop();
                    setState(DeviceState::ONLINE);

                } else {
                    gBle.notifyStatus("wifi_fail");
                    setState(DeviceState::BLE_CONNECTED);
                }
            }
            break;

        // ─────────────────────────────────────────────────────────────────────
        // Normal Operation (Online)
        // ─────────────────────────────────────────────────────────────────────
        case DeviceState::ONLINE:

            // Periodic energy measurement
            if (millis() - lastEnergyRead >= ENERGY_READ_INTERVAL_MS) {
                lastEnergyRead = millis();

                gEnergy.read();

                // Future: publish to cloud (MQTT / HTTP)
                // Current: notify via BLE if client is connected
                if (gBle.isConnected()) {
                    gBle.notifyEnergy(
                        gEnergy.getIrms(),
                        gEnergy.getWatts()
                    );
                }
            }

            // Detect Wi-Fi disconnection
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WIFI] Connection lost — reconnecting...");
                setState(DeviceState::WIFI_CONNECTING);

                String ssid, pass;

                if (NvsManager::loadWifi(ssid, pass) &&
                    connectWifi(ssid, pass)) {

                    setState(DeviceState::ONLINE);

                } else {
                    // Recovery strategy: clear credentials and restart
                    NvsManager::clearAll();
                    ESP.restart();  // Soft factory reset
                }
            }
            break;

        // ─────────────────────────────────────────────────────────────────────
        // Transitional State
        // ─────────────────────────────────────────────────────────────────────
        case DeviceState::WIFI_CONNECTING:
            // Transitional state handled externally
            delay(100);
            break;

        default:
            break;
    }
}