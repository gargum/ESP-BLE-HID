/**
 * @file Digitizer.cpp
 * @brief Implementation of the touch digitizer
 */

#include "Digitizer.h"

#if !SPACEMOUSE_ENABLE

static const char* DIGI_TAG = "SQUIDTABLET";

SQUIDTABLET::SQUIDTABLET() 
    : transport(nullptr), _delay_ms(7), 
      _screenWidth(1920), _screenHeight(1080) {
    memset(&_digitizerReport, 0, sizeof(_digitizerReport));
}

void SQUIDTABLET::begin(Transport* trans, uint32_t delay_ms) {
    transport = trans;
    _delay_ms = delay_ms;
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

void SQUIDTABLET::click(uint16_t x, uint16_t y, DigitizerKey b) {
    
    uint8_t digitizerButtons = static_cast<uint8_t>(b);
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer click at X:%u, Y:%u, buttons: 0x%02X", x, y, digitizerButtons);
    
    moveTo(x, y, 127, b); // Press with pressure
    delay(_delay_ms);
    moveTo(x, y, 0, DigitizerKey{0});   // Release
    
    SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer click completed");
}

void SQUIDTABLET::setDigitizerRange(uint16_t maxX, uint16_t maxY) {
    SQUID_LOG_DEBUG(DIGI_TAG, "Setting digitizer range - previous: X:%u, Y:%u", _screenWidth, _screenHeight);
    
    _screenWidth = maxX;
    _screenHeight = maxY;

    SQUID_LOG_INFO(DIGI_TAG, "Digitizer range set to X:%u, Y:%u", _screenWidth, _screenHeight);
}

void SQUIDTABLET::moveTo(uint16_t x, uint16_t y, uint8_t pressure, DigitizerKey buttons) {
    if (isConnected() && transport) {
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
        SQUID_LOG_DEBUG(DIGI_TAG, "Digitizer movement ignored - %s%s", 
                     !isConnected() ? "not connected" : "",
                     !transport ? "no input characteristic" : "");
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

#endif
