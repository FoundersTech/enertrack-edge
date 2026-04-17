#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Callback structure used by main.cpp
// ─────────────────────────────────────────────────────────────────────────────
struct ProvisioningCallbacks {
    // Triggered when both SSID and password are received via BLE
    std::function<void(const String& ssid, const String& pass)> onCredentialsReceived;

    // Triggered when a BLE client connects
    std::function<void()> onClientConnected;

    // Triggered when a BLE client disconnects
    std::function<void()> onClientDisconnected;
};

// ─────────────────────────────────────────────────────────────────────────────
// BLE Provisioning Service
//
// Implements Wi-Fi provisioning over BLE using a GATT server.
// Pattern inspired by common IoT provisioning flows (e.g., Mibo-style).
//
// Responsibilities:
// - Receive Wi-Fi credentials via GATT write
// - Notify device status to the mobile app
// - Stream energy readings via BLE notifications
// ─────────────────────────────────────────────────────────────────────────────
class BleProvisioning {
public:

    // Initializes BLE stack, GATT server, and advertising
    void begin(const String& deviceName, ProvisioningCallbacks cbs) {
        _cbs = cbs;

        NimBLEDevice::init(deviceName.c_str());
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Max TX power

        _server = NimBLEDevice::createServer();
        _server->setCallbacks(new ServerCB(this));

        // Create provisioning service
        NimBLEService* svc = _server->createService(BLE_SERVICE_UUID);

        // Characteristic: Wi-Fi SSID (WRITE)
        _charSsid = svc->createCharacteristic(
            BLE_CHAR_WIFI_SSID_UUID,
            NIMBLE_PROPERTY::WRITE
        );
        _charSsid->setCallbacks(new WriteCB(this, false));

        // Characteristic: Wi-Fi Password (WRITE)
        _charPass = svc->createCharacteristic(
            BLE_CHAR_WIFI_PASS_UUID,
            NIMBLE_PROPERTY::WRITE
        );
        _charPass->setCallbacks(new WriteCB(this, true));

        // Characteristic: Device status (READ + NOTIFY)
        _charStatus = svc->createCharacteristic(
            BLE_CHAR_STATUS_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        // Characteristic: Energy readings (READ + NOTIFY)
        _charEnergy = svc->createCharacteristic(
            BLE_CHAR_ENERGY_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        svc->start();

        // Configure advertising
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        adv->addServiceUUID(BLE_SERVICE_UUID);
        adv->setScanResponse(true);
        adv->start(BLE_ADV_TIMEOUT_SEC);

        Serial.printf("[BLE] Advertising started: %s\n", deviceName.c_str());
    }

    // Sends device status to the connected client
    // Example values: "connecting", "wifi_ok", "wifi_fail"
    void notifyStatus(const String& status) {
        if (_charStatus && _server->getConnectedCount() > 0) {
            _charStatus->setValue(status.c_str());
            _charStatus->notify();
            Serial.printf("[BLE] Status notified: %s\n", status.c_str());
        }
    }

    // Sends energy measurement data via BLE notification
    // Payload format: JSON { "irms": float, "watts": float }
    void notifyEnergy(float irms, float watts) {
        if (_charEnergy && _server->getConnectedCount() > 0) {
            JsonDocument doc;
            doc["irms"]  = irms;
            doc["watts"] = watts;

            String json;
            serializeJson(doc, json);

            _charEnergy->setValue(json.c_str());
            _charEnergy->notify();
        }
    }

    // Stops BLE advertising
    void stop() {
        NimBLEDevice::getAdvertising()->stop();
    }

    // Returns true if at least one BLE client is connected
    bool isConnected() {
        return _server && _server->getConnectedCount() > 0;
    }

private:
    NimBLEServer*         _server     = nullptr;
    NimBLECharacteristic* _charSsid   = nullptr;
    NimBLECharacteristic* _charPass   = nullptr;
    NimBLECharacteristic* _charStatus = nullptr;
    NimBLECharacteristic* _charEnergy = nullptr;

    ProvisioningCallbacks _cbs;

    String _pendingSsid;
    String _pendingPass;

    // Handles SSID write event
    void onSsidWrite(const String& val) {
        _pendingSsid = val;
        tryProvision();
    }

    // Handles password write event
    void onPassWrite(const String& val) {
        _pendingPass = val;
        tryProvision();
    }

    // Triggers provisioning only when both SSID and password are available
    void tryProvision() {
        if (_pendingSsid.length() > 0 && _pendingPass.length() > 0) {
            if (_cbs.onCredentialsReceived) {
                _cbs.onCredentialsReceived(_pendingSsid, _pendingPass);
            }
            _pendingSsid = "";
            _pendingPass = "";
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Internal BLE callbacks
    // ─────────────────────────────────────────────────────────────────────────

    // Server connection lifecycle callbacks
    struct ServerCB : public NimBLEServerCallbacks {
        BleProvisioning* _p;

        ServerCB(BleProvisioning* p) : _p(p) {}

        void onConnect(NimBLEServer*) override {
            Serial.println("[BLE] Client connected");
            if (_p->_cbs.onClientConnected) {
                _p->_cbs.onClientConnected();
            }
        }

        void onDisconnect(NimBLEServer*) override {
            Serial.println("[BLE] Client disconnected");

            if (_p->_cbs.onClientDisconnected) {
                _p->_cbs.onClientDisconnected();
            }

            // Restart advertising after disconnection
            NimBLEDevice::getAdvertising()->start(BLE_ADV_TIMEOUT_SEC);
        }
    };

    // Characteristic write callback (SSID / Password)
    struct WriteCB : public NimBLECharacteristicCallbacks {
        BleProvisioning* _p;
        bool _isPass;

        WriteCB(BleProvisioning* p, bool isPass)
            : _p(p), _isPass(isPass) {}

        void onWrite(NimBLECharacteristic* c) override {
            String val = c->getValue().c_str();

            if (_isPass) {
                _p->onPassWrite(val);
            } else {
                _p->onSsidWrite(val);
            }
        }
    };
};