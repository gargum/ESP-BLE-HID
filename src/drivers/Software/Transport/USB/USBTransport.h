/**
 * @file USBTransport.h
 * @brief USB HID transport implementation
 */

#ifndef USBTRANSPORT_H
#define USBTRANSPORT_H

#include "../../../Appearance.h"
#include "../../Log/Log.h"
#include "../Transport.h"

#if __has_include(<USB.h>)
#include <USB.h>

#if __has_include(<USBHID.h>)
#include <USBHID.h>
#endif

class USBTransport : public Transport {
private:
    TransportCallbacks* callbacks;
    std::string deviceName;
    std::string deviceManufacturer;
    uint16_t vid;
    uint16_t pid;
    uint16_t version;
    bool connected;
    uint8_t batteryLevel;
    uint16_t appearance;
    
    #ifdef USB_HID_AVAILABLE
      USBHID *hidDevice;
    #endif
    
    const uint8_t* reportMap;
    size_t reportMapLength;
    bool hidDeviceInitialized;

public:
    USBTransport();
    ~USBTransport();
    
    // Transport lifecycle
    bool begin() override;
    void end() override;
    void update() override;
    
    // Connection management
    bool isConnected() override;
    bool connect() override;
    void disconnect() override;
    
    // Data transmission
    bool sendData(const uint8_t* data, size_t length) override;
    bool sendReport(uint8_t reportId, const uint8_t* data, size_t length) override;
    
    // Service management
    void setDeviceInfo(const char* name, const char* manufacturer, uint16_t vid, uint16_t pid, uint16_t version) override;
    void setBatteryLevel(uint8_t level) override;
    void setAppearance(uint16_t appearance) override;
    
    // Callback registration
    void setCallbacks(TransportCallbacks* callbacks) override;
    
    // HID descriptor management (for compatibility with Transport interface)
    void setReportMap(const uint8_t* descriptor, size_t length) override;
    
    // Service availability
    bool supportsHID() override;
    
    // Service reinitialization
    bool reinitializeHID();
};

#endif
#endif
