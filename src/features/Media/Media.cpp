/**
 * @file Media.cpp
 * @brief Implementation of the media keys
 */

#include "Media.h"
#include "NimBLEDevice.h"
#include "../Log/Log.h"

static const char* MEDIA_TAG = "BLEMEDIA";

BLEMEDIA::BLEMEDIA() 
    : inputMediaKeys(nullptr), _currentMediaKey(MediaKey{0}), _delay_ms(7) { }

void BLEMEDIA::begin(NimBLECharacteristic* mediaChar, uint32_t delay_ms) {
    inputMediaKeys = mediaChar;
    _delay_ms = delay_ms;
    _currentMediaKey = MediaKey{0};
    
    BLE_LOG_DEBUG(MEDIA_TAG, "Media subsystem initialized with delay: %lu ms", delay_ms);
    BLE_LOG_INFO(MEDIA_TAG, "Media keys service ready (single-key mode)");
}

bool BLEMEDIA::isConnected() {
    bool connected = (inputMediaKeys && NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0);
    BLE_LOG_DEBUG(MEDIA_TAG, "Connection check: %s", connected ? "connected" : "disconnected");
    return connected;
}

size_t BLEMEDIA::press(MediaKey mediaKey) {
    _currentMediaKey = mediaKey;
    BLE_LOG_DEBUG(MEDIA_TAG, "Media key pressed: 0x%04X", static_cast<uint16_t>(mediaKey));
    sendMediaReport();
    return 1;
}

size_t BLEMEDIA::release(MediaKey mediaKey) {
    if (_currentMediaKey == mediaKey) {
        _currentMediaKey = MediaKey{0};
        BLE_LOG_DEBUG(MEDIA_TAG, "Media key released: 0x%04X", static_cast<uint16_t>(mediaKey));
        sendMediaReport();
        return 1;
    }
    return 0;
}

size_t BLEMEDIA::write(MediaKey mediaKey) {
    BLE_LOG_DEBUG(MEDIA_TAG, "Writing media key: 0x%04X", static_cast<uint16_t>(mediaKey));
    size_t p = press(mediaKey);
    if (p > 0) {
        release(mediaKey);
    }
    return p;
}


MediaKey BLEMEDIA::getCurrentMediaKey() {
    BLE_LOG_DEBUG(MEDIA_TAG, "Current media key: 0x%04X", static_cast<uint16_t>(_currentMediaKey));
    return _currentMediaKey;
}

void BLEMEDIA::sendMediaReport() {
    if (isConnected() && inputMediaKeys) {
        // Convert to underlying type for BLE transmission
        uint16_t rawKey = static_cast<uint16_t>(_currentMediaKey);
        inputMediaKeys->setValue((uint8_t*)&rawKey, sizeof(uint16_t));
        
        if (inputMediaKeys->notify()) {
            BLE_LOG_DEBUG(MEDIA_TAG, "Media report sent successfully - key: 0x%04X", rawKey);
        } else {
            BLE_LOG_WARN(MEDIA_TAG, "Failed to send media report notification");
        }
        
        delay(_delay_ms);
    } else {
        BLE_LOG_DEBUG(MEDIA_TAG, "Cannot send media report - %s", 
                     !isConnected() ? "not connected" : "no input characteristic");
    }
}

void BLEMEDIA::releaseAll() {
    BLE_LOG_DEBUG(MEDIA_TAG, "Releasing all media keys - previous key: 0x%04X", 
                 static_cast<uint16_t>(_currentMediaKey));
    _currentMediaKey = MediaKey{0};
    sendMediaReport();
    BLE_LOG_DEBUG(MEDIA_TAG, "All media keys released");
}
