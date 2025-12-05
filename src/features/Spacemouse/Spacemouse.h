/**
 * @file Spacemouse.h
 * @brief 3DConnexion Spacemouse feature header
 */
 
#ifndef SPACEMOUSE_H
#define SPACEMOUSE_H

#include "drivers/Software/Log/Log.h"
#include "drivers/Software/Event/Types.h"
#include "drivers/Software/HID/SquidHIDTypes.h"
#include "drivers/Software/Transport/Transport.h"

#define SPACETRANS_ID 0x04 // 3DConnexion, why tf did you give one thing 3 report IDs?!?!
#define SPACEROTAT_ID 0x05 // Look at any gamepad's HID Report you guys, you don't have to do this
#define SPACECLICK_ID 0x06 // Why would you do it like this?! 

#if DIGITIZER_ENABLE
#define DEFAULT_WIDTH   1920
#define DEFAULT_HEIGHT  1080
#endif

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
    uint32_t buttons[2];  // 64 bits for button bitmask
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
  REPORT_SIZE(1),      0x01,                      REPORT_COUNT(1),     0x40,                    
  USAGE_PAGE(1),       0x09,                      USAGE_MINIMUM(1),    0x01,                    
  USAGE_MAXIMUM(1),    0x40,                      HIDINPUT(1),         0x02,                    
  END_COLLECTION(0),                              END_COLLECTION(0),
};

enum class SpacemouseAnalogues : uint8_t {

  SPACEMOUSE_TX = 0,  // Translation X
  SPACEMOUSE_TY = 1,  // Translation Y
  SPACEMOUSE_TZ = 2,  // Translation Z
  SPACEMOUSE_RX = 3,  // Rotation X
  SPACEMOUSE_RY = 4,  // Rotation Y
  SPACEMOUSE_RZ = 5   // Rotation Z

};

MK(SpacemouseAnalogue, SM_TX, SPACEMOUSE_TX);
MK(SpacemouseAnalogue, SM_TY, SPACEMOUSE_TY);
MK(SpacemouseAnalogue, SM_TZ, SPACEMOUSE_TZ);
MK(SpacemouseAnalogue, SM_RX, SPACEMOUSE_RX);
MK(SpacemouseAnalogue, SM_RY, SPACEMOUSE_RY);
MK(SpacemouseAnalogue, SM_RZ, SPACEMOUSE_RZ);

enum class SpacemouseKeys : uint32_t {
  SPACEMOUSE_1  = 1,
  SPACEMOUSE_2  = 2,
  SPACEMOUSE_3  = 3,
  SPACEMOUSE_4  = 4,
  SPACEMOUSE_5  = 5,
  SPACEMOUSE_6  = 6,
  SPACEMOUSE_7  = 7,
  SPACEMOUSE_8  = 8,
  SPACEMOUSE_9  = 9,
  SPACEMOUSE_10 = 10,
  SPACEMOUSE_11 = 11,
  SPACEMOUSE_12 = 12,
  SPACEMOUSE_13 = 13,
  SPACEMOUSE_14 = 14,
  SPACEMOUSE_15 = 15,
  SPACEMOUSE_16 = 16,
  SPACEMOUSE_17 = 17,
  SPACEMOUSE_18 = 18,
  SPACEMOUSE_19 = 19,
  SPACEMOUSE_20 = 20,
  SPACEMOUSE_21 = 21,
  SPACEMOUSE_22 = 22,
  SPACEMOUSE_23 = 23,
  SPACEMOUSE_24 = 24,
  SPACEMOUSE_25 = 25,
  SPACEMOUSE_26 = 26,
  SPACEMOUSE_27 = 27,
  SPACEMOUSE_28 = 28,
  SPACEMOUSE_29 = 29,
  SPACEMOUSE_30 = 30,
  SPACEMOUSE_31 = 31,
  SPACEMOUSE_32 = 32,
  SPACEMOUSE_33 = 33,
  SPACEMOUSE_34 = 34,
  SPACEMOUSE_35 = 35,
  SPACEMOUSE_36 = 36,
  SPACEMOUSE_37 = 37,
  SPACEMOUSE_38 = 38,
  SPACEMOUSE_39 = 39,
  SPACEMOUSE_40 = 40,
  SPACEMOUSE_41 = 41,
  SPACEMOUSE_42 = 42,
  SPACEMOUSE_43 = 43,
  SPACEMOUSE_44 = 44,
  SPACEMOUSE_45 = 45,
  SPACEMOUSE_46 = 46,
  SPACEMOUSE_47 = 47,
  SPACEMOUSE_48 = 48,
  SPACEMOUSE_49 = 49,
  SPACEMOUSE_50 = 50,
  SPACEMOUSE_51 = 51,
  SPACEMOUSE_52 = 52,
  SPACEMOUSE_53 = 53,
  SPACEMOUSE_54 = 54,
  SPACEMOUSE_55 = 55,
  SPACEMOUSE_56 = 56,
  SPACEMOUSE_57 = 57,
  SPACEMOUSE_58 = 58,
  SPACEMOUSE_59 = 59,
  SPACEMOUSE_60 = 60,
  SPACEMOUSE_61 = 61,
  SPACEMOUSE_62 = 62,
  SPACEMOUSE_63 = 63,
  SPACEMOUSE_64 = 64
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
MK(SpacemouseKey, SM_33, SPACEMOUSE_33);
MK(SpacemouseKey, SM_34, SPACEMOUSE_34);
MK(SpacemouseKey, SM_35, SPACEMOUSE_35);
MK(SpacemouseKey, SM_36, SPACEMOUSE_36);
MK(SpacemouseKey, SM_37, SPACEMOUSE_37);
MK(SpacemouseKey, SM_38, SPACEMOUSE_38);
MK(SpacemouseKey, SM_39, SPACEMOUSE_39);
MK(SpacemouseKey, SM_40, SPACEMOUSE_40);
MK(SpacemouseKey, SM_41, SPACEMOUSE_41);
MK(SpacemouseKey, SM_42, SPACEMOUSE_42);
MK(SpacemouseKey, SM_43, SPACEMOUSE_43);
MK(SpacemouseKey, SM_44, SPACEMOUSE_44);
MK(SpacemouseKey, SM_45, SPACEMOUSE_45);
MK(SpacemouseKey, SM_46, SPACEMOUSE_46);
MK(SpacemouseKey, SM_47, SPACEMOUSE_47);
MK(SpacemouseKey, SM_48, SPACEMOUSE_48);
MK(SpacemouseKey, SM_49, SPACEMOUSE_49);
MK(SpacemouseKey, SM_50, SPACEMOUSE_50);
MK(SpacemouseKey, SM_51, SPACEMOUSE_51);
MK(SpacemouseKey, SM_52, SPACEMOUSE_52);
MK(SpacemouseKey, SM_53, SPACEMOUSE_53);
MK(SpacemouseKey, SM_54, SPACEMOUSE_54);
MK(SpacemouseKey, SM_55, SPACEMOUSE_55);
MK(SpacemouseKey, SM_56, SPACEMOUSE_56);
MK(SpacemouseKey, SM_57, SPACEMOUSE_57);
MK(SpacemouseKey, SM_58, SPACEMOUSE_58);
MK(SpacemouseKey, SM_59, SPACEMOUSE_59);
MK(SpacemouseKey, SM_60, SPACEMOUSE_60);
MK(SpacemouseKey, SM_61, SPACEMOUSE_61);
MK(SpacemouseKey, SM_62, SPACEMOUSE_62);
MK(SpacemouseKey, SM_63, SPACEMOUSE_63);
MK(SpacemouseKey, SM_64, SPACEMOUSE_64);

#if MOUSE_ENABLE || DIGITIZER_ENABLE
MK(SpacemouseKey, MO_BTN1, SPACEMOUSE_1);
MK(SpacemouseKey, MO_BTN2, SPACEMOUSE_2);
MK(SpacemouseKey, MO_BTN3, SPACEMOUSE_3);
MK(SpacemouseKey, MO_BTN4, SPACEMOUSE_4);
MK(SpacemouseKey, MO_BTN5, SPACEMOUSE_5);
MK(SpacemouseKey, DI_BTN1, SPACEMOUSE_1);
MK(SpacemouseKey, DI_BTN2, SPACEMOUSE_2);
MK(SpacemouseKey, DI_BTN3, SPACEMOUSE_3);
#endif

#if GAMEPAD_ENABLE
MK(SpacemouseAnalogue, GA_LX, SPACEMOUSE_TX);
MK(SpacemouseAnalogue, GA_LY, SPACEMOUSE_TY);
MK(SpacemouseAnalogue, GA_RX, SPACEMOUSE_TZ);
MK(SpacemouseAnalogue, GA_RY, SPACEMOUSE_RX);
MK(SpacemouseAnalogue, GA_LT, SPACEMOUSE_RY);
MK(SpacemouseAnalogue, GA_RT, SPACEMOUSE_RZ);

MK(SpacemouseKey, GB_00, SPACEMOUSE_1);
MK(SpacemouseKey, GB_SO, SPACEMOUSE_1); // "South" like the bottom face button. So 'Cross' on a Playstation controller or 'A' on an Xbox controller.
MK(SpacemouseKey, GB_01, SPACEMOUSE_2);
MK(SpacemouseKey, GB_EA, SPACEMOUSE_2);  // "East" like the right face button. So 'Circle' on a Playstation controller or 'B' on an Xbox controller.
MK(SpacemouseKey, GB_02, SPACEMOUSE_3);
MK(SpacemouseKey, GB_03, SPACEMOUSE_4);
MK(SpacemouseKey, GB_WE, SPACEMOUSE_4);  // "West" like the left face button. So 'Square' on a Playstation controller or 'X' on an Xbox controller.
MK(SpacemouseKey, GB_04, SPACEMOUSE_5);
MK(SpacemouseKey, GB_NO, SPACEMOUSE_5); // "North" like the top face button. So 'Triangle' on a Playstation controller or 'Y' on an Xbox controller.
MK(SpacemouseKey, GB_05, SPACEMOUSE_6);
MK(SpacemouseKey, GB_06, SPACEMOUSE_7);
MK(SpacemouseKey, GB_L1, SPACEMOUSE_7);
MK(SpacemouseKey, GB_07, SPACEMOUSE_8);
MK(SpacemouseKey, GB_R1, SPACEMOUSE_8);
MK(SpacemouseKey, GB_08, SPACEMOUSE_9);
MK(SpacemouseKey, GB_09, SPACEMOUSE_10);
MK(SpacemouseKey, GB_10, SPACEMOUSE_11);
MK(SpacemouseKey, GB_BA, SPACEMOUSE_11);
MK(SpacemouseKey, GB_11, SPACEMOUSE_12);
MK(SpacemouseKey, GB_ST, SPACEMOUSE_12);
MK(SpacemouseKey, GB_12, SPACEMOUSE_13);
MK(SpacemouseKey, GB_GU, SPACEMOUSE_13);
MK(SpacemouseKey, GB_13, SPACEMOUSE_14);
MK(SpacemouseKey, GB_L3, SPACEMOUSE_14);
MK(SpacemouseKey, GB_14, SPACEMOUSE_15);
MK(SpacemouseKey, GB_R3, SPACEMOUSE_15);
MK(SpacemouseKey, GB_15, SPACEMOUSE_16);
MK(SpacemouseKey, GB_16, SPACEMOUSE_17);
MK(SpacemouseKey, GB_17, SPACEMOUSE_18);
MK(SpacemouseKey, GB_18, SPACEMOUSE_19);
MK(SpacemouseKey, GB_19, SPACEMOUSE_20);
MK(SpacemouseKey, GB_20, SPACEMOUSE_21);
MK(SpacemouseKey, GB_21, SPACEMOUSE_22);
MK(SpacemouseKey, GB_22, SPACEMOUSE_23);
MK(SpacemouseKey, GB_23, SPACEMOUSE_24);
MK(SpacemouseKey, GB_24, SPACEMOUSE_25);
MK(SpacemouseKey, GB_25, SPACEMOUSE_26);
MK(SpacemouseKey, GB_26, SPACEMOUSE_27);
MK(SpacemouseKey, GB_27, SPACEMOUSE_28);
MK(SpacemouseKey, GB_28, SPACEMOUSE_29);
MK(SpacemouseKey, GB_29, SPACEMOUSE_30);
MK(SpacemouseKey, GB_30, SPACEMOUSE_31);
MK(SpacemouseKey, GB_31, SPACEMOUSE_32);
MK(SpacemouseKey, GB_32, SPACEMOUSE_33);
MK(SpacemouseKey, GB_33, SPACEMOUSE_34);
MK(SpacemouseKey, GB_34, SPACEMOUSE_35);
MK(SpacemouseKey, GB_35, SPACEMOUSE_36);
MK(SpacemouseKey, GB_36, SPACEMOUSE_37);
MK(SpacemouseKey, GB_37, SPACEMOUSE_38);
MK(SpacemouseKey, GB_38, SPACEMOUSE_39);
MK(SpacemouseKey, GB_39, SPACEMOUSE_40);
MK(SpacemouseKey, GB_40, SPACEMOUSE_41);
MK(SpacemouseKey, GB_41, SPACEMOUSE_42);
MK(SpacemouseKey, GB_42, SPACEMOUSE_43);
MK(SpacemouseKey, GB_43, SPACEMOUSE_44);
MK(SpacemouseKey, GB_44, SPACEMOUSE_45);
MK(SpacemouseKey, GB_45, SPACEMOUSE_46);
MK(SpacemouseKey, GB_46, SPACEMOUSE_47);
MK(SpacemouseKey, GB_47, SPACEMOUSE_48);
MK(SpacemouseKey, GB_48, SPACEMOUSE_49);
MK(SpacemouseKey, GB_49, SPACEMOUSE_50);
MK(SpacemouseKey, GB_50, SPACEMOUSE_51);
MK(SpacemouseKey, GB_51, SPACEMOUSE_52);
MK(SpacemouseKey, GB_52, SPACEMOUSE_53);
MK(SpacemouseKey, GB_53, SPACEMOUSE_54);
MK(SpacemouseKey, GB_54, SPACEMOUSE_55);
MK(SpacemouseKey, GB_55, SPACEMOUSE_56);
MK(SpacemouseKey, GB_56, SPACEMOUSE_57);
MK(SpacemouseKey, GB_57, SPACEMOUSE_58);
MK(SpacemouseKey, GB_58, SPACEMOUSE_59);
MK(SpacemouseKey, GB_59, SPACEMOUSE_60);
MK(SpacemouseKey, GB_60, SPACEMOUSE_61);
MK(SpacemouseKey, GB_61, SPACEMOUSE_62);
MK(SpacemouseKey, GB_62, SPACEMOUSE_63);
MK(SpacemouseKey, GB_63, SPACEMOUSE_64);
MK(SpacemouseKey, GB_UP, SPACEMOUSE_61);
MK(SpacemouseKey, GB_RI, SPACEMOUSE_62);
MK(SpacemouseKey, GB_DO, SPACEMOUSE_63);
MK(SpacemouseKey, GB_LE, SPACEMOUSE_64);
#endif

class SQUIDSPACEMOUSE {
private:
    Transport*              transport;
    SpaceTranslationReport  _transReport;
    SpaceRotationReport     _rotReport;  
    SpaceButtonReport       _buttonReport;
    uint32_t                _delay_ms;
    
    #if MOUSE_ENABLE || DIGITIZER_ENABLE
      uint16_t              _relativeX;
      uint16_t              _relativeY;
      uint16_t              _screenWidth;
      uint16_t              _screenHeight;
      uint16_t              _currentAbsoluteX;
      uint16_t              _currentAbsoluteY;
    #endif
    
    #if GAMEPAD_ENABLE
    
    #endif
    
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
    void   press(SpacemouseKey button);
    void   release(SpacemouseKey button);
    bool   isPressed(SpacemouseKey button);
    void   setAllButtons(uint32_t lowButtons, uint32_t highButtons = 0);
    void   sendReport();
    void   releaseAll();
    
    #if MOUSE_ENABLE || DIGITIZER_ENABLE
    void   click(SpacemouseKey b = MO_BTN1);
    void   move(int16_t x, int16_t y, int16_t wheel = 0, int16_t hWheel = 0);
    void   moveRelative(int16_t relX, int16_t relY, bool sendImmediately = true);
    void   sendMouseReport();
    void   click(uint16_t x, uint16_t y, SpacemouseKey b = DI_BTN1); 
    void   moveTo(uint16_t x, uint16_t y, uint8_t pressure = 0, SpacemouseKey buttons = SpacemouseKey{0});
    void   beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure = 1270);
    void   updateStroke(uint16_t x, uint16_t y, uint16_t pressure);
    void   endStroke(uint16_t x, uint16_t y);
    void   setDigitizerRange(uint16_t maxX, uint16_t maxY);
    void   sendDigitizerReport();
    #endif
    
    #if GAMEPAD_ENABLE
    void    gamepadSetLeftStick(int16_t x, int16_t y);
    void    gamepadSetRightStick(int16_t x, int16_t y);
    void    gamepadSetTriggers(int16_t left, int16_t right);
    void    gamepadSetAxis(SpacemouseAnalogue axis, int16_t value);
    int16_t gamepadGetAxis(SpacemouseAnalogue axis);
    void    gamepadSetAllAxes(int16_t values[6]);
    void    sendGamepadReport();
    #endif
    
};

#endif
