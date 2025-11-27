/**
 * @file Spacemouse.h
 * @brief 3DConnexion Spacemouse feature header
 */
 
#ifndef SPACEMOUSE_H
#define SPACEMOUSE_H

#include <stdint.h>
#include "HIDTypes.h"
#include "../../drivers/Software/Log/Log.h"
#include "../../drivers/Software/Event/Types.h"
#include "../../drivers/Software/Transport/Transport.h"

#define SPACETRANS_ID 0x07 // 3DConnexion, why tf did you give one thing 3 report IDs?!?!
#define SPACEROTAT_ID 0x08 // Look at any gamepad's HID Report you guys, you don't have to do this
#define SPACECLICK_ID 0x09 // Why would you do it like this?! 

// Spacemouse report structures
typedef struct {
    int16_t tx;  // Translation X
    int16_t ty;  // Translation Y  
    int16_t tz;  // Translation Z
} SpaceTranslationReport;

typedef struct {
    int16_t rx;  // Rotation X
    int16_t ry;  // Rotation Y
    int16_t rz;  // Rotation Z
} SpaceRotationReport;

typedef struct {
    uint32_t buttons;  // 32 bits for button bitmask
} SpaceButtonReport;

static const uint8_t _spacemouseReportDescriptor[] = {
  0x05, 0x01,           // Usage Page (Generic Desktop)
  0x09, 0x08,           //  0x08: Usage (Multi-Axis)
  0xa1, 0x01,           // Collection (Application)
  0xa1, 0x00,           //   Collection (Physical)
  0x85, SPACETRANS_ID,  //     Report ID
  0x16, 0x00, 0x80,     //     Logical minimum (-500)
  0x26, 0xff, 0x7f,     //     Logical maximum (500)
  0x36, 0x00, 0x80,     //     Physical Minimum (-32768)
  0x46, 0xff, 0x7f,     //     Physical Maximum (32767)
  0x09, 0x30,           //     Usage (X)
  0x09, 0x31,           //     Usage (Y)
  0x09, 0x32,           //     Usage (Z)
  0x75, 0x10,           //     Report Size (16)
  0x95, 0x03,           //     Report Count (3)
  0x81, 0x02,           //     Input (variable,absolute)
  0xC0,                 //   End Collection

  0xa1, 0x00,           //   Collection (Physical)
  0x85, SPACEROTAT_ID,  //     Report ID
  0x16, 0x00, 0x80,     //     Logical minimum (-500)
  0x26, 0xff, 0x7f,     //     Logical maximum (500)
  0x36, 0x00, 0x80,     //     Physical Minimum (-32768)
  0x46, 0xff, 0x7f,     //     Physical Maximum (32767)
  0x09, 0x33,           //     Usage (RX)
  0x09, 0x34,           //     Usage (RY)
  0x09, 0x35,           //     Usage (RZ)
  0x75, 0x10,           //     Report Size (16)
  0x95, 0x03,           //     Report Count (3)
  0x81, 0x02,           //     Input (variable,absolute)
  0xC0,                 //   End Collection

  0xa1, 0x00,           //   Collection (Physical)
  0x85, SPACECLICK_ID,  //     Report ID
  0x15, 0x00,           //     Logical Minimum (0)
  0x25, 0x01,           //     Logical Maximum (1)
  0x75, 0x01,           //     Report Size (1)
  0x95, 32,             //     Report Count (24)
  0x05, 0x09,           //     Usage Page (Button)
  0x19, 1,              //     Usage Minimum (Button #1)
  0x29, 32,             //     Usage Maximum (Button #24)
  0x81, 0x02,           //     Input (variable,absolute)
  0xC0,                 //   End Collection
  0xC0,                 // End Collection
};

enum class SpacemouseKeys : uint32_t {
  SPACEMOUSE_1  = 0x00000001,
  SPACEMOUSE_2  = 0x00000002,
  SPACEMOUSE_3  = 0x00000004,
  SPACEMOUSE_4  = 0x00000008,
  SPACEMOUSE_5  = 0x00000010,
  SPACEMOUSE_6  = 0x00000020,
  SPACEMOUSE_7  = 0x00000040,
  SPACEMOUSE_8  = 0x00000080,
  SPACEMOUSE_9  = 0x00000100,
  SPACEMOUSE_10 = 0x00000200,
  SPACEMOUSE_11 = 0x00000400,
  SPACEMOUSE_12 = 0x00000800,
  SPACEMOUSE_13 = 0x00001000,
  SPACEMOUSE_14 = 0x00002000,
  SPACEMOUSE_15 = 0x00004000,
  SPACEMOUSE_16 = 0x00008000,
  SPACEMOUSE_17 = 0x00010000,
  SPACEMOUSE_18 = 0x00020000,
  SPACEMOUSE_19 = 0x00040000,
  SPACEMOUSE_20 = 0x00080000,
  SPACEMOUSE_21 = 0x00100000,
  SPACEMOUSE_22 = 0x00200000,
  SPACEMOUSE_23 = 0x00400000,
  SPACEMOUSE_24 = 0x00800000,
  SPACEMOUSE_25 = 0x01000000,
  SPACEMOUSE_26 = 0x02000000,
  SPACEMOUSE_27 = 0x04000000,
  SPACEMOUSE_28 = 0x08000000,
  SPACEMOUSE_29 = 0x10000000,
  SPACEMOUSE_30 = 0x20000000,
  SPACEMOUSE_31 = 0x40000000,
  SPACEMOUSE_32 = 0x80000000
};

MK(SpacemouseKey, SM_01, SPACEMOUSE_1);
MK(SpacemouseKey, SM_02, SPACEMOUSE_2);
MK(SpacemouseKey, SM_03, SPACEMOUSE_3);
MK(SpacemouseKey, SM_04, SPACEMOUSE_4);
MK(SpacemouseKey, SM_05, SPACEMOUSE_5);
MK(SpacemouseKey, SM_06, SPACEMOUSE_6);
MK(SpacemouseKey, SM_07, SPACEMOUSE_7);
MK(SpacemouseKey, SM_08, SPACEMOUSE_8);
MK(SpacemouseKey, SM_09, SPACEMOUSE_9);
MK(SpacemouseKey, SM_10, SPACEMOUSE_10);
MK(SpacemouseKey, SM_11, SPACEMOUSE_11);
MK(SpacemouseKey, SM_12, SPACEMOUSE_12);
MK(SpacemouseKey, SM_13, SPACEMOUSE_13);
MK(SpacemouseKey, SM_14, SPACEMOUSE_14);
MK(SpacemouseKey, SM_15, SPACEMOUSE_15);
MK(SpacemouseKey, SM_16, SPACEMOUSE_16);
MK(SpacemouseKey, SM_17, SPACEMOUSE_17);
MK(SpacemouseKey, SM_18, SPACEMOUSE_18);
MK(SpacemouseKey, SM_19, SPACEMOUSE_19);
MK(SpacemouseKey, SM_20, SPACEMOUSE_20);
MK(SpacemouseKey, SM_21, SPACEMOUSE_21);
MK(SpacemouseKey, SM_22, SPACEMOUSE_22);
MK(SpacemouseKey, SM_23, SPACEMOUSE_23);
MK(SpacemouseKey, SM_24, SPACEMOUSE_24);
MK(SpacemouseKey, SM_25, SPACEMOUSE_25);
MK(SpacemouseKey, SM_26, SPACEMOUSE_26);
MK(SpacemouseKey, SM_27, SPACEMOUSE_27);
MK(SpacemouseKey, SM_28, SPACEMOUSE_28);
MK(SpacemouseKey, SM_29, SPACEMOUSE_29);
MK(SpacemouseKey, SM_30, SPACEMOUSE_30);
MK(SpacemouseKey, SM_31, SPACEMOUSE_31);
MK(SpacemouseKey, SM_32, SPACEMOUSE_32);

class SQUIDSPACEMOUSE {
private:
    Transport*              transport;
    SpaceTranslationReport  _transReport;
    SpaceRotationReport     _rotReport;  
    SpaceButtonReport       _buttonReport;
    uint32_t                _delay_ms;
    
public:
    SQUIDSPACEMOUSE();
    
    void begin(Transport* transport, uint32_t delay_ms = 7);
    bool isConnected();
    void onConnect();
    void onDisconnect();
    
    // Space Mouse methods
    void move(int16_t tx, int16_t ty, int16_t tz, int16_t rx, int16_t ry, int16_t rz);
    void translate(int16_t tx, int16_t ty, int16_t tz);
    void rotate(int16_t rx, int16_t ry, int16_t rz);
    void press(uint8_t button);
    void release(uint8_t button);
    bool isPressed(uint8_t button);
    void setAllButtons(uint32_t buttons);
    void sendReport();
    void releaseAll();
};

#endif
