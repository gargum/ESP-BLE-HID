/**
 * @file Mouse.h
 * @brief Relative mouse pointer
 */
 
#ifndef MOUSE_H
#define MOUSE_H

#include "HIDTypes.h"
#include "NimBLECharacteristic.h"
#include <stdint.h>
#include "../Event/Types.h"

#define MOUSE_ID      0x04

typedef struct {
  uint8_t buttons;
  int8_t relX;
  int8_t relY;
  int8_t wheel;
  int8_t hWheel;
} MouseReport;

static const uint8_t _mouseReportDescriptor[] = {
  // ------------------------------------------------- Pointers - Relative/Mouse
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x02,             
  COLLECTION(1),      0x01,                      REPORT_ID(1),       MOUSE_ID,         
  USAGE(1),           0x01,                      COLLECTION(1),      0x00,             
  // Buttons (5 bits)
  USAGE_PAGE(1),      0x09,                      USAGE_MINIMUM(1),   0x01,             
  USAGE_MAXIMUM(1),   0x05,                      LOGICAL_MINIMUM(1), 0x00,             
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,             
  REPORT_COUNT(1),    0x05,                      HIDINPUT(1),        0x02,             
  // Button padding (3 bits)
  REPORT_SIZE(1),     0x03,                      REPORT_COUNT(1),    0x01,             
  HIDINPUT(1),        0x03,             
  // Relative X, Y, Wheel
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x30,             
  USAGE(1),           0x31,                      USAGE(1),           0x38,             
  LOGICAL_MINIMUM(1), 0x81,                      LOGICAL_MAXIMUM(1), 0x7F,             
  REPORT_SIZE(1),     0x08,                      REPORT_COUNT(1),    0x03,             
  HIDINPUT(1),        0x06,             
  // Horizontal Wheel
  USAGE_PAGE(1),      0x0C,                      USAGE(2),           0x38, 0x02,            
  LOGICAL_MINIMUM(1), 0x81,                      LOGICAL_MAXIMUM(1), 0x7F,             
  REPORT_SIZE(1),     0x08,                      REPORT_COUNT(1),    0x01,             
  HIDINPUT(1),        0x06,                      END_COLLECTION(0),                    
  END_COLLECTION(0),   
};

enum class MouseKeys : uint8_t {
  MOUSE_LEFT    = 0x01,
  MOUSE_RIGHT   = 0x02,
  MOUSE_MIDDLE  = 0x04,
  MOUSE_BACK    = 0x08,
  MOUSE_FORWARD = 0x10
};

MK(MouseKey, MO_BTN1, MOUSE_LEFT);
MK(MouseKey, MO_BTN2, MOUSE_RIGHT);
MK(MouseKey, MO_BTN3, MOUSE_MIDDLE);
MK(MouseKey, MO_BTN4, MOUSE_BACK);
MK(MouseKey, MO_BTN5, MOUSE_FORWARD);

class BLEMOUSE {
private:
    NimBLECharacteristic* inputMouse;
    MouseReport _mouseReport;
    MouseKey    _mouseKeys;
    uint32_t    _delay_ms;
    
public:
    BLEMOUSE();
    
    void begin(NimBLECharacteristic* mouseChar, uint32_t delay_ms = 7);
    bool isConnected();
    
    // Mouse methods
    size_t press(MouseKey b = MO_BTN1);
    size_t release(MouseKey b = MO_BTN1);
    void click(MouseKey b = MO_BTN1);
    void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
    bool mouseIsPressed(MouseKey b = MO_BTN1);
    void sendMouseReport();
    void releaseAll();
};
#endif
