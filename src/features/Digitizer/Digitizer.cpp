#include "Digitizer.h"
#include "NimBLEDevice.h"

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
}

bool BLEDIGI::isConnected() {
    if (!inputDigitizer) return false;
    return NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0;
}

void BLEDIGI::_detectModeFromAppearance(uint16_t appearance) {
    // If appearance is set to digitizer or tablet, default pointer is the digitizer
    if (appearance == 0x0404 /* DIGITIZER */ || appearance == 0x0405 /* DIGITAL_PEN */ || appearance == 0x0406 /* TABLET */) {
        _useAbsolute = true;
        Serial.printf("[%s] Auto-detected absolute mode from appearance: 0x%04X\n", DIGI_TAG, appearance);
    // For all other appearances, default pointer is the mouse
    } else {
        _useAbsolute = false;
        Serial.printf("[%s] Auto-detected relative mode from appearance: 0x%04X\n", DIGI_TAG, appearance);
    }
}

void BLEDIGI::click(uint16_t x, uint16_t y, char b) {
    // This is a digitizer method - always use absolute mode
    if (!_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        Serial.printf("[%s] Auto-switched to absolute mode for coordinate click\n", DIGI_TAG);
    }
    
    // Map mouse button constants to digitizer button constants for simplicity/convenience
    uint8_t digitizerButtons = 0;
    if (b & 1) digitizerButtons |= DIGITIZER_BTN1;  // MOUSE_LEFT
    if (b & 2) digitizerButtons |= DIGITIZER_BTN2;  // MOUSE_RIGHT
    if (b & 4) digitizerButtons |= DIGITIZER_BTN3;  // MOUSE_MIDDLE
    
    moveTo(x, y, 127, digitizerButtons); // Press with pressure
    delay(_delay_ms);
    moveTo(x, y, 0, 0);   // Release
}

void BLEDIGI::useAbsoluteMode(bool state) {
    _useAbsolute = state;
    _autoMode = false; // If they start managing the states manually, just assume they don't want it to switch automatically
    _digitizerConfigured = state; // Digitizers actually get configured when you make digitizers the default pointer mode
    Serial.printf("[%s] %s mode %s (auto-mode disabled)\n", DIGI_TAG, state ? "Absolute" : "Relative", state ? "enabled" : "disabled");
}

void BLEDIGI::useAutoMode(bool state) {
    _autoMode = state;
    Serial.printf("[%s] Auto-mode %s\n", DIGI_TAG, state ? "enabled" : "disabled");
}

void BLEDIGI::setDigitizerRange(uint16_t maxX, uint16_t maxY) {
    _screenWidth = maxX;
    _screenHeight = maxY;
    _digitizerConfigured = true; // Mark that digitizer was explicitly configured
    
    // If auto-mode is enabled and digitizer is configured, switch to absolute mode
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        Serial.printf("[%s] Auto-switched to absolute mode due to digitizer configuration\n", DIGI_TAG);
    }
    
    Serial.printf("[%s] Digitizer range set to X:%u, Y:%u\n", DIGI_TAG, _screenWidth, _screenHeight);
}

void BLEDIGI::moveTo(uint16_t x, uint16_t y, uint8_t pressure, uint8_t buttons) {
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        Serial.printf("[%s] Auto-switched to absolute mode\n", DIGI_TAG);
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
        
        sendDigitizerReport();
    }
}

void BLEDIGI::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) {
    moveTo(x, y, initialPressure);
}

void BLEDIGI::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) {
    moveTo(x, y, pressure);
}

void BLEDIGI::endStroke(uint16_t x, uint16_t y) {
    moveTo(x, y, 0);
}

void BLEDIGI::sendDigitizerReport() {
    if (isConnected() && inputDigitizer && _useAbsolute) {
        inputDigitizer->setValue((uint8_t*)&_digitizerReport, sizeof(_digitizerReport));
        inputDigitizer->notify();
        delay(_delay_ms);
    }
}

bool BLEDIGI::isAbsoluteMode() {
    // If the user explicitly says they don't want it to switch by itself, then respect their decision
    if (!_autoMode) {return _useAbsolute;}
    
    // If the digitizer was explicitly configured, use absolute mode
    if (_digitizerConfigured) { return true;}
    
    // Default to relative pointers because having digitizers be the default is definitely gonna confuse people
    return false;
}

bool BLEDIGI::isAutoModeEnabled() {
    return _autoMode;
}

void BLEDIGI::setAppearance(uint16_t appearance) {
    if (_autoMode) {
        _detectModeFromAppearance(appearance);
    }
}
