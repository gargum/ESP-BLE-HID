/**
 * @file Spacemouse.cpp
 * @brief Implementation of the 3DConnexion Spacemouse feature
 */

#include "Spacemouse.h"

SQUIDSPACEMOUSE::SQUIDSPACEMOUSE() 
    : 
    #if DIGITIZER_ENABLE
    _screenWidth(DEFAULT_WIDTH), _screenHeight(DEFAULT_HEIGHT),
    #endif
    transport(nullptr), _delay_ms(7)
    {
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
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse subsystem initialized with delay: %lu ms", delay_ms);
    SQUID_LOG_INFO(SPACEMOUSE_TAG, "Spacemouse service ready");
}

bool SQUIDSPACEMOUSE::isConnected() {
    return transport ? transport->isConnected() : false;
}

void SQUIDSPACEMOUSE::onConnect() {
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse connected");
}

void SQUIDSPACEMOUSE::onDisconnect() {
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse disconnected");
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
            "Spacemouse movement - T:(%d, %d, %d) R:(%d, %d, %d)", tx, ty, tz, rx, ry, rz);
        
        sendReport();
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no transport");
    }
}

void SQUIDSPACEMOUSE::translate(int16_t tx, int16_t ty, int16_t tz) {
    if (isConnected() && transport) {
        // Set translation values
        _transReport.tx = tx;
        _transReport.ty = ty;
        _transReport.tz = tz;
        
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse movement - T:(%d, %d, %d)", tx, ty, tz);
        
        sendReport();
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no transport");
    }
}

void SQUIDSPACEMOUSE::rotate(int16_t rx, int16_t ry, int16_t rz) {
    if (isConnected() && transport) {
        // Set rotation values
        _rotReport.rx = rx;
        _rotReport.ry = ry;
        _rotReport.rz = rz;
        
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse movement - R:(%d, %d, %d)", rx, ry, rz);
        
        sendReport();
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no transport");
    }
}

void SQUIDSPACEMOUSE::press(SpacemouseKey button) {
    if (button < 1 || button > 64) {
        SQUID_LOG_WARN(SPACEMOUSE_TAG, "Invalid button number: %d (must be 1-64)", button);
        return;
    }
    
    uint32_t buttonMask = (1UL << ((button - 1) % 32));
    uint8_t arrayIndex = (button - 1) / 32;
    _buttonReport.buttons[arrayIndex] |= buttonMask;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse button %d pressed - button state[%d]: 0x%08lX", 
                 button, arrayIndex, _buttonReport.buttons[arrayIndex]);
    sendReport();
}

void SQUIDSPACEMOUSE::release(SpacemouseKey button) {
    if (button < 1 || button > 64) {
        SQUID_LOG_WARN(SPACEMOUSE_TAG, "Invalid button number: %d (must be 1-64)", button);
        return;
    }
    
    uint32_t buttonMask = (1UL << ((button - 1) % 32));
    uint8_t arrayIndex = (button - 1) / 32;
    _buttonReport.buttons[arrayIndex] &= ~buttonMask;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse button %d released - button state[%d]: 0x%08lX", 
                 button, arrayIndex, _buttonReport.buttons[arrayIndex]);
    sendReport();
}

bool SQUIDSPACEMOUSE::isPressed(SpacemouseKey button) {
    if (button < 1 || button > 64) {
        return false;
    }
    
    uint32_t buttonMask = (1UL << ((button - 1) % 32));
    uint8_t arrayIndex = (button - 1) / 32;
    bool pressed = (_buttonReport.buttons[arrayIndex] & buttonMask) != 0;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse button %d check - Pressed: %s", 
                 button, pressed ? "true" : "false");
    return pressed;
}

void SQUIDSPACEMOUSE::setAllButtons(uint32_t lowButtons, uint32_t highButtons) {
    uint32_t previousLow = _buttonReport.buttons[0];
    uint32_t previousHigh = _buttonReport.buttons[1];
    
    _buttonReport.buttons[0] = lowButtons;
    _buttonReport.buttons[1] = highButtons;
    
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse buttons set - previous: [0x%08lX, 0x%08lX], new: [0x%08lX, 0x%08lX]", 
                 previousLow, previousHigh, lowButtons, highButtons);
    sendReport();
}

void SQUIDSPACEMOUSE::releaseAll() {
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Releasing all Spacemouse buttons - previous state: Low=0x%08lX, High=0x%08lX", 
                 _buttonReport.buttons[0], _buttonReport.buttons[1]);
    
    _buttonReport.buttons[0] = 0;
    _buttonReport.buttons[1] = 0;
    
    sendReport();
    SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "All Spacemouse buttons released");
}

#if MOUSE_ENABLE || DIGITIZER_ENABLE
void SQUIDSPACEMOUSE::click(SpacemouseKey b) {
    uint8_t button = static_cast<uint8_t>(b);
    if (button >= 1 && button <= 64) {
        uint8_t arrayIndex = (button - 1) / 32;
        uint32_t buttonMask = (1UL << ((button - 1) % 32));
        
        SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse click, button: %d", button);
        
        _buttonReport.buttons[arrayIndex] |= buttonMask;
        sendReport();
        delay(_delay_ms);
        _buttonReport.buttons[arrayIndex] &= ~buttonMask; // Release
        sendReport();
        
        SQUID_LOG_DEBUG(DIGI_TAG, "Mouse click completed");
    }
}

void SQUIDSPACEMOUSE::move(int16_t x, int16_t y, int16_t wheel, int16_t hWheel) {
    if (!isConnected() || !transport) {
        SQUID_LOG_DEBUG(MOUSE_TAG, "Relative mouse movement ignored - not connected");
        return;
    }
    
    _relativeX += x;
    _relativeY += y;
    
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse movement - X:%d, Y:%d, Wheel:%d, HWheel:%d (accumulated)", x, y);
    
    // Convert relative to absolute and send
    moveRelative(x, y, true);

}

void SQUIDSPACEMOUSE::moveRelative(int16_t relX, int16_t relY, bool sendImmediately) {
    
    // Convert relative movement to absolute coordinates
    int32_t newX = _currentAbsoluteX + relX;
    int32_t newY = _currentAbsoluteY + relY;
    
    // Apply screen boundaries
    newX = (newX < 0) ? 0 : (newX > _screenWidth) ? _screenWidth : newX;
    newY = (newY < 0) ? 0 : (newY > _screenHeight) ? _screenHeight : newY;
    
    // Update current position
    _currentAbsoluteX = static_cast<uint16_t>(newX);
    _currentAbsoluteY = static_cast<uint16_t>(newY);
    
    SQUID_LOG_DEBUG(MOUSE_TAG, "Relative->Absolute conversion - Rel:(%d,%d) -> Abs:(%u,%u)", 
                 relX, relY, _currentAbsoluteX, _currentAbsoluteY);
    
    moveTo(_currentAbsoluteX, _currentAbsoluteY, 0, SpacemouseKey{0});

}

void SQUIDSPACEMOUSE::sendMouseReport() {
  sendReport();
}

void SQUIDSPACEMOUSE::click(uint16_t x, uint16_t y, SpacemouseKey b) {
    uint8_t button = static_cast<uint8_t>(b);
    
    if (button < 1 || button > 64) {
        SQUID_LOG_WARN(DIGI_TAG, "Invalid button number: %d (must be 1-64)", button);
        return;
    }
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer click at X:%u, Y:%u, button: %d", x, y, button);
    
    _transReport.tx = x;
    _transReport.ty = y;
    
    // Press the button
    uint8_t arrayIndex = (button - 1) / 32;
    uint32_t buttonMask = (1UL << ((button - 1) % 32));
    _buttonReport.buttons[arrayIndex] |= buttonMask;
    
    sendReport();
    delay(_delay_ms);
    
    // Release just this button
    _buttonReport.buttons[arrayIndex] &= ~buttonMask;
    sendReport();
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer click completed");
}

void SQUIDSPACEMOUSE::moveTo(uint16_t x, uint16_t y, uint8_t pressure, SpacemouseKey buttons) {
    if (isConnected() && transport) {
        // Scale to HID descriptor's 0-32767 range
        uint16_t scaledX = (x * 32767ULL) / _screenWidth;
        uint16_t scaledY = (y * 32767ULL) / _screenHeight;
        uint16_t truePressure = pressure * 10;
        uint8_t buttonValue = static_cast<uint8_t>(buttons);
        
        _transReport.tx = scaledX;
        _transReport.ty = scaledY;
        _transReport.tz = truePressure;
        
        // Clear all buttons first
        _buttonReport.buttons[0] = 0;
        _buttonReport.buttons[1] = 0;
        
        // Set the specified button(s) if any
        if (buttonValue >= 1 && buttonValue <= 64) {
            uint8_t arrayIndex = (buttonValue - 1) / 32;
            uint32_t buttonMask = (1UL << ((buttonValue - 1) % 32));
            _buttonReport.buttons[arrayIndex] |= buttonMask;
        }
        
        sendReport();
        
        SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer move - X:%u->%u, Y:%u->%u, Pressure:%u, Button:%u",
                     x, scaledX, y, scaledY, pressure, buttonValue);
    } else {
        SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer movement ignored - %s%s", 
                     !isConnected() ? "not connected" : "",
                     !transport ? "no input characteristic" : "");
    }
}

void SQUIDSPACEMOUSE::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Beginning stroke at X:%u, Y:%u, initial pressure:%u", x, y, initialPressure);
    uint16_t truePressure = initialPressure * 10;
    moveTo(x, y, truePressure, SpacemouseKey{0});
}

void SQUIDSPACEMOUSE::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Updating stroke at X:%u, Y:%u, pressure:%u", x, y, pressure);
    uint16_t truePressure = pressure * 10;
    moveTo(x, y, pressure, SpacemouseKey{0});
}

void SQUIDSPACEMOUSE::endStroke(uint16_t x, uint16_t y) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Ending stroke at X:%u, Y:%u", x, y);
    moveTo(x, y, 0, SpacemouseKey{0});
}

void SQUIDSPACEMOUSE::setDigitizerRange(uint16_t maxX, uint16_t maxY) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Setting digitizer range - previous: X:%u, Y:%u", _screenWidth, _screenHeight);
    
    _screenWidth = maxX;
    _screenHeight = maxY;
    
    SQUID_LOG_INFO(DIGI_TAG, "Digitizer range set to X:%u, Y:%u", _screenWidth, _screenHeight);
}

void SQUIDSPACEMOUSE::sendDigitizerReport() {
  sendReport();
}
#endif

#if GAMEPAD_ENABLE
void SQUIDSPACEMOUSE::gamepadSetLeftStick(int16_t x, int16_t y) {
    _transReport.tx = x;
    _transReport.ty = y;
    sendReport();
}

void SQUIDSPACEMOUSE::gamepadSetRightStick(int16_t x, int16_t y) {
    _transReport.tz = x;
    _rotReport.rx   = y;
    sendReport();
}

void SQUIDSPACEMOUSE::gamepadSetTriggers(int16_t left, int16_t right) {
    _rotReport.ry = left;
    _rotReport.rz = right;
    sendReport();
}

void SQUIDSPACEMOUSE::gamepadSetAxis(SpacemouseAnalogue axis, int16_t value) {
    int8_t axisIndex = static_cast<int8_t>(axis);
    if (axisIndex < 6) {
        switch (axisIndex) {
        case 0:
            _transReport.tx = value;
            break;
        case 1:
            _transReport.ty = value;
            break;
        case 2:
            _transReport.tz = value;
            break;
        case 3:
            _rotReport.rx = value;
            break;
        case 4:
            _rotReport.ry = value;
            break;
        case 5:
            _rotReport.rz = value;
            break;
        default:
            break;
        }
        sendReport();
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid axis set attempt - Axis: %d, Value: %d", axisIndex, value);
    }
}

int16_t SQUIDSPACEMOUSE::gamepadGetAxis(SpacemouseAnalogue axis) {
    int8_t axisIndex = static_cast<int8_t>(axis);
    int16_t value = 0;

    if (axisIndex < 6) {
        switch (axisIndex) {
        case 0:
            value =_transReport.tx;
            break;
        case 1:
            value = _transReport.ty;
            break;
        case 2:
            value = _transReport.tz;
            break;
        case 3:
            value = _rotReport.rx;
            break;
        case 4:
            value = _rotReport.ry;
            break;
        case 5:
            value = _rotReport.rz;
            break;
        default:
            break;
        }
        
        return value;
        
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid axis get attempt - Axis: %d, Value: %d", axisIndex, value);
    }
}

void SQUIDSPACEMOUSE::gamepadSetAllAxes(int16_t values[6]) {
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Setting all analogue axes");
    _transReport.tx = values[0];
    _transReport.ty = values[1];
    _transReport.tz = values[2];
    _rotReport.rx   = values[3];
    _rotReport.ry   = values[4];
    _rotReport.rz   = values[5];
    sendReport();
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "All analogue axes set successfully");
}

void SQUIDSPACEMOUSE::sendGamepadReport() {
  sendReport();
}
#endif

void SQUIDSPACEMOUSE::sendReport() {
    if (!isConnected() || !transport) {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Cannot send Spacemouse report - not connected or no transport");
        return;
    }
    
    // Send translation report (Report ID 0x04)
    bool transResult = transport->sendReport(SPACETRANS_ID, (uint8_t*)&_transReport, sizeof(_transReport));
    
    // Send rotation report (Report ID 0x05)  
    bool rotResult = transport->sendReport(SPACEROTAT_ID, (uint8_t*)&_rotReport, sizeof(_rotReport));
    
    // Send button report (Report ID 0x06)
    bool buttonResult = transport->sendReport(SPACECLICK_ID, (uint8_t*)&_buttonReport, sizeof(_buttonReport));
    
    if (!transResult || !rotResult || !buttonResult) {
        SQUID_LOG_ERROR(SPACEMOUSE_TAG, "Failed to send Spacemouse reports - T:%s R:%s B:%s",
                     transResult ? "OK" : "FAIL",
                     rotResult ? "OK" : "FAIL", 
                     buttonResult ? "OK" : "FAIL");
    } else {
        SQUID_LOG_DEBUG(SPACEMOUSE_TAG, "Spacemouse reports sent successfully");
    }
    
    delay(_delay_ms);
}
