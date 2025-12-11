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
    #if __has_include(<USBHID.h>)
    , hidDevice(nullptr)
    #endif
    , reportMap(nullptr)
    , reportMapLength(0)
    , hidDeviceInitialized(false)
{
}

USBTransport::~USBTransport() {
    end();
}

bool USBTransport::begin() {
    if (initialized) {
        return true;
    }
    
    SQUID_LOG_INFO(USB_TAG, "Initializing USB HID transport");
    
    #ifdef USB_HID_AVAILABLE
    // Initialize USB
    USB.begin();
    
    // Wait for USB to stabilize
    delay(100);
    
    // Create HID device with the report descriptor
    createHIDService();
    
    // Start USB HID
    if (hidDevice && hidDevice->begin()) {
        initialized = true;
        connected = USB.connected();
        
        if (connected) {
            SQUID_LOG_INFO(USB_TAG, "USB HID initialized and connected");
            if (transportCallbacks) {
                transportCallbacks->onConnect();
            }
        } else {
            SQUID_LOG_INFO(USB_TAG, "USB HID initialized - waiting for host connection");
        }
        
        return true;
    }
    #endif
    
    SQUID_LOG_ERROR(USB_TAG, "Failed to initialize USB HID");
    return false;
}

void USBTransport::end() {
    SQUID_LOG_INFO(USB_TAG, "Shutting down USB HID transport");
    
    #if __has_include(<USBHID.h>)
    // Clean up HID device
      if (hidDevice) {
        delete hidDevice;
        hidDevice = nullptr;
      }
    
      USB.end();
    #endif
    bool wasConnected = connected;
    connected = false;
    
    if (wasConnected && callbacks) {
        callbacks->onDisconnect();
    }
    
    SQUID_LOG_INFO(USB_TAG, "USB HID transport shut down");
}

void USBTransport::update() {
    #ifdef USB_HID_AVAILABLE
    static uint32_t lastCheck = 0;
    uint32_t now = millis();
    
    // Check connection state periodically
    if (now - lastCheck >= 100) { // Check every 100ms
        lastCheck = now;
        
        bool currentState = USB.connected();
        if (currentState != connected) {
            connected = currentState;
            
            if (connected && transportCallbacks) {
                SQUID_LOG_INFO(USB_TAG, "USB connected");
                transportCallbacks->onConnect();
            } else if (!connected && transportCallbacks) {
                SQUID_LOG_INFO(USB_TAG, "USB disconnected");
                transportCallbacks->onDisconnect();
            }
        }
    }
    #endif
}

void USBTransport::createHIDService() {
    #ifdef USB_HID_AVAILABLE
    if (hidDevice) {
        delete hidDevice;
    }
    
    // Create USB HID device with custom report descriptor
    if (reportMap && reportMapLength > 0) {
        hidDevice = new USBHID();
        
        // Set the report map (descriptor)
        hidDevice->setReportMap(const_cast<uint8_t*>(reportMap), reportMapLength);
        
        SQUID_LOG_INFO(USB_TAG, "USB HID created with custom report descriptor - Length: %zu", 
                       reportMapLength);
    } else {
        // Fall back to default keyboard descriptor
        hidDevice = new USBHID();
        SQUID_LOG_WARN(USB_TAG, "No report map available, using default USB HID descriptor");
    }
    
    // Setup HID endpoints
    if (!setupHIDEndpoints()) {
        SQUID_LOG_ERROR(USB_TAG, "Failed to setup USB HID endpoints");
    }
    #endif
}

bool USBTransport::setupHIDEndpoints() {
    // For USB, endpoints are configured automatically by the HID stack
    // We just need to ensure the report IDs match what the feature modules expect
    #ifdef USB_HID_AVAILABLE
    if (!hidDevice) {
        return false;
    }
    
    SQUID_LOG_DEBUG(USB_TAG, "USB HID endpoints configured");
    
    // Note: USB HID doesn't need explicit endpoint setup like BLE characteristics
    // The report descriptor defines all the reports, and USB HID stack handles the rest
    
    return true;
    #else
    return false;
    #endif
}

bool USBTransport::isConnected() {
    #if __has_include(<USBHID.h>)
    return USB.connected();
    #else
    return false;
    #endif
}

bool USBTransport::connect() {
    // USB connection is established during begin()
    if (!connected) {
        SQUID_LOG_WARN(USB_TAG, "USB not initialized - call begin() first");
        return false;
    }
    return true;
}

void USBTransport::disconnect() {
    SQUID_LOG_DEBUG(USB_TAG, "USB disconnect requested - calling end()");
    end();
}

bool USBTransport::sendData(const uint8_t* data, size_t length) {
    return sendReport(0, data, length);
}

bool USBTransport::sendReport(uint8_t reportId, const uint8_t* data, size_t length) {
#if __has_include(<USBHID.h>)
    if (!connected || !hidDevice || !hidDeviceInitialized) {
        SQUID_LOG_DEBUG(USB_TAG, "Cannot send report - USB not connected or HID not initialized");
        return false;
    }
    
    // USB HID requires the report ID to be prepended to the data
    uint8_t* usbBuffer = new uint8_t[length + 1];
    usbBuffer[0] = reportId;
    memcpy(usbBuffer + 1, data, length);
    
    // Map report IDs to appropriate USB HID report types
    bool result = false;
    
    switch (reportId) {
        case 0x01:     
//            result = hidDevice->keyboardReport(reportId, usbBuffer, length + 1);
            break;
        #if KEYBOARD_ENABLE
        case 0x02:
            result = hidDevice->NKROReport(reportId, usbBuffer, length + 1);
            break;
        #endif
        #if MEDIA_ENABLE
        case 0x03:     
            result = hidDevice->MediaReport(reportId, usbBuffer, length + 1);
            break;
        #endif
        #if SPACEMOUSE_ENABLE
        case 0x04:
            result = hidDevice->SpaceTranslationReport(reportId, usbBuffer, length + 1);
            break;
        case 0x05:
            result = hidDevice->SpaceRotationReport(reportId, usbBuffer, length + 1);
            break;
        case 0x06:
            result = hidDevice->SpaceButtonReport(reportId, usbBuffer, length + 1);
            break;
        #else
        #if MOUSE_ENABLE
        case 0x07:     
            result = hidDevice->MouseReport(reportId, usbBuffer, length + 1);
            break;
        #endif
        #if DIGITIZER_ENABLE
        case 0x08:
            result = hidDevice->DigitizerReport(reportId, usbBuffer, length + 1);
            break;
        #endif
        #if GAMEPAD_ENABLE
        case 0x09:     
            result = hidDevice->GamepadReport(reportId, usbBuffer, length + 1);
            break;
        #endif
        #endif
        #if STENO_ENABLE
        case 0x50:
            result = hidDevice->StenoReport(reportId, usbBuffer, length + 1);
            break;
        #endif
        default:
            // Generic HID report for other types
            result = hidDevice->sendReport(reportId, usbBuffer, length + 1);
            break;
    }
    
    delete[] usbBuffer;
    
    if (result) {
        SQUID_LOG_DEBUG(USB_TAG, "USB HID report sent - ID: 0x%02X, Length: %zu", reportId, length);
    } else {
        SQUID_LOG_ERROR(USB_TAG, "Failed to send USB HID report - ID: 0x%02X", reportId);
    }
    return result;
#else
    SQUID_LOG_DEBUG(USB_TAG, "USB HID not available for report 0x%02X", reportId);
    return false;
#endif
}

void USBTransport::setDeviceInfo(const char* name, const char* manufacturer, uint16_t vid, uint16_t pid, uint16_t version) {
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
    hidDeviceInitialized = false;
    
    SQUID_LOG_DEBUG(USB_TAG, "Report map set - length: %zu", length);
    
    // For USB, we might need to process the descriptor differently
    #if __has_include(<USBHID.h>)
    if (USB && isConnected()) {
        SQUID_LOG_INFO(USB_TAG, "Reinitializing USB HID with new report descriptor");
        reinitializeHID();
    }
    #endif
}

bool USBTransport::supportsHID() {
    return true;
}

bool USBTransport::reinitializeHID() {
    #if __has_include(<USBHID.h>)
    // Clean up existing HID device if any
    if (hidDevice) {
        delete hidDevice;
        hidDevice = nullptr;
    }
    
    // Create new HID device with the current report descriptor
    #if defined(ARDUINO_ARCH_ESP32)
        // ESP32-S2/S3 allows dynamic HID descriptors
        hidDevice = new USBHID();
        if (hidDevice) {
            // Set the report map
            if (reportMap && reportMapLength > 0) {
                hidDevice->setReportMap(const_cast<uint8_t*>(reportMap), reportMapLength);
                SQUID_LOG_INFO(USB_TAG, "USB HID initialized with dynamic report descriptor, size: %zu", reportMapLength);
            } else {
                SQUID_LOG_ERROR(USB_TAG, "No report map available for USB HID initialization");
                return false;
            }
            
            if (hidDevice->begin()) {
                hidDeviceInitialized = true;
                SQUID_LOG_INFO(USB_TAG, "USB HID initialized successfully");
                return true;
            }
        }
    #else
        // For other platforms, we may need a different approach
        SQUID_LOG_WARN(USB_TAG, "Dynamic HID descriptors may not be supported on this platform");
        // Fall back to a basic keyboard descriptor
        hidDevice = new USBHID();
        if (hidDevice && hidDevice->begin()) {
            hidDeviceInitialized = true;
            SQUID_LOG_INFO(USB_TAG, "USB HID initialized with default descriptor");
            return true;
        }
    #endif
    
    SQUID_LOG_ERROR(USB_TAG, "Failed to initialize USB HID device");
    if (hidDevice) {
        delete hidDevice;
        hidDevice = nullptr;
    }
    return false;
    #else
    return false;
    #endif
}
