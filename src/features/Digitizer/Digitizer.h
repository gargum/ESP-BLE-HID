/**
 * @file Digitizer.h
 * @brief Touch digitizer with pressure sensitivity, tip switch, and barrel
 */
 
#ifndef DIGITIZER_H
#define DIGITIZER_H

#if !SPACEMOUSE_ENABLE

#include <stdint.h>
#include "HIDTypes.h"
#include "../../drivers/Software/Log/Log.h"
#include "../../drivers/Software/Event/Types.h"
#include "../../drivers/Software/Transport/Transport.h"

#define DIGITIZER_ID  0x08

typedef struct {
    uint8_t buttons;
    uint8_t flags;     // In range, tip switch, etc.
    uint16_t x;
    uint16_t y;
    uint8_t pressure;  // 0-127 for Android compatibility
} DigitizerReport;

static const uint8_t _digitizerReportDescriptor[] = {               
  // ------------------------------------------------- Pointers - Absolute/Digitizer
  USAGE_PAGE(1),      0x0D,                      USAGE(1),           0x01,              
  COLLECTION(1),      0x01,                      REPORT_ID(1),       DIGITIZER_ID,      
  USAGE(1),           0x20,                      COLLECTION(1),      0x00,              
  // Buttons: Barrel, Eraser (Tip Switch is separate, not a button)
  USAGE_PAGE(1),      0x09,                      USAGE_MINIMUM(1),   0x01,              
  USAGE_MAXIMUM(1),   0x02,                      LOGICAL_MINIMUM(1), 0x00,              
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,              
  REPORT_COUNT(1),    0x02,                      HIDINPUT(1),        0x02,              
  // Padding (6 bits)
  REPORT_SIZE(1),     0x06,                      REPORT_COUNT(1),    0x01,              
  HIDINPUT(1),        0x01,              
  // Stylus state flags: In Range, Tip Switch, Invert (eraser), Barrel Switch
  USAGE_PAGE(1),      0x0D,                      USAGE(1),           0x32,              
  USAGE(1),           0x42,                      USAGE(1),           0x3C,              
  USAGE(1),           0x44,                      LOGICAL_MINIMUM(1), 0x00,              
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,              
  REPORT_COUNT(1),    0x04,                      HIDINPUT(1),        0x02,              
  // Padding (4 bits)
  REPORT_SIZE(1),     0x04,                      REPORT_COUNT(1),    0x01,              
  HIDINPUT(1),        0x01,              
  // X, Y coordinates (absolute)
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x30,              
  USAGE(1),           0x31,                      LOGICAL_MINIMUM(1), 0x00,              
  LOGICAL_MAXIMUM(2), 0xFF, 0x7F,                REPORT_SIZE(1),     0x10,              
  REPORT_COUNT(1),    0x02,                      HIDINPUT(1),        0x02,              
  // Pressure
  USAGE_PAGE(1),      0x0D,                      USAGE(1),           0x30,              
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x7F,              
  REPORT_SIZE(1),     0x08,                      REPORT_COUNT(1),    0x01,              
  HIDINPUT(1),        0x02,                      END_COLLECTION(0),                       
  END_COLLECTION(0),   
};

// Button bitmask constants
enum class DigitizerKeys : uint8_t {
  DIGITIZER_BTN1 = 0x01, // Button 1 (tip button, if present)
  DIGITIZER_BTN2 = 0x02, // Button 2 (barrel/side button)
  DIGITIZER_BTN3 = 0x04  // Button 3 (eraser)
};

MK(DigitizerKey, DI_BTN1, DIGITIZER_BTN1);
MK(DigitizerKey, DI_BTN2, DIGITIZER_BTN2);
MK(DigitizerKey, DI_BTN3, DIGITIZER_BTN3);

// Flag constants (internal, for 'flags' field)
const uint8_t DIGITIZER_FLAG_IN_RANGE     = 0x01;  // Bit 0
const uint8_t DIGITIZER_FLAG_TIP_SWITCH   = 0x02;  // Bit 1
const uint8_t DIGITIZER_FLAG_INVERT       = 0x04;  // Bit 2 (eraser)
const uint8_t DIGITIZER_FLAG_BARREL_SW    = 0x08;  // Bit 3

class SQUIDTABLET {
private:
    Transport*            transport; 
    DigitizerReport       _digitizerReport;
    uint32_t              _delay_ms;
    uint16_t              _screenWidth;
    uint16_t              _screenHeight;
    
public:
    SQUIDTABLET();
    
    void begin(Transport* transport, uint32_t delay_ms = 7);
    bool isConnected();
    void onConnect();
    void onDisconnect();
    
    // Digitizer methods
    void click(uint16_t x, uint16_t y, DigitizerKey b = DI_BTN1);  // 1 = MOUSE_LEFT equivalent
    void moveTo(uint16_t x, uint16_t y, uint8_t pressure = 0, DigitizerKey buttons = DigitizerKey{0});
    void beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure = 127);
    void updateStroke(uint16_t x, uint16_t y, uint16_t pressure);
    void endStroke(uint16_t x, uint16_t y);
    void setDigitizerRange(uint16_t maxX, uint16_t maxY);
    void sendDigitizerReport();
};
#endif
#endif
