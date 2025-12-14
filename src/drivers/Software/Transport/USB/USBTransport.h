/**
 * @file USBTransport.h
 * @brief USB HID transport implementation for ESP32-S3
 */

#ifndef USBTRANSPORT_H
#define USBTRANSPORT_H

#include "../Transport.h"
#include "../../Log/Log.h"
#include "../../../Appearance.h"
#include <string>

// Use ESP32's built-in USB HID support
#include "USB.h"
#include "USBHID.h"

class USBTransport : public Transport, public USBHIDDevice {
private:
    TransportCallbacks* callbacks;
    std::string deviceName;
    std::string deviceManufacturer;
    uint16_t vid, pid, version;
    uint8_t batteryLevel;
    uint16_t appearance;

    // USB Components
    USBHID hid;
    const uint8_t* reportMap;
    size_t reportMapLength;
    
    bool initialized;
    bool connected;

public:
    USBTransport();
    ~USBTransport();

    // Transport interface implementation
    bool begin() override;
    void end() override;
    void update() override;

    bool isConnected() override;
    bool connect() override;
    void disconnect() override;

    bool sendData(const uint8_t* data, size_t length) override;
    bool sendReport(uint8_t reportId, const uint8_t* data, size_t length) override;

    void setDeviceInfo(const char* name, const char* manufacturer, 
                      uint16_t vid, uint16_t pid, uint16_t version) override;
    void setBatteryLevel(uint8_t level) override;
    void setAppearance(uint16_t appearance) override;
    void setCallbacks(TransportCallbacks* callbacks) override;
    void setReportMap(const uint8_t* descriptor, size_t length) override;

    bool supportsHID() override { return true; }

    // USBHIDDevice interface
    uint16_t _onGetDescriptor(uint8_t* buffer) override;
    void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) override;
};

#endif // USBTRANSPORT_H
