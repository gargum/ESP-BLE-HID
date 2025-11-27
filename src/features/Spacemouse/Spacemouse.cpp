/**
 * @file Spacemouse.cpp
 * @brief Implementation of the 3DConnexion Spacemouse feature
 */

#include "Spacemouse.h"

static const char* SPACEMOUSE_TAG = "SQUID6DOF";

SQUIDSPACEMOUSE::SQUIDSPACEMOUSE() 
    : transport(nullptr), _delay_ms(7) {
    memset(&_transReport, 0, sizeof(_transReport));
    memset(&_rotReport, 0, sizeof(_rotReport));
    memset(&_buttonReport, 0, sizeof(_buttonReport));
}

void SQUIDSPACEMOUSE::begin(Transport* trans, uint32_t delay_ms) {
    transport = trans;
    _delay_ms = delay_ms;
    memset(&_transReport, 0, sizeof(_transReport));
    memset(&_rotReport, 0, sizeof(_rotReport));
    memset(&_buttonReport, 0, sizeof(_buttonReport));
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse subsystem initialized with delay: %lu ms", delay_ms);
    SQUID_LOG_INFO(SPACEMOUSE_TAG, "Space Mouse service ready");
}

bool SQUIDSPACEMOUSE::isConnected() {
    return transport ? transport->isConnected() : false;
}

void SQUIDSPACEMOUSE::onConnect() {
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse connected");
}

void SQUIDSPACEMOUSE::onDisconnect() {
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse disconnected");
}

void SQUIDSPACEMOUSE::move(int16_t tx, int16_t ty, int16_t tz, int16_t rx, int16_t ry, int16_t rz) {
    if (isConnected() && transport) {
        // Set translation values
        _transReport.tx = tx;
        _transReport.ty = ty;
        _transReport.tz = tz;
        
        // Set rotation values
        _rotReport.rx = rx;
        _rotReport.ry = ry;
        _rotReport.rz = rz;
        
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, 
            "Space Mouse movement - T:(%d, %d, %d) R:(%d, %d, %d)", tx, ty, tz, rx, ry, rz);
        
        sendReport();
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no transport");
    }
}

void SQUIDSPACEMOUSE::translate(int16_t tx, int16_t ty, int16_t tz) {
    if (isConnected() && transport) {
        // Set translation values
        _transReport.tx = tx;
        _transReport.ty = ty;
        _transReport.tz = tz;
        
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse movement - T:(%d, %d, %d)", tx, ty, tz);
        
        sendReport();
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no transport");
    }
}

void SQUIDSPACEMOUSE::rotate(int16_t rx, int16_t ry, int16_t rz) {
    if (isConnected() && transport) {
        // Set rotation values
        _rotReport.rx = rx;
        _rotReport.ry = ry;
        _rotReport.rz = rz;
        
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse movement - R:(%d, %d, %d)", rx, ry, rz);
        
        sendReport();
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no transport");
    }
}

void SQUIDSPACEMOUSE::press(uint8_t button) {
    if (button < 1 || button > 32) {
        SQUID_LOG_WARN(SPACEMOUSE_TAG, "Invalid button number: %d (must be 1-32)", button);
        return;
    }
    
    uint32_t buttonMask = (1UL << (button - 1));
    _buttonReport.buttons |= buttonMask;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse button %d pressed - button state: 0x%08lX", 
                 button, _buttonReport.buttons);
    sendReport();
}

void SQUIDSPACEMOUSE::release(uint8_t button) {
    if (button < 1 || button > 32) {
        SQUID_LOG_WARN(SPACEMOUSE_TAG, "Invalid button number: %d (must be 1-32)", button);
        return;
    }
    
    uint32_t buttonMask = (1UL << (button - 1));
    _buttonReport.buttons &= ~buttonMask;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse button %d released - button state: 0x%08lX", 
                 button, _buttonReport.buttons);
    sendReport();
}

bool SQUIDSPACEMOUSE::isPressed(uint8_t button) {
    if (button < 1 || button > 32) {
        return false;
    }
    
    uint32_t buttonMask = (1UL << (button - 1));
    bool pressed = (_buttonReport.buttons & buttonMask) != 0;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse button %d check - Pressed: %s", 
                 button, pressed ? "true" : "false");
    return pressed;
}

void SQUIDSPACEMOUSE::setAllButtons(uint32_t buttons) {
    uint32_t previousButtons = _buttonReport.buttons;
    _buttonReport.buttons = buttons;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse buttons set - previous: 0x%08lX, new: 0x%08lX", 
                 previousButtons, buttons);
    sendReport();
}

void SQUIDSPACEMOUSE::releaseAll() {
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Releasing all Space Mouse buttons - previous state: 0x%08lX", 
                 _buttonReport.buttons);
    _buttonReport.buttons = 0;
    sendReport();
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "All Space Mouse buttons released");
}

void SQUIDSPACEMOUSE::sendReport() {
    if (!isConnected() || !transport) {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Cannot send Space Mouse report - not connected or no transport");
        return;
    }
    
    // Send translation report (Report ID 0x07)
    bool transResult = transport->sendReport(SPACETRANS_ID, (uint8_t*)&_transReport, sizeof(_transReport));
    
    // Send rotation report (Report ID 0x08)  
    bool rotResult = transport->sendReport(SPACEROTAT_ID, (uint8_t*)&_rotReport, sizeof(_rotReport));
    
    // Send button report (Report ID 0x09)
    bool buttonResult = transport->sendReport(SPACECLICK_ID, (uint8_t*)&_buttonReport, sizeof(_buttonReport));
    
    if (!transResult || !rotResult || !buttonResult) {
        SQUID_LOG_ERROR(SPACEMOUSE_TAG, "Failed to send Space Mouse reports - T:%s R:%s B:%s",
                     transResult ? "OK" : "FAIL",
                     rotResult ? "OK" : "FAIL", 
                     buttonResult ? "OK" : "FAIL");
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Space Mouse reports sent successfully");
    }
    
    delay(_delay_ms);
}
