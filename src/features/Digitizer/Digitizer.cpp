#include "Digitizer.h"
#include "NimBLEDevice.h"
#include "../Log/Log.h"

static const char* DIGI_TAG = "BLEDIGI";

BLEDIGI::BLEDIGI() 
    : inputDigitizer(nullptr), _delay_ms(7), _useAbsolute(false), 
      _autoMode(true), _digitizerConfigured(false), 
      _screenWidth(1920), _screenHeight(1080) {
    memset(&_digitizerReport, 0, sizeof(_digitizerReport));
}

void BLEDIGI::begin(NimBLECharacteristic* digitizerChar, uint32_t delay_ms) {
    inputDigitizer = digitizerChar;
    _delay_ms = delay_ms;
    _useAbsolute = false;
    _autoMode = true;
    _digitizerConfigured = false;
    _screenWidth = 1920;
    _screenHeight = 1080;
    memset(&_digitizerReport, 0, sizeof(_digitizerReport));
    
    BLE_LOG_DEBUG(DIGI_TAG, "Digitizer subsystem initialized with delay: %lu ms", delay_ms);
    BLE_LOG_INFO(DIGI_TAG, "Digitizer service ready");
}

bool BLEDIGI::isConnected() {
    bool connected = (inputDigitizer && NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0);
    BLE_LOG_DEBUG(DIGI_TAG, "Connection check: %s", connected ? "connected" : "disconnected");
    return connected;
}

void BLEDIGI::_detectModeFromAppearance(uint16_t appearance) {
    // If appearance is set to digitizer or tablet, default pointer is the digitizer
    if (appearance == 0x0404 /* DIGITIZER */ || appearance == 0x0405 /* DIGITAL_PEN */ || appearance == 0x0406 /* TABLET */) {
        _useAbsolute = true;
        BLE_LOG_INFO(DIGI_TAG, "Auto-detected absolute mode from appearance: 0x%04X", appearance);
    // For all other appearances, default pointer is the mouse
    } else {
        _useAbsolute = false;
        BLE_LOG_INFO(DIGI_TAG, "Auto-detected relative mode from appearance: 0x%04X", appearance);
    }
}

void BLEDIGI::click(uint16_t x, uint16_t y, char b) {
    // This is a digitizer method - always use absolute mode
    if (!_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        BLE_LOG_INFO(DIGI_TAG, "Auto-switched to absolute mode for coordinate click");
    }
    
    // Map mouse button constants to digitizer button constants for simplicity/convenience
    uint8_t digitizerButtons = 0;
    if (b & 1) digitizerButtons |= DIGITIZER_BTN1;  // MOUSE_LEFT
    if (b & 2) digitizerButtons |= DIGITIZER_BTN2;  // MOUSE_RIGHT
    if (b & 4) digitizerButtons |= DIGITIZER_BTN3;  // MOUSE_MIDDLE
    
    BLE_LOG_DEBUG(DIGI_TAG, "Digitizer click at X:%u, Y:%u, buttons: 0x%02X", x, y, digitizerButtons);
    
    moveTo(x, y, 127, digitizerButtons); // Press with pressure
    delay(_delay_ms);
    moveTo(x, y, 0, 0);   // Release
    
    BLE_LOG_DEBUG(DIGI_TAG, "Digitizer click completed");
}

void BLEDIGI::useAbsoluteMode(bool state) {
    bool previousMode = _useAbsolute;
    _useAbsolute = state;
    _autoMode = false; // If they start managing the states manually, just assume they don't want it to switch automatically
    _digitizerConfigured = state; // Digitizers actually get configured when you make digitizers the default pointer mode
    
    BLE_LOG_INFO(DIGI_TAG, "%s mode %s (auto-mode disabled) - previous mode: %s", 
                 state ? "Absolute" : "Relative", 
                 state ? "enabled" : "disabled",
                 previousMode ? "absolute" : "relative");
}

void BLEDIGI::useAutoMode(bool state) {
    bool previousAutoMode = _autoMode;
    _autoMode = state;
    BLE_LOG_INFO(DIGI_TAG, "Auto-mode %s - previous: %s", 
                 state ? "enabled" : "disabled",
                 previousAutoMode ? "enabled" : "disabled");
}

void BLEDIGI::setDigitizerRange(uint16_t maxX, uint16_t maxY) {
    BLE_LOG_DEBUG(DIGI_TAG, "Setting digitizer range - previous: X:%u, Y:%u", _screenWidth, _screenHeight);
    
    _screenWidth = maxX;
    _screenHeight = maxY;
    _digitizerConfigured = true; // Mark that digitizer was explicitly configured
    
    // If auto-mode is enabled and digitizer is configured, switch to absolute mode
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        BLE_LOG_INFO(DIGI_TAG, "Auto-switched to absolute mode due to digitizer configuration");
    }
    
    BLE_LOG_INFO(DIGI_TAG, "Digitizer range set to X:%u, Y:%u", _screenWidth, _screenHeight);
}

void BLEDIGI::moveTo(uint16_t x, uint16_t y, uint8_t pressure, uint8_t buttons) {
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        BLE_LOG_DEBUG(DIGI_TAG, "Auto-switched to absolute mode for coordinate movement");
    }
    
    if (isConnected() && inputDigitizer && _useAbsolute) {
        // Scale to HID descriptor's 0-32767 range
        uint16_t scaledX = (x * 32767ULL) / _screenWidth;
        uint16_t scaledY = (y * 32767ULL) / _screenHeight;
        
        _digitizerReport.buttons = buttons & 0x07;  // Mask to 3 bits
        _digitizerReport.x = scaledX;
        _digitizerReport.y = scaledY;
        _digitizerReport.pressure = (pressure > 127) ? 127 : pressure;
        
        // Set flags: ALWAYS report In Range when active
        _digitizerReport.flags = DIGITIZER_FLAG_IN_RANGE;
        // Tip Switch = ON when pressure > 0 (touching), OFF when hovering
        if (pressure > 0) {
            _digitizerReport.flags |= DIGITIZER_FLAG_TIP_SWITCH;
        }
        
        BLE_LOG_DEBUG(DIGI_TAG, "Digitizer move - X:%u->%u, Y:%u->%u, Pressure:%u, Buttons:0x%02X, Flags:0x%02X",
                     x, scaledX, y, scaledY, pressure, buttons, _digitizerReport.flags);
        
        sendDigitizerReport();
    } else {
        BLE_LOG_DEBUG(DIGI_TAG, "Digitizer movement ignored - %s%s%s", 
                     !isConnected() ? "not connected" : "",
                     !inputDigitizer ? "no input characteristic" : "",
                     !_useAbsolute ? "not in absolute mode" : "");
    }
}

void BLEDIGI::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) {
    BLE_LOG_DEBUG(DIGI_TAG, "Beginning stroke at X:%u, Y:%u, initial pressure:%u", x, y, initialPressure);
    moveTo(x, y, initialPressure);
}

void BLEDIGI::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) {
    BLE_LOG_DEBUG(DIGI_TAG, "Updating stroke at X:%u, Y:%u, pressure:%u", x, y, pressure);
    moveTo(x, y, pressure);
}

void BLEDIGI::endStroke(uint16_t x, uint16_t y) {
    BLE_LOG_DEBUG(DIGI_TAG, "Ending stroke at X:%u, Y:%u", x, y);
    moveTo(x, y, 0);
}

void BLEDIGI::sendDigitizerReport() {
    if (isConnected() && inputDigitizer && _useAbsolute) {
        inputDigitizer->setValue((uint8_t*)&_digitizerReport, sizeof(_digitizerReport));
        
        if (inputDigitizer->notify()) {
            BLE_LOG_DEBUG(DIGI_TAG, "Digitizer report sent successfully - "
                         "X: %u, Y: %u, Pressure: %u, Buttons: 0x%02X, Flags: 0x%02X",
                         _digitizerReport.x, _digitizerReport.y, _digitizerReport.pressure,
                         _digitizerReport.buttons, _digitizerReport.flags);
        } else {
            BLE_LOG_WARN(DIGI_TAG, "Failed to send digitizer report notification");
        }
        
        delay(_delay_ms);
    } else {
        BLE_LOG_DEBUG(DIGI_TAG, "Cannot send digitizer report - %s%s%s", 
                     !isConnected() ? "not connected" : "",
                     !inputDigitizer ? "no input characteristic" : "",
                     !_useAbsolute ? "not in absolute mode" : "");
    }
}

bool BLEDIGI::isAbsoluteMode() {
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
    
    BLE_LOG_DEBUG(DIGI_TAG, "Absolute mode check - result: %s, auto-mode: %s, configured: %s", 
                 result ? "true" : "false",
                 _autoMode ? "true" : "false",
                 _digitizerConfigured ? "true" : "false");
    
    return result;
}

bool BLEDIGI::isAutoModeEnabled() {
    BLE_LOG_DEBUG(DIGI_TAG, "Auto-mode check: %s", _autoMode ? "enabled" : "disabled");
    return _autoMode;
}

void BLEDIGI::setAppearance(uint16_t appearance) {
    BLE_LOG_DEBUG(DIGI_TAG, "Setting appearance: 0x%04X, current auto-mode: %s", 
                 appearance, _autoMode ? "enabled" : "disabled");
    
    if (_autoMode) {
        _detectModeFromAppearance(appearance);
    }
    
    BLE_LOG_INFO(DIGI_TAG, "Appearance set to 0x%04X, mode: %s", 
                 appearance, _useAbsolute ? "absolute" : "relative");
}
