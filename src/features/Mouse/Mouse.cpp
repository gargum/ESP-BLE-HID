#include "Mouse.h"
#include "NimBLEDevice.h"

static const char* MOUSE_TAG = "BLEMOUSE";

BLEMOUSE::BLEMOUSE() 
    : inputMouse(nullptr), _mouseButtons(0), _delay_ms(7) {
    memset(&_mouseReport, 0, sizeof(_mouseReport));
}

void BLEMOUSE::begin(NimBLECharacteristic* mouseChar, uint32_t delay_ms) {
    inputMouse = mouseChar;
    _delay_ms = delay_ms;
    _mouseButtons = 0;
    memset(&_mouseReport, 0, sizeof(_mouseReport));
}

bool BLEMOUSE::isConnected() {
    if (!inputMouse) return false;
    return NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0;
}

size_t BLEMOUSE::press(char b) {
    _mouseButtons |= b;
    _mouseReport.buttons = _mouseButtons;
    sendMouseReport();
    return 1;
}

size_t BLEMOUSE::release(char b) {
    _mouseButtons &= ~b;
    _mouseReport.buttons = _mouseButtons;
    sendMouseReport();
    return 1;
}

void BLEMOUSE::releaseAll() {
    _mouseButtons = 0;
    _mouseReport.buttons = 0;
    sendMouseReport();
}

void BLEMOUSE::click(char b) {
    press(b);
    delay(_delay_ms);
    release(b);
}

void BLEMOUSE::move(signed char x, signed char y, signed char wheel, signed char hWheel) {
    if (isConnected() && inputMouse) {
        _mouseReport.buttons = _mouseButtons;
        
        // Set relative fields
        _mouseReport.relX = x;
        _mouseReport.relY = y;
        _mouseReport.wheel = wheel;
        _mouseReport.hWheel = hWheel;
        
        sendMouseReport();
    }
}

bool BLEMOUSE::mouseIsPressed(char b) {
    return (_mouseReport.buttons & b) != 0;
}

void BLEMOUSE::sendMouseReport() {
    if (isConnected() && inputMouse) {
        inputMouse->setValue((uint8_t*)&_mouseReport, sizeof(_mouseReport));
        inputMouse->notify();
        delay(_delay_ms);
    }
}
