#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "config.h"
#include "device_state.h"
#include "nvs_manager.h"
#include "ble_provisioning.h"
#include "energy_meter.h"

static DeviceState     gState  = DeviceState::BOOT;
static BleProvisioning gBle;
static EnergyMeter     gEnergy;

static volatile bool   gGotCredentials = false;
static String          gPendingSsid;
static String          gPendingPass;

void setState(DeviceState next) {
    Serial.printf("[STATE] %s → %s\n", stateToStr(gState), stateToStr(next));
    gState = next;
}

String buildDeviceName() {
    String id = NvsManager::getOrCreateDeviceId();
    return String(DEVICE_NAME_PREFIX) + "-" + id.substring(8);
}

bool connectWifi(const String& ssid, const String& pass) {
    Serial.printf("[WIFI] Conectando a: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            WiFi.disconnect(true);
            return false;
        }
        delay(300);
    }
    Serial.printf("[WIFI] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

void postReading(float irms, float watts) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (strlen(API_READINGS_URL) == 0)  return;
    HTTPClient http;
    http.begin(API_READINGS_URL);
    http.addHeader("Content-Type", "application/json");
    String mac  = NvsManager::getOrCreateDeviceId();
    String body = "{\"mac_address\":\"" + mac + "\","
                  "\"irms\":"  + String(irms, 3) + ","
                  "\"watts\":" + String(watts, 2) + "}";
    int code = http.POST(body);
    http.end();
    Serial.printf("[HTTP] POST readings → %d\n", code);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("\n=== EnerTrack Home v%s ===\n", FW_VERSION);

    String ssid, pass;
    if (NvsManager::loadWifi(ssid, pass)) {
        setState(DeviceState::WIFI_CONNECTING);
        if (connectWifi(ssid, pass)) {
            gEnergy.begin();
            setState(DeviceState::ONLINE);
            return;
        }
        NvsManager::clearAll();
    }

    gEnergy.begin();

    ProvisioningCallbacks cbs;
    cbs.onClientConnected    = []() { setState(DeviceState::BLE_CONNECTED); };
    cbs.onClientDisconnected = []() {
        if (gState == DeviceState::BLE_CONNECTED)
            setState(DeviceState::BLE_ADVERTISING);
    };
    cbs.onCredentialsReceived = [](const String& ssid, const String& pass) {
        gPendingSsid    = ssid;
        gPendingPass    = pass;
        gGotCredentials = true;
    };

    gBle.begin(buildDeviceName(), cbs);
    setState(DeviceState::BLE_ADVERTISING);
}

static unsigned long lastEnergyRead = 0;

void loop() {
    switch (gState) {

        case DeviceState::BLE_ADVERTISING:
        case DeviceState::BLE_CONNECTED:

            // Scan Wi-Fi solicitado pelo app
            if (gBle.pendingWifiScan) {
                gBle.pendingWifiScan = false;
                gBle.doWifiScan();
            }

            if (gGotCredentials) {
                gGotCredentials = false;
                setState(DeviceState::WIFI_CONNECTING);
                gBle.notifyStatus("connecting");

                if (connectWifi(gPendingSsid, gPendingPass)) {
                    NvsManager::saveWifi(gPendingSsid, gPendingPass);
                    gBle.notifyStatus("wifi_ok");
                    delay(500);
                    gBle.stop();
                    setState(DeviceState::ONLINE);
                } else {
                    gBle.notifyStatus("wifi_fail");
                    setState(DeviceState::BLE_CONNECTED);
                }
            }
            break;

        case DeviceState::ONLINE:
            if (millis() - lastEnergyRead >= ENERGY_READ_INTERVAL_MS) {
                lastEnergyRead = millis();
                gEnergy.read();
                postReading(gEnergy.getIrms(), gEnergy.getWatts());
                if (gBle.isConnected())
                    gBle.notifyEnergy(gEnergy.getIrms(), gEnergy.getWatts());
            }
            if (WiFi.status() != WL_CONNECTED) {
                setState(DeviceState::WIFI_CONNECTING);
                String ssid, pass;
                if (NvsManager::loadWifi(ssid, pass) && connectWifi(ssid, pass))
                    setState(DeviceState::ONLINE);
                else { NvsManager::clearAll(); ESP.restart(); }
            }
            break;

        case DeviceState::WIFI_CONNECTING:
            delay(100);
            break;

        default: break;
    }
}