/**
 * @file Transport.h
 * @brief Transport abstraction layer interface
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "drivers/Data.h"

// Determine default transport type based on config
#if   TRANSPORT == USB
    #define DEFAULT_TRANSPORT_TYPE TransportType::USB
#elif TRANSPORT == PS2
    #define DEFAULT_TRANSPORT_TYPE TransportType::PS2
#elif TRANSPORT == BLE
    #define DEFAULT_TRANSPORT_TYPE TransportType::BLE
#elif TRANSPORT == MULTI
    #define DEFAULT_TRANSPORT_TYPE TransportType::MULTI
#else
    #error "Invalid transport type selected"
#endif

enum class TransportType {
    USB,
    PS2,
    BLE,
    MULTI,
};

class TransportCallbacks {
public:
    virtual ~TransportCallbacks() = default;
    virtual void onConnect() = 0;
    virtual void onDisconnect() = 0;
    virtual void onDataReceived(const uint8_t* data, size_t length) = 0;
};

class Transport {
public:
    virtual ~Transport() = default;
    
    // Transport lifecycle
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual void update() = 0;
    
    // Connection management
    virtual bool isConnected() = 0;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    
    // Data transmission
    virtual bool sendData(const uint8_t* data, size_t length) = 0;
    virtual bool sendReport(uint8_t reportId, const uint8_t* data, size_t length) = 0;
    
    // Service management
    virtual void setDeviceInfo(const char* name, const char* manufacturer, uint16_t vid, uint16_t pid, uint16_t version) = 0;
    virtual void setBatteryLevel(uint8_t level) = 0;
    virtual void setAppearance(uint16_t appearance) = 0;
    
    // Callback registration
    virtual void setCallbacks(TransportCallbacks* callbacks) = 0;
    
    // HID descriptor management
    virtual void setReportMap(const uint8_t* descriptor, size_t length) = 0;
    
    // Service availability
    virtual bool supportsHID() = 0;
};

#endif
