/**
 * @file Mouse.h
 * @brief Relative mouse pointer
 */
 
#ifndef MOUSE_H
#define MOUSE_H

#if !SPACEMOUSE_ENABLE

#include "drivers/Software/HID/SquidHIDTypes.h"
#include "drivers/Software/Log/Log.h"
#include "drivers/Software/Event/Types.h"
#include "drivers/Software/Transport/Transport.h"

#define MOUSE_ID      0x07

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
  // Buttons (8 bits)
  USAGE_PAGE(1),      0x09,                      USAGE_MINIMUM(1),   0x01,             
  USAGE_MAXIMUM(1),   0x05,                      LOGICAL_MINIMUM(1), 0x00,             
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,             
  REPORT_COUNT(1),    0x08,                      HIDINPUT(1),        0x02,                        
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
  MOUSE_FORWARD = 0x10,
  MOUSE_SIDE_A  = 0x20,
  MOUSE_SIDE_B  = 0x40,
  MOUSE_SIDE_C  = 0x80
};

MK(MouseKey, MO_BTN1, MOUSE_LEFT);
MK(MouseKey, MO_BTN2, MOUSE_RIGHT);
MK(MouseKey, MO_BTN3, MOUSE_MIDDLE);
MK(MouseKey, MO_BTN4, MOUSE_BACK);
MK(MouseKey, MO_BTN5, MOUSE_FORWARD);
MK(MouseKey, MO_BTN6, MOUSE_SIDE_A);
MK(MouseKey, MO_BTN7, MOUSE_SIDE_B);
MK(MouseKey, MO_BTN8, MOUSE_SIDE_C);

enum class MouseAnalogues : uint8_t {
  MOUSE_X_AXIS    = 1,
  MOUSE_Y_AXIS    = 2,
  MOUSE_H_SCROLL  = 3,
  MOUSE_V_SCROLL  = 4
};

MK(MouseAnalogue, MO_AX, MOUSE_X_AXIS);
MK(MouseAnalogue, MO_AY, MOUSE_Y_AXIS);
MK(MouseAnalogue, MO_HS, MOUSE_H_SCROLL);
MK(MouseAnalogue, MO_VS, MOUSE_V_SCROLL);

class SQUIDMOUSE {
private:
    Transport*            transport; 
    MouseReport           _mouseReport;
    MouseKey              _mouseKeys;
    uint32_t              _delay_ms;
    
public:
    SQUIDMOUSE();
    
    void begin(Transport* transport, uint32_t delay_ms = 7);
    bool isConnected();
    void onConnect();
    void onDisconnect();
    
    // Mouse methods
    size_t press(MouseKey b = MO_BTN1);
    size_t release(MouseKey b = MO_BTN1);
    void click(MouseKey b = MO_BTN1);
    void move(int8_t x, int8_t y, int8_t wheel = 0, int8_t hWheel = 0);
    bool mouseIsPressed(MouseKey b = MO_BTN1);
    void sendMouseReport();
    void releaseAll();
};
#endif
#endif
