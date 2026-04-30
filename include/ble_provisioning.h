#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"

struct ProvisioningCallbacks {
    std::function<void(const String& ssid, const String& pass)> onCredentialsReceived;
    std::function<void()> onClientConnected;
    std::function<void()> onClientDisconnected;
};

class BleProvisioning {
public:

    void begin(const String& deviceName, ProvisioningCallbacks cbs) {
        _cbs = cbs;

        NimBLEDevice::init(deviceName.c_str());
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);

        _server = NimBLEDevice::createServer();
        _server->setCallbacks(new ServerCB(this));

        NimBLEService* svc = _server->createService(BLE_SERVICE_UUID);

        // SSID write
        _charSsid = svc->createCharacteristic(BLE_CHAR_WIFI_SSID_UUID, NIMBLE_PROPERTY::WRITE);
        _charSsid->setCallbacks(new WriteCB(this, false));

        // Password write
        _charPass = svc->createCharacteristic(BLE_CHAR_WIFI_PASS_UUID, NIMBLE_PROPERTY::WRITE);
        _charPass->setCallbacks(new WriteCB(this, true));

        // Status notify
        _charStatus = svc->createCharacteristic(
            BLE_CHAR_STATUS_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        // Energy notify
        _charEnergy = svc->createCharacteristic(
            BLE_CHAR_ENERGY_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );

        // Wi-Fi scan: write "scan" para solicitar, notify devolve JSON com redes
        _charWifiScan = svc->createCharacteristic(
            BLE_CHAR_WIFI_SCAN_UUID,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        _charWifiScan->setCallbacks(new ScanCB(this));

        svc->start();

        // UUID no advertising primário (Web Bluetooth filtra aqui)
        // Nome completo no scan response (aparece no picker do browser)
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        adv->addServiceUUID(BLE_SERVICE_UUID);
        adv->setMinPreferred(0x06);
        adv->setMaxPreferred(0x12);

        NimBLEAdvertisementData scanData;
        scanData.setName(deviceName.c_str());
        adv->setScanResponseData(scanData);

        adv->start(0);

        Serial.printf("[BLE] Advertising: %s\n", deviceName.c_str());
    }

    // Executa scan Wi-Fi e envia resultado via notify
    // Chamado quando o app escreve "scan" na característica WIFI_SCAN
    void doWifiScan() {
        Serial.println("[WIFI] Iniciando scan de redes...");
        notifyStatus("scanning");

        // Scan síncrono — bloqueia ~2s mas garante resultado completo
        int n = WiFi.scanNetworks();

        if (n <= 0) {
            notifyStatus("scan_empty");
            return;
        }

        // Envia em chunks de 3 redes (limite MTU BLE ~512 bytes)
        // Ordena por RSSI (já vem ordenado pelo ESP32)
        const int CHUNK = 3;
        for (int start = 0; start < n; start += CHUNK) {
            JsonDocument doc;
            JsonArray arr = doc["networks"].to<JsonArray>();

            for (int i = start; i < min(start + CHUNK, n); i++) {
                JsonObject net = arr.add<JsonObject>();
                net["ssid"]   = WiFi.SSID(i).c_str();
                net["rssi"]   = WiFi.RSSI(i);
                net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            }

            // Indica se há mais chunks
            doc["more"] = (start + CHUNK < n);
            doc["total"] = n;

            String json;
            serializeJson(doc, json);

            if (_charWifiScan && _server->getConnectedCount() > 0) {
                _charWifiScan->setValue(json.c_str());
                _charWifiScan->notify();
                delay(100); // Pequena pausa entre chunks
            }

            Serial.printf("[WIFI] Enviado chunk %d/%d\n", start/CHUNK + 1, (n + CHUNK - 1)/CHUNK);
        }

        WiFi.scanDelete();
        Serial.printf("[WIFI] Scan concluído — %d redes encontradas\n", n);
    }

    void notifyStatus(const String& status) {
        if (_charStatus && _server->getConnectedCount() > 0) {
            _charStatus->setValue(status.c_str());
            _charStatus->notify();
            Serial.printf("[BLE] Status: %s\n", status.c_str());
        }
    }

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

    void stop() { NimBLEDevice::getAdvertising()->stop(); }
    bool isConnected() { return _server && _server->getConnectedCount() > 0; }

    // Flag para o loop principal executar o scan (evita bloqueio no callback BLE)
    volatile bool pendingWifiScan = false;

private:
    NimBLEServer*         _server       = nullptr;
    NimBLECharacteristic* _charSsid     = nullptr;
    NimBLECharacteristic* _charPass     = nullptr;
    NimBLECharacteristic* _charStatus   = nullptr;
    NimBLECharacteristic* _charEnergy   = nullptr;
    NimBLECharacteristic* _charWifiScan = nullptr;
    ProvisioningCallbacks _cbs;
    String _pendingSsid;
    String _pendingPass;

    void onSsidWrite(const String& val) { _pendingSsid = val; tryProvision(); }
    void onPassWrite(const String& val) { _pendingPass = val; tryProvision(); }

    void tryProvision() {
        if (_pendingSsid.length() > 0 && _pendingPass.length() > 0) {
            if (_cbs.onCredentialsReceived)
                _cbs.onCredentialsReceived(_pendingSsid, _pendingPass);
            _pendingSsid = "";
            _pendingPass = "";
        }
    }

    struct ServerCB : public NimBLEServerCallbacks {
        BleProvisioning* _p;
        ServerCB(BleProvisioning* p) : _p(p) {}
        void onConnect(NimBLEServer*) override {
            Serial.println("[BLE] Client connected");
            if (_p->_cbs.onClientConnected) _p->_cbs.onClientConnected();
        }
        void onDisconnect(NimBLEServer*) override {
            Serial.println("[BLE] Client disconnected");
            if (_p->_cbs.onClientDisconnected) _p->_cbs.onClientDisconnected();
            NimBLEDevice::getAdvertising()->start(0);
        }
    };

    struct WriteCB : public NimBLECharacteristicCallbacks {
        BleProvisioning* _p;
        bool _isPass;
        WriteCB(BleProvisioning* p, bool isPass) : _p(p), _isPass(isPass) {}
        void onWrite(NimBLECharacteristic* c) override {
            String val = c->getValue().c_str();
            if (_isPass) _p->onPassWrite(val);
            else         _p->onSsidWrite(val);
        }
    };

    // Callback para a característica de scan Wi-Fi
    struct ScanCB : public NimBLECharacteristicCallbacks {
        BleProvisioning* _p;
        ScanCB(BleProvisioning* p) : _p(p) {}
        void onWrite(NimBLECharacteristic* c) override {
            String cmd = c->getValue().c_str();
            cmd.trim();
            if (cmd == "scan") {
                // Sinaliza para o loop principal — não bloqueia o callback BLE
                _p->pendingWifiScan = true;
            }
        }
    };
};