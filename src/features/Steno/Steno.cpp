/**
 * @file Steno.cpp
 * @brief Implementation of the PloverHID stenotype featureset
 */

#include "Steno.h"

static const char* STENO_TAG = "SQUIDSTENO";

SQUIDSTENO::SQUIDSTENO() 
    : transport(nullptr) {
    memset(&_stenoReport, 0, sizeof(_stenoReport));
    _stenoReport.reportId = STENO_ID;
}

SQUIDSTENO::~SQUIDSTENO() {
    SQUID_LOG_DEBUG(STENO_TAG, "Stenotype instance destroyed");
}

void SQUIDSTENO::begin(Transport* trans, uint32_t delay_ms) {
    transport = trans;
    _delay_ms = delay_ms;
    memset(&_stenoReport, 0, sizeof(_stenoReport));
    _stenoReport.reportId = STENO_ID;
    
    SQUID_LOG_INFO(STENO_TAG, "Plover HID stenotype initialized");
}

void SQUIDSTENO::updateStenoKey(StenoKey stenoKey, bool pressed) {
    uint8_t keyValue = static_cast<uint8_t>(stenoKey);
    
    if (keyValue < 32) {  // Only 32 bits in our report
        uint8_t byteIndex = keyValue / 8;
        uint8_t bitMask = 1 << (keyValue % 8);
        
        switch (byteIndex) {
            case 0:
                if (pressed) {
                    _stenoReport.keys0 |= bitMask;
                } else {
                    _stenoReport.keys0 &= ~bitMask;
                }
                break;
            case 1:
                if (pressed) {
                    _stenoReport.keys1 |= bitMask;
                } else {
                    _stenoReport.keys1 &= ~bitMask;
                }
                break;
            case 2:
                if (pressed) {
                    _stenoReport.keys2 |= bitMask;
                } else {
                    _stenoReport.keys2 &= ~bitMask;
                }
                break;
            case 3:
                if (pressed) {
                    _stenoReport.keys3 |= bitMask;
                } else {
                    _stenoReport.keys3 &= ~bitMask;
                }
                break;
        }
        
        SQUID_LOG_DEBUG(STENO_TAG, "Key %s - Key: %d, Byte%d: 0x%02X", 
                       pressed ? "press" : "release", keyValue, byteIndex,
                       byteIndex == 0 ? _stenoReport.keys0 :
                       byteIndex == 1 ? _stenoReport.keys1 :
                       byteIndex == 2 ? _stenoReport.keys2 : _stenoReport.keys3);
    }
}

size_t SQUIDSTENO::press(StenoKey stenoKey) {
    SQUID_LOG_DEBUG(STENO_TAG, "Stenotype key press: %d", static_cast<uint8_t>(stenoKey));
    updateStenoKey(stenoKey, true);
    sendStenoReport();
    return 1;
}

size_t SQUIDSTENO::release(StenoKey stenoKey) {
    SQUID_LOG_DEBUG(STENO_TAG, "Stenotype key release: %d", static_cast<uint8_t>(stenoKey));
    updateStenoKey(stenoKey, false);
    sendStenoReport();
    return 1;
}

void SQUIDSTENO::releaseAll() {
    SQUID_LOG_DEBUG(STENO_TAG, "Releasing all stenotype keys");
    
    _stenoReport.keys0 = 0;
    _stenoReport.keys1 = 0;
    _stenoReport.keys2 = 0;
    _stenoReport.keys3 = 0;
    sendStenoReport();
    
    SQUID_LOG_DEBUG(STENO_TAG, "All stenotype keys released");
}

void SQUIDSTENO::stenoStroke(const StenoKey* keys, size_t count) {
    SQUID_LOG_DEBUG(STENO_TAG, "Executing steno stroke with %zu keys", count);
    
    releaseAll();
    for (size_t i = 0; i < count; i++) {
        press(keys[i]);
    }
    sendStenoReport();
    
    SQUID_LOG_DEBUG(STENO_TAG, "Steno stroke completed");
}

void SQUIDSTENO::sendStenoReport() {
    if (!transport || !transport->isConnected()) {
        SQUID_LOG_DEBUG(STENO_TAG, "Cannot send steno report - not connected");
        return;
    }
    
    SQUID_LOG_DEBUG(STENO_TAG, "Sending Plover HID report: %02X %02X %02X %02X",
                 _stenoReport.keys0, _stenoReport.keys1, 
                 _stenoReport.keys2, _stenoReport.keys3);
    
    bool result = transport->sendReport(STENO_ID, 
                                      (uint8_t*)&_stenoReport, 
                                      sizeof(StenoReport));
    
    if (result) {
        SQUID_LOG_DEBUG(STENO_TAG, "Plover HID report sent successfully");
    } else {
        SQUID_LOG_ERROR(STENO_TAG, "Failed to send Plover HID report");
    }
    
    delay(_delay_ms);
}

void SQUIDSTENO::onConnect() {
    SQUID_LOG_DEBUG(STENO_TAG, "Steno HID connected");
}

void SQUIDSTENO::onDisconnect() {
    SQUID_LOG_DEBUG(STENO_TAG, "Steno HID disconnected");
}
