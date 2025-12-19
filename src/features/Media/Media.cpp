/**
 * @file Media.cpp
 * @brief Implementation of the media keys
 */

#include "Media.h"

SQUIDMEDIA::SQUIDMEDIA() 
    : transport(nullptr), _currentMediaKey(MediaKey{0}), _delay_ms(7) {
    memset(&_mediaReport, 0, sizeof(_mediaReport));
}

void SQUIDMEDIA::begin(Transport* trans, uint32_t delay_ms) {
    transport = trans;
    _delay_ms = delay_ms;
    _currentMediaKey = MediaKey{0};
    memset(&_mediaReport, 0, sizeof(_mediaReport));
    
    SQUID_LOG_DEBUG(MEDIA_TAG, "Media subsystem initialized with transport");
}

bool SQUIDMEDIA::isConnected() {
    return transport ? transport->isConnected() : false;
}

void SQUIDMEDIA::onConnect() {
    SQUID_LOG_DEBUG(MEDIA_TAG, "Media connected");
}

void SQUIDMEDIA::onDisconnect() {
    SQUID_LOG_DEBUG(MEDIA_TAG, "Media disconnected");
}

size_t SQUIDMEDIA::press(MediaKey mediaKey) {
    _currentMediaKey = mediaKey;
    _mediaReport.usage = static_cast<uint16_t>(mediaKey);  // Update the report
    
    SQUID_LOG_DEBUG(MEDIA_TAG, "Media key pressed: 0x%04X", static_cast<uint16_t>(mediaKey));
    sendMediaReport();
    return 1;
}

size_t SQUIDMEDIA::release(MediaKey mediaKey) {
    if (_currentMediaKey == mediaKey) {
        _currentMediaKey = MediaKey{0};
        _mediaReport.usage = 0;
        
        SQUID_LOG_DEBUG(MEDIA_TAG, "Media key released: 0x%04X", static_cast<uint16_t>(mediaKey));
        sendMediaReport();
        return 1;
    }
    return 0;
}

void SQUIDMEDIA::releaseAll() {
    SQUID_LOG_DEBUG(MEDIA_TAG, "Releasing all media keys - previous key: 0x%04X", 
                 static_cast<uint16_t>(_currentMediaKey));
    _currentMediaKey = MediaKey{0};
    _mediaReport.usage = 0;
    sendMediaReport();
    SQUID_LOG_DEBUG(MEDIA_TAG, "All media keys released");
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
    if (!isConnected() || !transport) {
        SQUID_LOG_DEBUG(MEDIA_TAG, "Cannot send media report - not connected or no transport");
        return;
    }
    
    bool result = transport->sendReport(MEDIA_KEYS_ID, (uint8_t*)&_mediaReport, sizeof(_mediaReport));
    if (!result) {
        SQUID_LOG_ERROR(MEDIA_TAG, "Failed to send media report via transport");
    } else {
        SQUID_LOG_DEBUG(MEDIA_TAG, "Media report sent successfully");
    }
    
    delay(_delay_ms);
}
