/**
 * @file USBTransport.cpp
 * @brief USB HID transport implementation
 */

#include "USBTransport.h"

static const char* USB_TAG = "USBTransport";

USBTransport::USBTransport() 
    : callbacks(nullptr)
    , deviceName("SquidHID")
    , deviceManufacturer("SquidHID")
    , vid(0x046D)
    , pid(0xC52B)
    , version(0x0310)
    , connected(false)
    , batteryLevel(100)
    , appearance(KEYBOARD)
    #ifdef USB_HID_AVAILABLE
    , hidDevice(nullptr)
    #endif
    , reportMap(nullptr)
    , reportMapLength(0)
{
}

USBTransport::~USBTransport() {
    end();
}

bool USBTransport::begin() {
    SQUID_LOG_INFO(USB_TAG, "Initializing USB HID transport");
    
    #if defined(USB_HID_AVAILABLE) && __has_include(<USBHID.h>)
    // Initialize USB
    #if defined(ARDUINO_ARCH_ESP32)
        USB.begin();
    #elif defined(ARDUINO_ARCH_RP2040)
        USB.begin();
    #else
        USB.begin();
    #endif
    
    // Create HID device
    hidDevice = new USBHID();
    if (!hidDevice) {
        SQUID_LOG_ERROR(USB_TAG, "Failed to create USB HID device");
        return false;
    }
    
    // Set report map if provided
    if (reportMap && reportMapLength > 0) {
        SQUID_LOG_DEBUG(USB_TAG, "HID report map available (length: %zu)", reportMapLength);
    }
    
    // USB is considered connected when initialized
    connected = true;
    
    if (callbacks) {
        callbacks->onConnect();
    }
    
    SQUID_LOG_INFO(USB_TAG, "USB HID transport initialized successfully");
    return true;
    #else
    SQUID_LOG_ERROR(USB_TAG, "USB HID not supported on this platform");
    return false;
    #endif
}

void USBTransport::end() {
    SQUID_LOG_INFO(USB_TAG, "Shutting down USB HID transport");
    
    #if defined(USB_HID_AVAILABLE) && __has_include(<USBHID.h>)
    if (hidDevice) {
        delete hidDevice;
        hidDevice = nullptr;
    }
    
    #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
    USB.end();
    #endif
    #endif
    
    bool wasConnected = connected;
    connected = false;
    
    if (wasConnected && callbacks) {
        callbacks->onDisconnect();
    }
    
    SQUID_LOG_INFO(USB_TAG, "USB HID transport shut down");
}

void USBTransport::update() {
    // Haven't implemented this part yet, I'm sorry :(
}

bool USBTransport::isConnected() {
    return connected;
}

bool USBTransport::connect() {
    // USB connection is established during begin() so this isn't really doing anything rn
    if (!connected) {
        SQUID_LOG_WARN(USB_TAG, "USB not initialized - call begin() first");
        return false;
    }
    return true;
}

void USBTransport::disconnect() {
    // USB can't be disconnected without deinitializing so, yeah that's all this is doing
    SQUID_LOG_DEBUG(USB_TAG, "USB disconnect requested - calling end()");
    end();
}

bool USBTransport::sendData(const uint8_t* data, size_t length) {
    // Generic data send - use report ID 0 by default
    return sendReport(0, data, length);
}

bool USBTransport::sendReport(uint8_t reportId, const uint8_t* data, size_t length) {
    #if defined(USB_HID_AVAILABLE) && __has_include(<USBHID.h>)
    if (!connected || !hidDevice) {
        SQUID_LOG_DEBUG(USB_TAG, "Cannot send report - USB not connected");
        return false;
    }
    
    bool result = hidDevice->SendReport(reportId, data, length);
    if (!result) {
        SQUID_LOG_WARN(USB_TAG, "Failed to send HID report %d, length: %zu", reportId, length);
    } else {
        SQUID_LOG_DEBUG(USB_TAG, "Sent HID report %d, length: %zu", reportId, length);
    }
    return result;
    #else
    SQUID_LOG_DEBUG(USB_TAG, "USB HID not available for report %d", reportId);
    return false;
    #endif
}

void USBTransport::setDeviceInfo(const char* name, const char* manufacturer, 
                               uint16_t vid, uint16_t pid, uint16_t version) {
    this->deviceName = name;
    this->deviceManufacturer = manufacturer;
    this->vid = vid;
    this->pid = pid;
    this->version = version;
    
    SQUID_LOG_DEBUG(USB_TAG, "Device info set - Name: %s, Manufacturer: %s, VID: 0x%04X, PID: 0x%04X", 
                   name, manufacturer, vid, pid);
}

void USBTransport::setBatteryLevel(uint8_t level) {
    this->batteryLevel = level;
    // USB HID doesn't have standard battery reporting afaik so I've made it a debug line so you can check it over USB CDC or something
    SQUID_LOG_DEBUG(USB_TAG, "Battery level set to %d%% (USB HID)", level);
}

void USBTransport::setAppearance(uint16_t appearance) {
    this->appearance = appearance;
    SQUID_LOG_DEBUG(USB_TAG, "Appearance set to 0x%04X", appearance);
}

void USBTransport::setCallbacks(TransportCallbacks* callbacks) {
    this->callbacks = callbacks;
    SQUID_LOG_DEBUG(USB_TAG, "Callbacks set");
}

void USBTransport::setReportMap(const uint8_t* descriptor, size_t length) {
    this->reportMap = descriptor;
    this->reportMapLength = length;
    SQUID_LOG_DEBUG(USB_TAG, "Report map set - length: %zu", length);
}

bool USBTransport::supportsHID() {
    return true;
}
