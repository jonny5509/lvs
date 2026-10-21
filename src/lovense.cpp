#include <Arduino.h>
#include <esp_mac.h>
#include "lovense.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "muse.h"

/*
 * Community-documented Lovense BLE profile:
 *   FFF0 service
 *   FFF1 response/notification characteristic (RX from toy)
 *   FFF2 command/write characteristic (TX to toy)
 *
 * This profile is a compatibility target, not proof that Lovense Connect
 * accepts a generic ESP32 peripheral. The official app may require additional
 * identity, advertising, or protocol behavior.
 */

#define LOVENSE_SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fb"
#define LOVENSE_RX_CHARACTERISTIC_UUID "0000fff1-0000-1000-8000-00805f9b34fb"
#define LOVENSE_TX_CHARACTERISTIC_UUID "0000fff2-0000-1000-8000-00805f9b34fb"

static NimBLECharacteristic *pLovenseRxCharacteristic = nullptr;

class ConnectionEventHandler : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *pServer) override {
        (void)pServer;
        // A BLE peripheral normally stops advertising when a central connects.
        // Re-arm advertising so compatible MuSE/LoveSpouse toys can continue
        // to receive manufacturer-data control broadcasts while the phone is
        // connected. This behavior still needs verification on the target toys.
        NimBLEDevice::startAdvertising();
    }

    void onDisconnect(NimBLEServer *pServer) override {
        (void)pServer;
        // Allow the phone to reconnect after it disconnects.
        NimBLEDevice::startAdvertising();
    }
};

std::string generate_response(const std::string& command) {
    if (command == "DeviceType;") {
        return get_device_info();
    } else if (command == "Battery;") {
        return get_battery_level();
    } else if (command == "Status:1;") {
        return get_device_status();
    } else if (command.find("AutoSwitch:") == 0) {
        return set_auto_switch_options(command);
    } else if (command.find("Vibrate:") == 0) {
        return set_vibration_speed(command);
    } else if (command == "PowerOff;") {
        return set_vibration_speed("Vibrate:0;");
    }
    return "OK;";
}

std::string get_device_info() {
    const std::string firmwareVersion = "01";
    const std::string deviceType = "G";

    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char macAddressStr[13];
    snprintf(macAddressStr, sizeof(macAddressStr), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return deviceType + ":" + firmwareVersion + ":" + macAddressStr + ";";
}

std::string get_battery_level() {
    // Placeholder: this board has no battery gauge wired into the firmware.
    return "100;";
}

std::string get_device_status() {
    return "2;";
}

bool autoSwitchTurnOff = true;
bool autoSwitchLastLevel = true;

std::string get_auto_switch_options() {
    std::string response = "AutoSwitch:";
    response += autoSwitchTurnOff ? "1:" : "0:";
    response += autoSwitchLastLevel ? "1;" : "0;";
    return response;
}

std::string set_auto_switch_options(const std::string& command) {
    if (command == "AutoSwitch:On:Off;") {
        autoSwitchTurnOff = true;
        autoSwitchLastLevel = false;
    } else if (command == "AutoSwitch:On:On;") {
        autoSwitchTurnOff = true;
        autoSwitchLastLevel = true;
    } else if (command == "AutoSwitch:Off:On;") {
        autoSwitchTurnOff = false;
        autoSwitchLastLevel = true;
    } else if (command == "AutoSwitch:Off:Off;") {
        autoSwitchTurnOff = false;
        autoSwitchLastLevel = false;
    }
    return "OK;";
}

std::string set_vibration_speed(const std::string& command) {
    const size_t colon = command.find(':');
    const size_t end = command.find(';');
    if (colon == std::string::npos || end == std::string::npos || end <= colon + 1) {
        return "Invalid command;";
    }

    int speedInt = 0;
    try {
        speedInt = std::stoi(command.substr(colon + 1, end - colon - 1));
    } catch (...) {
        return "Invalid command;";
    }

    speedInt = constrain(speedInt, 0, 20);
    muse_set_intensity(static_cast<float>(speedInt) / 20.0f);
    return "OK;";
}

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic *pCharacteristic) override {
        const std::string command = pCharacteristic->getValue();
        if (command.empty() || pLovenseRxCharacteristic == nullptr) {
            return;
        }

        const std::string response = generate_response(command);
        pLovenseRxCharacteristic->setValue(response);
        pLovenseRxCharacteristic->notify();
    }
};

void lovense_init() {
    NimBLEServer *pServer = NimBLEDevice::getServer();
    if (pServer == nullptr) {
        return;
    }

    pServer->setCallbacks(new ConnectionEventHandler());

    NimBLEService *pService = pServer->createService(LOVENSE_SERVICE_UUID);

    // Responses are read/notify; commands arrive on a separate write endpoint.
    pLovenseRxCharacteristic = pService->createCharacteristic(
        LOVENSE_RX_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    NimBLECharacteristic *pLovenseTxCharacteristic = pService->createCharacteristic(
        LOVENSE_TX_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    pLovenseRxCharacteristic->setValue("OK;");
    pLovenseTxCharacteristic->setCallbacks(new CharacteristicCallbacks());

    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(LOVENSE_SERVICE_UUID);
}
