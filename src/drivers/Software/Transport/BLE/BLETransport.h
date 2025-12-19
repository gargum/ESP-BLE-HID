/**
 * @file BLETransport.h
 * @brief BLE-specific transport implementation
 */

#ifndef BLETRANSPORT_H
#define BLETRANSPORT_H

#include "../Transport.h"

#include "HIDTypes.h"

#if __has_include("NimBLEAdvertising.h")
#include "NimBLEAdvertising.h"

#if __has_include("NimBLEDevice.h")
#include "NimBLEDevice.h"
#endif

#if __has_include("NimBLEHIDDevice.h")
#include "NimBLEHIDDevice.h"
#endif

#if __has_include("NimBLEService.h")
#include "NimBLEService.h"
#endif

class BLETransport : public Transport, 
                     public NimBLEServerCallbacks, 
                     public NimBLECharacteristicCallbacks {
private:
    // HID components
    const uint8_t* reportMap;
    size_t reportMapLength;
    
    // BLE components
    NimBLEServer*         server;
    NimBLEHIDDevice*      hidDevice;
    NimBLEAdvertising*    advertising;
    
    NimBLECharacteristic* inputNKRO;
    #if MEDIA_ENABLE
    NimBLECharacteristic* inputMediaKeys;
    #endif
    #if SPACEMOUSE_ENABLE
    NimBLECharacteristic* inputSpacetrans;
    NimBLECharacteristic* inputSpacerotat;
    NimBLECharacteristic* inputSpaceclick;
    #else
    #if MOUSE_ENABLE
    NimBLECharacteristic* inputMouse;
    #endif
    #if DIGITIZER_ENABLE
    NimBLECharacteristic* inputDigitizer;
    #endif
    #if GAMEPAD_ENABLE
    NimBLECharacteristic* inputGamepad;
    #endif
    #endif
    #if STENO_ENABLE
    NimBLECharacteristic* inputSteno;
    #endif
    NimBLECharacteristic* outputKeyboard;
    
    // Callbacks
    TransportCallbacks*   transportCallbacks;
    
    // Device info
    std::string deviceName;
    std::string deviceManufacturer;
    uint16_t vid, pid, version;
    uint8_t batteryLevel;
    uint16_t appearance;
    
    // State
    bool initialized;
    bool connected;
    
public:
    BLETransport();
    virtual ~BLETransport();
    
    // Transport interface implementation
    bool begin() override;
    void end() override;
    void update() override;
    
    bool isConnected() override;
    bool connect() override;
    void disconnect() override;
    
    bool sendData(const uint8_t* data, size_t length) override;
    bool sendReport(uint8_t reportId, const uint8_t* data, size_t length) override;
    
    void setDeviceInfo(const char* name, const char* manufacturer, uint16_t vid, uint16_t pid, uint16_t version) override;
    void setBatteryLevel(uint8_t level) override;
    void setAppearance(uint16_t appearance) override;
    void setCallbacks(TransportCallbacks* callbacks) override;
    void setReportMap(const uint8_t* descriptor, size_t length) override;
    
    bool supportsHID() override { return true; }
    
    // BLE-specific methods
    NimBLEHIDDevice* getHIDDevice() { return hidDevice; }
    
    void verifyCharacteristicHandles();
    void debugCharacteristics();
    void createHIDService();
    bool startAdvertising();
    
    // BLE callbacks
    void onConnect(NimBLEServer* pServer);
    void onDisconnect(NimBLEServer* pServer);
    void onWrite(NimBLECharacteristic* characteristic);
    void onSubscribe(NimBLEServer* pServer, ble_gap_conn_desc* desc, uint16_t attr_handle);
    void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc);
};

#endif
#endif
