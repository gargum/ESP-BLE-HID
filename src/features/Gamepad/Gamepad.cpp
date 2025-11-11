#include "Gamepad.h"
#include "NimBLEDevice.h"

static const char* GAMEPAD_TAG = "BLEGAMEPAD";

BLEGAMEPAD::BLEGAMEPAD() 
    : inputGamepad(nullptr), _delay_ms(7) {
    memset(&_gamepadReport, 0, sizeof(_gamepadReport));
    _gamepadReport.hat = HAT_CE;
}

void BLEGAMEPAD::begin(NimBLECharacteristic* gamepadChar, uint32_t delay_ms) {
    inputGamepad = gamepadChar;
    _delay_ms = delay_ms;
    memset(&_gamepadReport, 0, sizeof(_gamepadReport));
    _gamepadReport.hat = HAT_CE;
}

bool BLEGAMEPAD::isConnected() {
    if (!inputGamepad) return false;
    return NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0;
}

size_t BLEGAMEPAD::press(int8_t button) {
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        _gamepadReport.buttons[field] |= (1UL << bit);
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatPress[directionIndex][currentHat];
    }
    sendGamepadReport();
    return 1;
}

size_t BLEGAMEPAD::release(int8_t button) {
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        _gamepadReport.buttons[field] &= ~(1UL << bit);
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatRelease[directionIndex][currentHat];
    }
    sendGamepadReport();
    return 1;
}

void BLEGAMEPAD::releaseAll() {
  _gamepadReport.buttons[0] = 0;
  _gamepadReport.buttons[1] = 0;
  _gamepadReport.hat = HAT_CE;
  sendGamepadReport();
}

bool BLEGAMEPAD::gamepadIsPressed(int8_t button) {
  if (button >= 1 && button <= 64) {
    uint8_t field = (button - 1) / 32;
    uint8_t bit = (button - 1) % 32;
    return (_gamepadReport.buttons[field] & (1UL << bit)) != 0;
    return false;
  } else if (button >= 65 && button <= 68) {
    uint8_t currentHat = _gamepadReport.hat;
    
    switch (button) {
      case 65: // DPAD_UP
        return (currentHat == HAT_UP || currentHat == HAT_UR || currentHat == HAT_UL);
      case 66: // DPAD_RIGHT
        return (currentHat == HAT_RI || currentHat == HAT_UR || currentHat == HAT_DR);
      case 67: // DPAD_DOWN
        return (currentHat == HAT_DO || currentHat == HAT_DR || currentHat == HAT_DL);
      case 68: // DPAD_LEFT
        return (currentHat == HAT_LE || currentHat == HAT_UL || currentHat == HAT_DL);
    }
  }
  return false;
} 

void BLEGAMEPAD::gamepadSetAxis(int8_t axis, int16_t value) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    _gamepadReport.axes[axis] = value;
  }
  sendGamepadReport();
}

int16_t BLEGAMEPAD::gamepadGetAxis(int8_t axis) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    return _gamepadReport.axes[axis];
  }
  return 0;
}

void BLEGAMEPAD::gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]) {
  memcpy(_gamepadReport.axes, values, sizeof(_gamepadReport.axes));
  sendGamepadReport();
}

void BLEGAMEPAD::gamepadSetLeftStick(int16_t x, int16_t y) {
    _gamepadReport.axes[AXIS_LX] = x;
    _gamepadReport.axes[AXIS_LY] = y;
    sendGamepadReport();
}

void BLEGAMEPAD::gamepadSetRightStick(int16_t x, int16_t y) {
    _gamepadReport.axes[AXIS_RX] = x;
    _gamepadReport.axes[AXIS_RY] = y;
    sendGamepadReport();
}

void BLEGAMEPAD::gamepadSetTriggers(int16_t left, int16_t right) {
    _gamepadReport.axes[AXIS_LT] = left;
    _gamepadReport.axes[AXIS_RT] = right;
    sendGamepadReport();
}

void BLEGAMEPAD::gamepadGetLeftStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_LX);
    y = gamepadGetAxis(AXIS_LY);
}

void BLEGAMEPAD::gamepadGetRightStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_RX);
    y = gamepadGetAxis(AXIS_RY);
}

void BLEGAMEPAD::sendGamepadReport() {
    if (!isConnected() || !inputGamepad) return;
    inputGamepad->setValue((uint8_t*)&_gamepadReport, sizeof(_gamepadReport));
    inputGamepad->notify();
    delay(_delay_ms);
}
