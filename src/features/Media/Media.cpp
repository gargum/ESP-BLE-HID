/**
 * @file Media.cpp
 * @brief Implementation of the media keys
 */

#include "Media.h"
#include "../../drivers/Log/Log.h"

static const char* MEDIA_TAG = "SQUIDMEDIA";

SQUIDMEDIA::SQUIDMEDIA() 
    : inputMediaKeys(nullptr), _currentMediaKey(MediaKey{0}), _delay_ms(7) { }

void SQUIDMEDIA::begin(SquidCharacteristic* mediaChar, uint32_t delay_ms) {
    inputMediaKeys = mediaChar;
    _delay_ms = delay_ms;
    _currentMediaKey = MediaKey{0};
    
    SQUID_LOG_DEBUG(MEDIA_TAG, "Media subsystem initialized with delay: %lu ms", delay_ms);
    SQUID_LOG_INFO(MEDIA_TAG, "Media keys service ready (single-key mode)");
}

bool SQUIDMEDIA::isConnected() {
    // We need to check if the characteristic and underlying BLE stack are connected
    // Since we don't have direct access to SquidDevice, we'll check if the characteristic exists
    // and assume connection state is managed by the parent SQUIDHID class
    bool connected = (inputMediaKeys != nullptr);
    SQUID_LOG_DEBUG(MEDIA_TAG, "Connection check: %s", connected ? "characteristic available" : "no characteristic");
    return connected;
}

size_t SQUIDMEDIA::press(MediaKey mediaKey) {
    _currentMediaKey = mediaKey;
    SQUID_LOG_DEBUG(MEDIA_TAG, "Media key pressed: 0x%04X", static_cast<uint16_t>(mediaKey));
    sendMediaReport();
    return 1;
}

size_t SQUIDMEDIA::release(MediaKey mediaKey) {
    if (_currentMediaKey == mediaKey) {
        _currentMediaKey = MediaKey{0};
        SQUID_LOG_DEBUG(MEDIA_TAG, "Media key released: 0x%04X", static_cast<uint16_t>(mediaKey));
        sendMediaReport();
        return 1;
    }
    return 0;
}

size_t SQUIDMEDIA::write(MediaKey mediaKey) {
    SQUID_LOG_DEBUG(MEDIA_TAG, "Writing media key: 0x%04X", static_cast<uint16_t>(mediaKey));
    size_t p = press(mediaKey);
    if (p > 0) {
        release(mediaKey);
    }
    return p;
}


MediaKey SQUIDMEDIA::getCurrentMediaKey() {
    SQUID_LOG_DEBUG(MEDIA_TAG, "Current media key: 0x%04X", static_cast<uint16_t>(_currentMediaKey));
    return _currentMediaKey;
}

void SQUIDMEDIA::sendMediaReport() {
    if (isConnected() && inputMediaKeys) {
        // Convert to underlying type for BLE transmission
        uint16_t rawKey = static_cast<uint16_t>(_currentMediaKey);
        inputMediaKeys->setValue((uint8_t*)&rawKey, sizeof(uint16_t));
        
        if (inputMediaKeys->notify()) {
            SQUID_LOG_DEBUG(MEDIA_TAG, "Media report sent successfully - key: 0x%04X", rawKey);
        } else {
            SQUID_LOG_WARN(MEDIA_TAG, "Failed to send media report notification");
        }
        
        delay(_delay_ms);
    } else {
        SQUID_LOG_DEBUG(MEDIA_TAG, "Cannot send media report - %s", 
                     !isConnected() ? "not connected" : "no input characteristic");
    }
}

void SQUIDMEDIA::releaseAll() {
    SQUID_LOG_DEBUG(MEDIA_TAG, "Releasing all media keys - previous key: 0x%04X", 
                 static_cast<uint16_t>(_currentMediaKey));
    _currentMediaKey = MediaKey{0};
    sendMediaReport();
    SQUID_LOG_DEBUG(MEDIA_TAG, "All media keys released");
}
