/**
 * @file Digitizer.cpp
 * @brief Implementation of the touch digitizer
 */

#include "Digitizer.h"

static const char* DIGI_TAG = "SQUIDTABLET";

SQUIDTABLET::SQUIDTABLET() 
    : transport(nullptr), _delay_ms(7), _useAbsolute(false), 
      _autoMode(true), _digitizerConfigured(false), 
      _screenWidth(1920), _screenHeight(1080) {
    memset(&_digitizerReport, 0, sizeof(_digitizerReport));
}

void SQUIDTABLET::begin(Transport* trans, uint32_t delay_ms) {
    transport = trans;
    _delay_ms = delay_ms;
    _useAbsolute = false;
    _autoMode = true;
    _digitizerConfigured = false;
    _screenWidth = 1920;
    _screenHeight = 1080;
    memset(&_digitizerReport, 0, sizeof(_digitizerReport));
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer subsystem initialized with delay: %lu ms", delay_ms);
    SQUID_LOG_INFO(DIGI_TAG, "Digitizer service ready");
}

bool SQUIDTABLET::isConnected() {
    return transport ? transport->isConnected() : false;
}

void SQUIDTABLET::onConnect() {
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer connected");
}

void SQUIDTABLET::onDisconnect() {
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer disconnected");
}

void SQUIDTABLET::_detectModeFromAppearance(uint16_t appearance) {
    // If appearance is set to digitizer or tablet, default pointer is the digitizer
    if (appearance == 0x0404 /* DIGITIZER */ || appearance == 0x0405 /* DIGITAL_PEN */ || appearance == 0x0406 /* TABLET */) {
        _useAbsolute = true;
        SQUID_LOG_INFO(DIGI_TAG, "Auto-detected absolute mode from appearance: 0x%04X", appearance);
    // For all other appearances, default pointer is the mouse
    } else {
        _useAbsolute = false;
        SQUID_LOG_INFO(DIGI_TAG, "Auto-detected relative mode from appearance: 0x%04X", appearance);
    }
}

void SQUIDTABLET::click(uint16_t x, uint16_t y, DigitizerKey b) {
    // This is a digitizer method - always use absolute mode
    if (!_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        SQUID_LOG_INFO(DIGI_TAG, "Auto-switched to absolute mode for coordinate click");
    }
    
    uint8_t digitizerButtons = static_cast<uint8_t>(b);
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer click at X:%u, Y:%u, buttons: 0x%02X", x, y, digitizerButtons);
    
    moveTo(x, y, 127, b); // Press with pressure
    delay(_delay_ms);
    moveTo(x, y, 0, DigitizerKey{0});   // Release
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer click completed");
}

void SQUIDTABLET::useAbsoluteMode(bool state) {
    bool previousMode = _useAbsolute;
    _useAbsolute = state;
    _autoMode = false; // If they start managing the states manually, just assume they don't want it to switch automatically
    _digitizerConfigured = state; // Digitizers actually get configured when you make digitizers the default pointer mode
    
    SQUID_LOG_INFO(DIGI_TAG, "%s mode %s (auto-mode disabled) - previous mode: %s", 
                 state ? "Absolute" : "Relative", 
                 state ? "enabled" : "disabled",
                 previousMode ? "absolute" : "relative");
}

void SQUIDTABLET::useAutoMode(bool state) {
    bool previousAutoMode = _autoMode;
    _autoMode = state;
    SQUID_LOG_INFO(DIGI_TAG, "Auto-mode %s - previous: %s", 
                 state ? "enabled" : "disabled",
                 previousAutoMode ? "enabled" : "disabled");
}

void SQUIDTABLET::setDigitizerRange(uint16_t maxX, uint16_t maxY) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Setting digitizer range - previous: X:%u, Y:%u", _screenWidth, _screenHeight);
    
    _screenWidth = maxX;
    _screenHeight = maxY;
    _digitizerConfigured = true; // Mark that digitizer was explicitly configured
    
    // If auto-mode is enabled and digitizer is configured, switch to absolute mode
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        SQUID_LOG_INFO(DIGI_TAG, "Auto-switched to absolute mode due to digitizer configuration");
    }
    
    SQUID_LOG_INFO(DIGI_TAG, "Digitizer range set to X:%u, Y:%u", _screenWidth, _screenHeight);
}

void SQUIDTABLET::moveTo(uint16_t x, uint16_t y, uint8_t pressure, DigitizerKey buttons) {
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        SQUID_LOG_DEBUG(DIGI_TAG, "Auto-switched to absolute mode for coordinate movement");
    }
    
    if (isConnected() && transport && _useAbsolute) {
        // Scale to HID descriptor's 0-32767 range
        uint16_t scaledX = (x * 32767ULL) / _screenWidth;
        uint16_t scaledY = (y * 32767ULL) / _screenHeight;
        
        uint8_t buttonValue = static_cast<uint8_t>(buttons);
        _digitizerReport.buttons = buttonValue & 0x07;  // Mask to 3 bits
        _digitizerReport.x = scaledX;
        _digitizerReport.y = scaledY;
        _digitizerReport.pressure = (pressure > 127) ? 127 : pressure;
        
        // Set flags: ALWAYS report In Range when active
        _digitizerReport.flags = DIGITIZER_FLAG_IN_RANGE;
        // Tip Switch = ON when pressure > 0 (touching), OFF when hovering
        if (pressure > 0) {
            _digitizerReport.flags |= DIGITIZER_FLAG_TIP_SWITCH;
        }
        
        SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer move - X:%u->%u, Y:%u->%u, Pressure:%u, Buttons:0x%02X, Flags:0x%02X",
                     x, scaledX, y, scaledY, pressure, buttonValue, _digitizerReport.flags);
        
        sendDigitizerReport();
    } else {
        SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer movement ignored - %s%s%s", 
                     !isConnected() ? "not connected" : "",
                     !transport ? "no input characteristic" : "",
                     !_useAbsolute ? "not in absolute mode" : "");
    }
}

void SQUIDTABLET::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Beginning stroke at X:%u, Y:%u, initial pressure:%u", x, y, initialPressure);
    moveTo(x, y, initialPressure, DigitizerKey{0});
}

void SQUIDTABLET::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Updating stroke at X:%u, Y:%u, pressure:%u", x, y, pressure);
    moveTo(x, y, pressure, DigitizerKey{0});
}

void SQUIDTABLET::endStroke(uint16_t x, uint16_t y) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Ending stroke at X:%u, Y:%u", x, y);
    moveTo(x, y, 0, DigitizerKey{0});
}

bool SQUIDTABLET::isAbsoluteMode() {
    bool result = false;
    
    // If the user explicitly says they don't want it to switch by itself, then respect their decision
    if (!_autoMode) {
        result = _useAbsolute;
    } else if (_digitizerConfigured) {
        // If the digitizer was explicitly configured, use absolute mode
        result = true;
    } else {
        // Default to relative pointers because having digitizers be the default is definitely gonna confuse people
        result = false;
    }
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Absolute mode check - result: %s, auto-mode: %s, configured: %s", 
                 result ? "true" : "false",
                 _autoMode ? "true" : "false",
                 _digitizerConfigured ? "true" : "false");
    
    return result;
}

bool SQUIDTABLET::isAutoModeEnabled() {
    SQUID_LOG_DEBUG(DIGI_TAG, "Auto-mode check: %s", _autoMode ? "enabled" : "disabled");
    return _autoMode;
}

void SQUIDTABLET::setAppearance(uint16_t appearance) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Setting appearance: 0x%04X, current auto-mode: %s", 
                 appearance, _autoMode ? "enabled" : "disabled");
    
    if (_autoMode) {
        _detectModeFromAppearance(appearance);
    }
    
    SQUID_LOG_INFO(DIGI_TAG, "Appearance set to 0x%04X, mode: %s", 
                 appearance, _useAbsolute ? "absolute" : "relative");
}

void SQUIDTABLET::sendDigitizerReport() {
    if (!isConnected() || !transport) {
        SQUID_LOG_DEBUG(DIGI_TAG, "Cannot send digitizer report - not connected or no transport");
        return;
    }
    
    bool result = transport->sendReport(DIGITIZER_ID, (uint8_t*)&_digitizerReport, sizeof(_digitizerReport));
    if (!result) {
        SQUID_LOG_ERROR(DIGI_TAG, "Failed to send digitizer report via transport");
    } else {
        SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer report sent successfully");
    }
    
    delay(_delay_ms);
}
