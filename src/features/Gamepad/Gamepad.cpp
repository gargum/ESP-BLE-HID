/**
 * @file Gamepad.cpp
 * @brief Implementation of the gamepad
 */

#include "Gamepad.h"
#include "../../drivers/Log/Log.h"

static const char* GAMEPAD_TAG = "SQUIDGAMEPAD";

SQUIDGAMEPAD::SQUIDGAMEPAD() 
    : inputGamepad(nullptr), _delay_ms(7) {
    memset(&_gamepadReport, 0, sizeof(_gamepadReport));
    _gamepadReport.hat = static_cast<int8_t>(HAT_CE);
    
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad instance created");
}

void SQUIDGAMEPAD::begin(SquidCharacteristic* gamepadChar, uint32_t delay_ms) {
    inputGamepad = gamepadChar;
    _delay_ms = delay_ms;
    memset(&_gamepadReport, 0, sizeof(_gamepadReport));
    _gamepadReport.hat = static_cast<int8_t>(HAT_CE);
    
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad subsystem initialized with delay: %lu ms", delay_ms);
    SQUID_LOG_INFO(GAMEPAD_TAG, "Gamepad service ready");
}

bool SQUIDGAMEPAD::isConnected() {
    // We need to check if the characteristic and underlying BLE stack are connected
    // Since we don't have direct access to SquidDevice, we'll check if the characteristic exists
    // and assume connection state is managed by the parent SQUIDHID class
    bool connected = (inputGamepad != nullptr);
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Connection check: %s", connected ? "characteristic available" : "no characteristic");
    return connected;
}

size_t SQUIDGAMEPAD::press(GamepadButton button) {
    int8_t buttonValue = static_cast<int8_t>(button);
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button press: %d", buttonValue);
    
    if (buttonValue >= 1 && buttonValue <= 64) {
        uint8_t field = (buttonValue - 1) / 32;
        uint8_t bit = (buttonValue - 1) % 32;
        uint32_t previousButtons = _gamepadReport.buttons[field];
        _gamepadReport.buttons[field] |= (1UL << bit);
        
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "Regular button press - Field: %u, Bit: %u, Before: 0x%08lX, After: 0x%08lX", 
                     field, bit, previousButtons, _gamepadReport.buttons[field]);
    } else if (buttonValue >= 65 && buttonValue <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = buttonValue - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = static_cast<int8_t>(hatPress[directionIndex][currentHat]);
        
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "DPad button press - Direction: %u, Before: 0x%02X, After: 0x%02X", 
                     directionIndex, currentHat, _gamepadReport.hat);
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid button press attempt: %d", buttonValue);
        return 0;
    }
    
    sendGamepadReport();
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button press completed: %d", buttonValue);
    return 1;
}

size_t SQUIDGAMEPAD::release(GamepadButton button) {
    int8_t buttonValue = static_cast<int8_t>(button);
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button release: %d", buttonValue);
    
    if (buttonValue >= 1 && buttonValue <= 64) {
        uint8_t field = (buttonValue - 1) / 32;
        uint8_t bit = (buttonValue - 1) % 32;
        uint32_t previousButtons = _gamepadReport.buttons[field];
        _gamepadReport.buttons[field] &= ~(1UL << bit);
        
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "Regular button release - Field: %u, Bit: %u, Before: 0x%08lX, After: 0x%08lX", 
                     field, bit, previousButtons, _gamepadReport.buttons[field]);
    } else if (buttonValue >= 65 && buttonValue <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = buttonValue - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = static_cast<int8_t>(hatRelease[directionIndex][currentHat]);
        
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "DPad button release - Direction: %u, Before: 0x%02X, After: 0x%02X", 
                     directionIndex, currentHat, _gamepadReport.hat);
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid button release attempt: %d", buttonValue);
        return 0;
    }
    
    sendGamepadReport();
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button release completed: %d", buttonValue);
    return 1;
}

void SQUIDGAMEPAD::releaseAll() {
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Releasing all gamepad buttons - Buttons[0]: 0x%08lX, Buttons[1]: 0x%08lX, Hat: 0x%02X", 
                 _gamepadReport.buttons[0], _gamepadReport.buttons[1], _gamepadReport.hat);
    
    _gamepadReport.buttons[0] = 0;
    _gamepadReport.buttons[1] = 0;
    _gamepadReport.hat = static_cast<int8_t>(HAT_CE);
    
    sendGamepadReport();
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "All gamepad buttons released");
}

bool SQUIDGAMEPAD::gamepadIsPressed(GamepadButton button) {
    int8_t buttonValue = static_cast<int8_t>(button);
    bool pressed = false;
    
    if (buttonValue >= 1 && buttonValue <= 64) {
        uint8_t field = (buttonValue - 1) / 32;
        uint8_t bit = (buttonValue - 1) % 32;
        pressed = (_gamepadReport.buttons[field] & (1UL << bit)) != 0;
    } else if (buttonValue >= 65 && buttonValue <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        
        switch (buttonValue) {
            case 65: // DPAD_UP
                pressed = (currentHat == static_cast<int8_t>(HAT_UP) || 
                          currentHat == static_cast<int8_t>(HAT_UR) || 
                          currentHat == static_cast<int8_t>(HAT_UL));
                break;
            case 66: // DPAD_RIGHT
                pressed = (currentHat == static_cast<int8_t>(HAT_RI) || 
                          currentHat == static_cast<int8_t>(HAT_UR) || 
                          currentHat == static_cast<int8_t>(HAT_DR));
                break;
            case 67: // DPAD_DOWN
                pressed = (currentHat == static_cast<int8_t>(HAT_DO) || 
                          currentHat == static_cast<int8_t>(HAT_DR) || 
                          currentHat == static_cast<int8_t>(HAT_DL));
                break;
            case 68: // DPAD_LEFT
                pressed = (currentHat == static_cast<int8_t>(HAT_LE) || 
                          currentHat == static_cast<int8_t>(HAT_UL) || 
                          currentHat == static_cast<int8_t>(HAT_DL));
                break;
        }
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid button check attempt: %d", buttonValue);
        return false;
    }
    
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Button pressed check - Button: %d, Pressed: %s", 
                 buttonValue, pressed ? "true" : "false");
    return pressed;
} 

void SQUIDGAMEPAD::gamepadSetAxis(GamepadAnalogue axis, int16_t value) {
    int8_t axisIndex = static_cast<int8_t>(axis);
    if (axisIndex < GAMEPAD_ANALOGUE_COUNT) {
        int16_t previousValue = _gamepadReport.analogues[axisIndex];
        _gamepadReport.analogues[axisIndex] = value;
        
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "Axis set - Axis: %d, Value: %d -> %d", 
                     axisIndex, previousValue, value);
        sendGamepadReport();
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid axis set attempt - Axis: %d, Value: %d", axisIndex, value);
    }
}

int16_t SQUIDGAMEPAD::gamepadGetAxis(GamepadAnalogue axis) {
    int8_t axisIndex = static_cast<int8_t>(axis);
    int16_t value = 0;
    
    if (axisIndex < GAMEPAD_ANALOGUE_COUNT) {
        value = _gamepadReport.analogues[axisIndex];
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "Axis get - Axis: %d, Value: %d", axisIndex, value);
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Invalid axis get attempt: %d", axisIndex);
    }
    
    return value;
}

void SQUIDGAMEPAD::gamepadSetAllAxes(int16_t values[GAMEPAD_ANALOGUE_COUNT]) {
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Setting all analogue axes");
    memcpy(_gamepadReport.analogues, values, sizeof(_gamepadReport.analogues));
    sendGamepadReport();
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "All analogue axes set successfully");
}

void SQUIDGAMEPAD::gamepadSetLeftStick(int16_t x, int16_t y) {
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Setting left stick - X: %d -> %d, Y: %d -> %d", 
                 _gamepadReport.analogues[static_cast<int8_t>(GA_LX)], x, 
                 _gamepadReport.analogues[static_cast<int8_t>(GA_LY)], y);
    
    _gamepadReport.analogues[static_cast<int8_t>(GA_LX)] = x;
    _gamepadReport.analogues[static_cast<int8_t>(GA_LY)] = y;
    sendGamepadReport();
}

void SQUIDGAMEPAD::gamepadSetRightStick(int16_t x, int16_t y) {
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Setting right stick - X: %d -> %d, Y: %d -> %d", 
                 _gamepadReport.analogues[static_cast<int8_t>(GA_RX)], x, 
                 _gamepadReport.analogues[static_cast<int8_t>(GA_RY)], y);
    
    _gamepadReport.analogues[static_cast<int8_t>(GA_RX)] = x;
    _gamepadReport.analogues[static_cast<int8_t>(GA_RY)] = y;
    sendGamepadReport();
}

void SQUIDGAMEPAD::gamepadSetTriggers(int16_t left, int16_t right) {
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Setting triggers - Left: %d -> %d, Right: %d -> %d", 
                 _gamepadReport.analogues[static_cast<int8_t>(GA_LT)], left, 
                 _gamepadReport.analogues[static_cast<int8_t>(GA_RT)], right);
    
    _gamepadReport.analogues[static_cast<int8_t>(GA_LT)] = left;
    _gamepadReport.analogues[static_cast<int8_t>(GA_RT)] = right;
    sendGamepadReport();
}

void SQUIDGAMEPAD::gamepadGetLeftStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(GA_LX);
    y = gamepadGetAxis(GA_LY);
    
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Getting left stick - X: %d, Y: %d", x, y);
}

void SQUIDGAMEPAD::gamepadGetRightStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(GA_RX);
    y = gamepadGetAxis(GA_RY);
    
    SQUID_LOG_DEBUG(GAMEPAD_TAG, "Getting right stick - X: %d, Y: %d", x, y);
}

void SQUIDGAMEPAD::sendGamepadReport() {
    if (!isConnected() || !inputGamepad) {
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "Cannot send gamepad report - %s%s", 
                     !isConnected() ? "not connected" : "", 
                     !inputGamepad ? "no input characteristic" : "");
        return;
    }
    
    inputGamepad->setValue((uint8_t*)&_gamepadReport, sizeof(_gamepadReport));
    
    if (inputGamepad->notify()) {
        SQUID_LOG_DEBUG(GAMEPAD_TAG, "Gamepad report sent successfully - "
                     "Buttons[0]: 0x%08lX, Buttons[1]: 0x%08lX, Hat: 0x%02X, "
                     "LX: %d, LY: %d, RX: %d, RY: %d, LT: %d, RT: %d",
                     _gamepadReport.buttons[0], _gamepadReport.buttons[1], _gamepadReport.hat,
                     _gamepadReport.analogues[static_cast<int8_t>(GA_LX)], _gamepadReport.analogues[static_cast<int8_t>(GA_LY)],
                     _gamepadReport.analogues[static_cast<int8_t>(GA_RX)], _gamepadReport.analogues[static_cast<int8_t>(GA_RY)],
                     _gamepadReport.analogues[static_cast<int8_t>(GA_LT)], _gamepadReport.analogues[static_cast<int8_t>(GA_RT)]);
    } else {
        SQUID_LOG_WARN(GAMEPAD_TAG, "Failed to send gamepad report notification");
    }
    
    delay(_delay_ms);
}
