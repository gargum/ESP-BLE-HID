/**
 * @file USBTransport.cpp
 * @brief USB HID transport implementation
 */

#include "USBTransport.h"

USBTransport::USBTransport()
    : callbacks(nullptr)
    , deviceName("SquidHID")
    , deviceManufacturer("SquidHID")
    , vid(0x046D)
    , pid(0xC52B)
    , version(0x0310)
    , batteryLevel(100)
    , appearance(KEYBOARD)
    , reportMap(nullptr)
    , reportMapLength(0)
    , initialized(false)
    , connected(false)
{
    static bool deviceAdded = false;
    if (!deviceAdded && reportMap && reportMapLength > 0) {
        deviceAdded = true;
        #if __has_include("USBHID.h")
        hid.addDevice(this, reportMapLength);
        #endif
    }
}

USBTransport::~USBTransport() {
    end();
}

bool USBTransport::begin() {
    if (initialized) {
        return true;
    }
    
    if (!reportMap || reportMapLength == 0) {
        SQUID_LOG_ERROR(USB_TAG, "No report map configured");
        return false;
    }
    
    SQUID_LOG_INFO(USB_TAG, "Initializing USB Transport");
    
    #if __has_include("USBHID.h")
    // Configure USB settings
    USB.VID(vid);
    USB.PID(pid);
    USB.firmwareVersion(version);
    USB.productName(deviceName.c_str());
    USB.manufacturerName(deviceManufacturer.c_str());
    USB.serialNumber("SQUID001");
    
    // USB configuration matching the working reference
    USB.usbAttributes(TUSB_DESC_CONFIG_ATT_SELF_POWERED);
    USB.usbPower(500);
    USB.usbClass(TUSB_CLASS_MISC);
    USB.usbSubClass(MISC_SUBCLASS_COMMON);
    USB.usbProtocol(MISC_PROTOCOL_IAD);
    USB.usbVersion(0x0200);
    
    // Add device to HID if not already added
    hid.addDevice(this, reportMapLength);
    
    // Start HID
    hid.begin();
    
    // Start USB (this should hopefully include our HID interface please work)
    if (!USB.begin()) {
        SQUID_LOG_ERROR(USB_TAG, "Failed to start USB");
        return false;
    }
    #endif
    SQUID_LOG_INFO(USB_TAG, "USBHID initialized with report descriptor: %zu bytes", reportMapLength);
    
    initialized = true;
    
    SQUID_LOG_INFO(USB_TAG, "USB Transport initialized");
    
    return true;
}

void USBTransport::end() {
    if (initialized) {
        SQUID_LOG_INFO(USB_TAG, "Ending USB Transport");
        
        if (callbacks && connected) {
            callbacks->onDisconnect();
        }
        
        connected = false;
        initialized = false;
        
        SQUID_LOG_INFO(USB_TAG, "USB Transport ended");
    }
}

void USBTransport::update() {
    if (!initialized) return;
    
    static uint32_t lastCheck = 0;
    uint32_t now = millis();
    
    if (now - lastCheck >= 500) {
        lastCheck = now;
        
    #if __has_include("USBHID.h")
        bool currentlyConnected = (bool)USB && hid.ready();
    #else
        bool currentlyConnected = false;
    #endif
        if (currentlyConnected != connected) {
            connected = currentlyConnected;
            
            if (connected) {
                SQUID_LOG_INFO(USB_TAG, "USB connected and HID ready");
                if (callbacks) callbacks->onConnect();
            } else {
                SQUID_LOG_INFO(USB_TAG, "USB disconnected or HID not ready");
                if (callbacks) callbacks->onDisconnect();
            }
        }
    }
}

bool USBTransport::isConnected() {
  #if __has_include("USBHID.h")
    return initialized && (bool)USB && hid.ready();
  #else
    return false;
  #endif
}

bool USBTransport::connect() {
    return isConnected();
}

void USBTransport::disconnect() {
    SQUID_LOG_WARN(USB_TAG, "Manual USB disconnect not supported");
}

bool USBTransport::sendReport(uint8_t reportId, const uint8_t* data, size_t length) {
    if (!isConnected()) {
        SQUID_LOG_DEBUG(USB_TAG, "Cannot send report - USB not connected or HID not ready");
        return false;
    }
    
    SQUID_LOG_DEBUG(USB_TAG, "Sending USB HID report ID: 0x%02X, length: %zu", reportId, length);
    
    #if __has_include("USBHID.h")
    bool result = hid.SendReport(reportId, data, (uint8_t)length);
    
    if (result) {
        SQUID_LOG_DEBUG(USB_TAG, "USB HID report %d sent: %zu bytes", reportId, length);
    } else {
        SQUID_LOG_ERROR(USB_TAG, "Failed to send USB HID report %d", reportId);
    }
    
    return result;
    #else
    return false;
    #endif
}

bool USBTransport::sendData(const uint8_t* data, size_t length) {
    return sendReport(0, data, length);
}

#if __has_include("USBHIDVendor.h")

// USBHIDDevice interface implementation
uint16_t USBTransport::_onGetDescriptor(uint8_t* buffer) {
    if (buffer && reportMap && reportMapLength > 0) {
        memcpy(buffer, reportMap, reportMapLength);
        return reportMapLength;
    }
    return 0;
}

void USBTransport::_onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) {
    // Handle HID output reports (like LED status, haptics, other things I haven't properly implemented)
    if (callbacks) {
        callbacks->onDataReceived(buffer, len);
    }
}

#endif

void USBTransport::setDeviceInfo(const char* name, const char* manufacturer, 
                                uint16_t vid, uint16_t pid, uint16_t version) {
    this->deviceName = name ? name : "";
    this->deviceManufacturer = manufacturer ? manufacturer : "";
    this->vid = vid;
    this->pid = pid;
    this->version = version;
    
    SQUID_LOG_INFO(USB_TAG, "Device info set: %s by %s (VID: 0x%04X, PID: 0x%04X)", 
                  deviceName.c_str(), deviceManufacturer.c_str(), vid, pid);
}

void USBTransport::setBatteryLevel(uint8_t level) {
    batteryLevel = level;
    SQUID_LOG_DEBUG(USB_TAG, "Battery level: %d%%", batteryLevel);
}

void USBTransport::setAppearance(uint16_t appearance) {
    this->appearance = appearance;
    SQUID_LOG_DEBUG(USB_TAG, "Appearance: 0x%04X", appearance);
}

void USBTransport::setCallbacks(TransportCallbacks* callbacks) {
    this->callbacks = callbacks;
}

void USBTransport::setReportMap(const uint8_t* descriptor, size_t length) {
    reportMap = descriptor;
    reportMapLength = length;
    SQUID_LOG_INFO(USB_TAG, "Report map set: %zu bytes", length);
}
