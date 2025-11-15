/**
 * @file Mouse.cpp
 * @brief Implementation of the relative mouse pointer
 */

#include "Mouse.h"
#include "../../drivers/Log/Log.h"

static const char* MOUSE_TAG = "SQUIDMOUSE";

SQUIDMOUSE::SQUIDMOUSE() 
    : inputMouse(nullptr), _mouseKeys(MouseKey{0}), _delay_ms(7) {
    memset(&_mouseReport, 0, sizeof(_mouseReport));
}

void SQUIDMOUSE::begin(SquidCharacteristic* mouseChar, uint32_t delay_ms) {
    inputMouse = mouseChar;
    _delay_ms = delay_ms;
    _mouseKeys = MouseKey{0};
    memset(&_mouseReport, 0, sizeof(_mouseReport));
    
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse subsystem initialized with delay: %lu ms", delay_ms);
    SQUID_LOG_INFO(MOUSE_TAG, "Mouse service ready");
}

bool SQUIDMOUSE::isConnected() {
    // We need to check if the characteristic and underlying BLE stack are connected
    // Since we don't have direct access to SquidDevice, we'll check if the characteristic exists
    // and assume connection state is managed by the parent SQUIDHID class
    bool connected = (inputMouse != nullptr);
    SQUID_LOG_DEBUG(MOUSE_TAG, "Connection check: %s", connected ? "characteristic available" : "no characteristic");
    return connected;
}

size_t SQUIDMOUSE::press(MouseKey b) {
    // Convert to underlying type for bit operations
    uint8_t buttonValue = static_cast<uint8_t>(b);
    uint8_t currentKeys = static_cast<uint8_t>(_mouseKeys);
    
    currentKeys |= buttonValue;
    _mouseKeys = MouseKey{currentKeys};
    _mouseReport.buttons = currentKeys;
    
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse button pressed: 0x%02X, button state: 0x%02X", 
                 buttonValue, currentKeys);
    sendMouseReport();
    return 1;
}

size_t SQUIDMOUSE::release(MouseKey b) {
    // Convert to underlying type for bit operations
    uint8_t buttonValue = static_cast<uint8_t>(b);
    uint8_t currentKeys = static_cast<uint8_t>(_mouseKeys);
    
    currentKeys &= ~buttonValue;
    _mouseKeys = MouseKey{currentKeys};
    _mouseReport.buttons = currentKeys;
    
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse button released: 0x%02X, button state: 0x%02X", 
                 buttonValue, currentKeys);
    sendMouseReport();
    return 1;
}

void SQUIDMOUSE::releaseAll() {
    SQUID_LOG_DEBUG(MOUSE_TAG, "Releasing all mouse buttons - previous state: 0x%02X", 
                 static_cast<uint8_t>(_mouseKeys));
    _mouseKeys = MouseKey{0};
    _mouseReport.buttons = 0;
    sendMouseReport();
    SQUID_LOG_DEBUG(MOUSE_TAG, "All mouse buttons released");
}

void SQUIDMOUSE::click(MouseKey b) {
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse click: 0x%02X", static_cast<uint8_t>(b));
    press(b);
    delay(_delay_ms);
    release(b);
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse click completed: 0x%02X", static_cast<uint8_t>(b));
}

void SQUIDMOUSE::move(signed char x, signed char y, signed char wheel, signed char hWheel) {
    if (isConnected() && inputMouse) {
        _mouseReport.buttons = static_cast<uint8_t>(_mouseKeys);  // Convert to underlying type
        
        // Set relative fields
        _mouseReport.relX = x;
        _mouseReport.relY = y;
        _mouseReport.wheel = wheel;
        _mouseReport.hWheel = hWheel;
        
        SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse movement - X: %d, Y: %d, Wheel: %d, HWheel: %d, Buttons: 0x%02X", 
                     x, y, wheel, hWheel, static_cast<uint8_t>(_mouseKeys));
        sendMouseReport();
    } else {
        SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse movement ignored - %s", 
                     !isConnected() ? "not connected" : "no input characteristic");
    }
}

bool SQUIDMOUSE::mouseIsPressed(MouseKey b) {
    uint8_t buttonValue = static_cast<uint8_t>(b);
    uint8_t currentKeys = static_cast<uint8_t>(_mouseKeys);
    bool pressed = (currentKeys & buttonValue) != 0;
    
    SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse button check - Button: 0x%02X, Pressed: %s", 
                 buttonValue, pressed ? "true" : "false");
    return pressed;
}

void SQUIDMOUSE::sendMouseReport() {
    if (isConnected() && inputMouse) {
        inputMouse->setValue((uint8_t*)&_mouseReport, sizeof(_mouseReport));
        
        if (inputMouse->notify()) {
            SQUID_LOG_DEBUG(MOUSE_TAG, "Mouse report sent successfully - "
                         "Buttons: 0x%02X, X: %d, Y: %d, Wheel: %d, HWheel: %d",
                         _mouseReport.buttons, _mouseReport.relX, _mouseReport.relY, 
                         _mouseReport.wheel, _mouseReport.hWheel);
        } else {
            SQUID_LOG_WARN(MOUSE_TAG, "Failed to send mouse report notification");
        }
        
        delay(_delay_ms);
    } else {
        SQUID_LOG_DEBUG(MOUSE_TAG, "Cannot send mouse report - %s", 
                     !isConnected() ? "not connected" : "no input characteristic");
    }
}
