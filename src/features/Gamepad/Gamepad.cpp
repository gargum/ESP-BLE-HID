#include "Gamepad.h"
#include "NimBLEDevice.h"
#include "../Log/Log.h"

static const char* GAMEPAD_TAG = "BLEGAMEPAD";

BLEGAMEPAD::BLEGAMEPAD() 
    : inputGamepad(nullptr), _delay_ms(7) {
    memset(&_gamepadReport, 0, sizeof(_gamepadReport));
    _gamepadReport.hat = HAT_CE;
    
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad instance created");
}

void BLEGAMEPAD::begin(NimBLECharacteristic* gamepadChar, uint32_t delay_ms) {
    inputGamepad = gamepadChar;
    _delay_ms = delay_ms;
    memset(&_gamepadReport, 0, sizeof(_gamepadReport));
    _gamepadReport.hat = HAT_CE;
    
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad subsystem initialized with delay: %lu ms", delay_ms);
    BLE_LOG_INFO(GAMEPAD_TAG, "Gamepad service ready");
}

bool BLEGAMEPAD::isConnected() {
    bool connected = (inputGamepad && NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0);
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Connection check: %s", connected ? "connected" : "disconnected");
    return connected;
}

size_t BLEGAMEPAD::press(int8_t button) {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button press: %d", button);
    
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        uint32_t previousButtons = _gamepadReport.buttons[field];
        _gamepadReport.buttons[field] |= (1UL << bit);
        
        BLE_LOG_DEBUG(GAMEPAD_TAG, "Regular button press - Field: %u, Bit: %u, Before: 0x%08lX, After: 0x%08lX", 
                     field, bit, previousButtons, _gamepadReport.buttons[field]);
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatPress[directionIndex][currentHat];
        
        BLE_LOG_DEBUG(GAMEPAD_TAG, "DPad button press - Direction: %u, Before: 0x%02X, After: 0x%02X", 
                     directionIndex, currentHat, _gamepadReport.hat);
    } else {
        BLE_LOG_WARN(GAMEPAD_TAG, "Invalid button press attempt: %d", button);
        return 0;
    }
    
    sendGamepadReport();
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button press completed: %d", button);
    return 1;
}

size_t BLEGAMEPAD::release(int8_t button) {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button release: %d", button);
    
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        uint32_t previousButtons = _gamepadReport.buttons[field];
        _gamepadReport.buttons[field] &= ~(1UL << bit);
        
        BLE_LOG_DEBUG(GAMEPAD_TAG, "Regular button release - Field: %u, Bit: %u, Before: 0x%08lX, After: 0x%08lX", 
                     field, bit, previousButtons, _gamepadReport.buttons[field]);
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatRelease[directionIndex][currentHat];
        
        BLE_LOG_DEBUG(GAMEPAD_TAG, "DPad button release - Direction: %u, Before: 0x%02X, After: 0x%02X", 
                     directionIndex, currentHat, _gamepadReport.hat);
    } else {
        BLE_LOG_WARN(GAMEPAD_TAG, "Invalid button release attempt: %d", button);
        return 0;
    }
    
    sendGamepadReport();
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad button release completed: %d", button);
    return 1;
}

void BLEGAMEPAD::releaseAll() {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Releasing all gamepad buttons - Buttons[0]: 0x%08lX, Buttons[1]: 0x%08lX, Hat: 0x%02X", 
                 _gamepadReport.buttons[0], _gamepadReport.buttons[1], _gamepadReport.hat);
    
    _gamepadReport.buttons[0] = 0;
    _gamepadReport.buttons[1] = 0;
    _gamepadReport.hat = HAT_CE;
    
    sendGamepadReport();
    BLE_LOG_DEBUG(GAMEPAD_TAG, "All gamepad buttons released");
}

bool BLEGAMEPAD::gamepadIsPressed(int8_t button) {
    bool pressed = false;
    
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        pressed = (_gamepadReport.buttons[field] & (1UL << bit)) != 0;
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        
        switch (button) {
            case 65: // DPAD_UP
                pressed = (currentHat == HAT_UP || currentHat == HAT_UR || currentHat == HAT_UL);
                break;
            case 66: // DPAD_RIGHT
                pressed = (currentHat == HAT_RI || currentHat == HAT_UR || currentHat == HAT_DR);
                break;
            case 67: // DPAD_DOWN
                pressed = (currentHat == HAT_DO || currentHat == HAT_DR || currentHat == HAT_DL);
                break;
            case 68: // DPAD_LEFT
                pressed = (currentHat == HAT_LE || currentHat == HAT_UL || currentHat == HAT_DL);
                break;
        }
    } else {
        BLE_LOG_WARN(GAMEPAD_TAG, "Invalid button check attempt: %d", button);
        return false;
    }
    
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Button pressed check - Button: %d, Pressed: %s", 
                 button, pressed ? "true" : "false");
    return pressed;
} 

void BLEGAMEPAD::gamepadSetAxis(int8_t axis, int16_t value) {
    if (axis < GAMEPAD_AXIS_COUNT) {
        int16_t previousValue = _gamepadReport.axes[axis];
        _gamepadReport.axes[axis] = value;
        
        BLE_LOG_DEBUG(GAMEPAD_TAG, "Axis set - Axis: %d, Value: %d -> %d", 
                     axis, previousValue, value);
        sendGamepadReport();
    } else {
        BLE_LOG_WARN(GAMEPAD_TAG, "Invalid axis set attempt - Axis: %d, Value: %d", axis, value);
    }
}

int16_t BLEGAMEPAD::gamepadGetAxis(int8_t axis) {
    int16_t value = 0;
    
    if (axis < GAMEPAD_AXIS_COUNT) {
        value = _gamepadReport.axes[axis];
        BLE_LOG_DEBUG(GAMEPAD_TAG, "Axis get - Axis: %d, Value: %d", axis, value);
    } else {
        BLE_LOG_WARN(GAMEPAD_TAG, "Invalid axis get attempt: %d", axis);
    }
    
    return value;
}

void BLEGAMEPAD::gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]) {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Setting all axes");
    memcpy(_gamepadReport.axes, values, sizeof(_gamepadReport.axes));
    sendGamepadReport();
    BLE_LOG_DEBUG(GAMEPAD_TAG, "All axes set successfully");
}

void BLEGAMEPAD::gamepadSetLeftStick(int16_t x, int16_t y) {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Setting left stick - X: %d -> %d, Y: %d -> %d", 
                 _gamepadReport.axes[AXIS_LX], x, _gamepadReport.axes[AXIS_LY], y);
    
    _gamepadReport.axes[AXIS_LX] = x;
    _gamepadReport.axes[AXIS_LY] = y;
    sendGamepadReport();
}

void BLEGAMEPAD::gamepadSetRightStick(int16_t x, int16_t y) {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Setting right stick - X: %d -> %d, Y: %d -> %d", 
                 _gamepadReport.axes[AXIS_RX], x, _gamepadReport.axes[AXIS_RY], y);
    
    _gamepadReport.axes[AXIS_RX] = x;
    _gamepadReport.axes[AXIS_RY] = y;
    sendGamepadReport();
}

void BLEGAMEPAD::gamepadSetTriggers(int16_t left, int16_t right) {
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Setting triggers - Left: %d -> %d, Right: %d -> %d", 
                 _gamepadReport.axes[AXIS_LT], left, _gamepadReport.axes[AXIS_RT], right);
    
    _gamepadReport.axes[AXIS_LT] = left;
    _gamepadReport.axes[AXIS_RT] = right;
    sendGamepadReport();
}

void BLEGAMEPAD::gamepadGetLeftStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_LX);
    y = gamepadGetAxis(AXIS_LY);
    
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Getting left stick - X: %d, Y: %d", x, y);
}

void BLEGAMEPAD::gamepadGetRightStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_RX);
    y = gamepadGetAxis(AXIS_RY);
    
    BLE_LOG_DEBUG(GAMEPAD_TAG, "Getting right stick - X: %d, Y: %d", x, y);
}

void BLEGAMEPAD::sendGamepadReport() {
    if (!isConnected() || !inputGamepad) {
        BLE_LOG_DEBUG(GAMEPAD_TAG, "Cannot send gamepad report - %s%s", 
                     !isConnected() ? "not connected" : "", 
                     !inputGamepad ? "no input characteristic" : "");
        return;
    }
    
    inputGamepad->setValue((uint8_t*)&_gamepadReport, sizeof(_gamepadReport));
    
    if (inputGamepad->notify()) {
        BLE_LOG_DEBUG(GAMEPAD_TAG, "Gamepad report sent successfully - "
                     "Buttons[0]: 0x%08lX, Buttons[1]: 0x%08lX, Hat: 0x%02X, "
                     "LX: %d, LY: %d, RX: %d, RY: %d, LT: %d, RT: %d",
                     _gamepadReport.buttons[0], _gamepadReport.buttons[1], _gamepadReport.hat,
                     _gamepadReport.axes[AXIS_LX], _gamepadReport.axes[AXIS_LY],
                     _gamepadReport.axes[AXIS_RX], _gamepadReport.axes[AXIS_RY],
                     _gamepadReport.axes[AXIS_LT], _gamepadReport.axes[AXIS_RT]);
    } else {
        BLE_LOG_WARN(GAMEPAD_TAG, "Failed to send gamepad report notification");
    }
    
    delay(_delay_ms);
}
