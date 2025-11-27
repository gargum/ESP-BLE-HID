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

#define SPACETRANS_ID 0x04 // 3DConnexion, why tf did you give one thing 3 report IDs?!?!
#define SPACEROTAT_ID 0x05 // Look at any gamepad's HID Report you guys, you don't have to do this
#define SPACECLICK_ID 0x06 // Why would you do it like this?! 

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
  // Spacemouse Translation axis
  USAGE_PAGE(1),       0x01,                      USAGE(1),            0x08,                      
  COLLECTION(1),       0x01,                      COLLECTION(1),       0x00,                    
  REPORT_ID(1),        SPACETRANS_ID,             LOGICAL_MINIMUM(2),  0x00, 0x80,            
  LOGICAL_MAXIMUM(2),  0xFF, 0x7F,                PHYSICAL_MINIMUM(2), 0x00, 0x80,           
  PHYSICAL_MAXIMUM(2), 0xFF, 0x7F,                USAGE(1),            0x30,                    
  USAGE(1),            0x31,                      USAGE(1),            0x32,                    
  REPORT_SIZE(1),      0x10,                      REPORT_COUNT(1),     0x03,                    
  HIDINPUT(1),         0x02,                      END_COLLECTION(0),
  // Spacemouse Rotation axis
  COLLECTION(1),       0x00,                      REPORT_ID(1),        SPACEROTAT_ID,
  LOGICAL_MINIMUM(2),  0x00, 0x80,                LOGICAL_MAXIMUM(2),  0xFF, 0x7F,            
  PHYSICAL_MINIMUM(2), 0x00, 0x80,                PHYSICAL_MAXIMUM(2), 0xFF, 0x7F,          
  USAGE(1),            0x33,                      USAGE(1),            0x34,                    
  USAGE(1),            0x35,                      REPORT_SIZE(1),      0x10,                    
  REPORT_COUNT(1),     0x03,                      HIDINPUT(1),         0x02,                    
  END_COLLECTION(0),
  // Spacemouse/3DConnexion Buttons (I added 32 of them)
  COLLECTION(1),       0x00,                      REPORT_ID(1),        SPACECLICK_ID,
  LOGICAL_MINIMUM(1),  0x00,                      LOGICAL_MAXIMUM(1),  0x01,                  
  REPORT_SIZE(1),      0x01,                      REPORT_COUNT(1),     0x20,                    
  USAGE_PAGE(1),       0x09,                      USAGE_MINIMUM(1),    0x01,                    
  USAGE_MAXIMUM(1),    0x20,                      HIDINPUT(1),         0x02,                    
  END_COLLECTION(0),                              END_COLLECTION(0),
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
    
    void   begin(Transport* transport, uint32_t delay_ms = 7);
    bool   isConnected();
    void   onConnect();
    void   onDisconnect();
    
    // Space Mouse methods
    void   move(int16_t tx, int16_t ty, int16_t tz, int16_t rx, int16_t ry, int16_t rz);
    void   translate(int16_t tx, int16_t ty, int16_t tz);
    void   rotate(int16_t rx, int16_t ry, int16_t rz);
    void   press(uint8_t button);
    void   release(uint8_t button);
    bool   isPressed(uint8_t button);
    void   setAllButtons(uint32_t buttons);
    void   sendReport();
    void   releaseAll();
    
};

#endif
