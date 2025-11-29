/**
 * @file Gamepad.h
 * @brief Gamepad with 64 buttons, POV hat, 2 sticks, 2 triggers
 */
 
#ifndef GAMEPAD_H
#define GAMEPAD_H

#if !SPACEMOUSE_ENABLE

#include <stdint.h>
#include "HIDTypes.h"
#include "../../drivers/Software/Log/Log.h"
#include "../../drivers/Software/Event/Types.h"
#include "../../drivers/Software/Transport/Transport.h"

#define GAMEPAD_BUTTON_COUNT 64
#define GAMEPAD_ANALOGUE_COUNT 6

#define GAMEPAD_ID    0x09

typedef struct {
  uint32_t buttons[2];
  int8_t hat;
  uint8_t padding;
  int16_t analogues[GAMEPAD_ANALOGUE_COUNT];
} GamepadReport;

static const uint8_t _gamepadReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x05,             
  COLLECTION(1),      0x01,                      REPORT_ID(1),       GAMEPAD_ID,       
  // 64 buttons in bitfield
  USAGE_PAGE(1),      0x09,                      USAGE_MINIMUM(1),   0x01,             
  USAGE_MAXIMUM(1),   0x40,                      LOGICAL_MINIMUM(1), 0x00,             
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,             
  REPORT_COUNT(1),    0x40,                      HIDINPUT(1),        0x02,       
  // Hat Switch
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x39,             
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x07,             
  REPORT_SIZE(1),     0x08,                      REPORT_COUNT(1),    0x01,             
  HIDINPUT(1),        0x02,         
  // Padding
  REPORT_SIZE(1),     0x08,                      REPORT_COUNT(1),    0x01,
  HIDINPUT(1),        0x03,
  // Left Stick X/Y + Right Stick X/Y
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x01,             
  COLLECTION(1),      0x00,                      USAGE(1),           0x30,             
  USAGE(1),           0x31,                      USAGE(1),           0x33,
  USAGE(1),           0x34,                      LOGICAL_MINIMUM(2), 0x01, 0x80,       
  LOGICAL_MAXIMUM(2), 0xFF, 0x7F,                REPORT_SIZE(1),     0x10,             
  REPORT_COUNT(1),    0x04,                      HIDINPUT(1),        0x02,             
  END_COLLECTION(0),                                      
  // Triggers
  USAGE(1),           0x32,                      USAGE(1),           0x35,             
  LOGICAL_MINIMUM(2), 0x00, 0x00,                LOGICAL_MAXIMUM(2), 0xFF, 0x7F,       
  REPORT_SIZE(1),     0x10,                      REPORT_COUNT(1),    0x02,             
  HIDINPUT(1),        0x02,                      END_COLLECTION(0),           
};

enum class GamepadButtons : uint8_t {

  GAMEPAD_0 = 1,
  GAMEPAD_SOUTH = 1,
  GAMEPAD_1 = 2,
  GAMEPAD_EAST = 2,
  GAMEPAD_2 = 3,
  GAMEPAD_3 = 4,
  GAMEPAD_WEST = 4,
  GAMEPAD_4 = 5,
  GAMEPAD_NORTH = 5,
  GAMEPAD_5 = 6,
  GAMEPAD_6 = 7,
  GAMEPAD_L1 = 7,
  GAMEPAD_7 = 8,
  GAMEPAD_R1 = 8,
  GAMEPAD_8 = 9,
  GAMEPAD_9 = 10,
  GAMEPAD_10 = 11,
  GAMEPAD_BACK = 11,
  GAMEPAD_11 = 12,
  GAMEPAD_START = 12,
  GAMEPAD_12 = 13,
  GAMEPAD_GUIDE = 13,
  GAMEPAD_13 = 14,
  GAMEPAD_L3 = 14,
  GAMEPAD_14 = 15,
  GAMEPAD_R3 = 15,
  GAMEPAD_15 = 16,
  GAMEPAD_16 = 17,
  GAMEPAD_17 = 18,
  GAMEPAD_18 = 19,
  GAMEPAD_19 = 20,
  GAMEPAD_20 = 21,
  GAMEPAD_21 = 22,
  GAMEPAD_22 = 23,
  GAMEPAD_23 = 24,
  GAMEPAD_24 = 25,
  GAMEPAD_25 = 26,
  GAMEPAD_26 = 27,
  GAMEPAD_27 = 28,
  GAMEPAD_28 = 29,
  GAMEPAD_29 = 30,
  GAMEPAD_30 = 31,
  GAMEPAD_31 = 32,
  GAMEPAD_32 = 33,
  GAMEPAD_33 = 34,
  GAMEPAD_34 = 35,
  GAMEPAD_35 = 36,
  GAMEPAD_36 = 37,
  GAMEPAD_37 = 38,
  GAMEPAD_38 = 39,
  GAMEPAD_39 = 40,
  GAMEPAD_40 = 41,
  GAMEPAD_41 = 42,
  GAMEPAD_42 = 43,
  GAMEPAD_43 = 44,
  GAMEPAD_44 = 45,
  GAMEPAD_45 = 46,
  GAMEPAD_46 = 47,
  GAMEPAD_47 = 48,
  GAMEPAD_48 = 49,
  GAMEPAD_49 = 50,
  GAMEPAD_50 = 51,
  GAMEPAD_51 = 52,
  GAMEPAD_52 = 53,
  GAMEPAD_53 = 54,
  GAMEPAD_54 = 55,
  GAMEPAD_55 = 56,
  GAMEPAD_56 = 57,
  GAMEPAD_57 = 58,
  GAMEPAD_58 = 59,
  GAMEPAD_59 = 60,
  GAMEPAD_60 = 61,
  GAMEPAD_61 = 62,
  GAMEPAD_62 = 63,
  GAMEPAD_63 = 64,
  GAMEPAD_64 = 65,
  GAMEPAD_UP = 65,
  GAMEPAD_RIGHT = 66,
  GAMEPAD_DOWN = 67,
  GAMEPAD_LEFT = 68

};

MK(GamepadButton, GB_00, GAMEPAD_0);
MK(GamepadButton, GB_SO, GAMEPAD_SOUTH); // "South" like the bottom face button. So 'Cross' on a Playstation controller or 'A' on an Xbox controller.
MK(GamepadButton, GB_01, GAMEPAD_1);
MK(GamepadButton, GB_EA, GAMEPAD_EAST);  // "East" like the right face button. So 'Circle' on a Playstation controller or 'B' on an Xbox controller.
MK(GamepadButton, GB_02, GAMEPAD_2);
MK(GamepadButton, GB_03, GAMEPAD_3);
MK(GamepadButton, GB_WE, GAMEPAD_WEST);  // "West" like the left face button. So 'Square' on a Playstation controller or 'X' on an Xbox controller.
MK(GamepadButton, GB_04, GAMEPAD_4);
MK(GamepadButton, GB_NO, GAMEPAD_NORTH); // "North" like the top face button. So 'Triangle' on a Playstation controller or 'Y' on an Xbox controller.
MK(GamepadButton, GB_05, GAMEPAD_5);
MK(GamepadButton, GB_06, GAMEPAD_6);
MK(GamepadButton, GB_L1, GAMEPAD_L1);
MK(GamepadButton, GB_07, GAMEPAD_7);
MK(GamepadButton, GB_R1, GAMEPAD_R1);
MK(GamepadButton, GB_08, GAMEPAD_8);
MK(GamepadButton, GB_09, GAMEPAD_9);
MK(GamepadButton, GB_10, GAMEPAD_10);
MK(GamepadButton, GB_BA, GAMEPAD_BACK);
MK(GamepadButton, GB_11, GAMEPAD_11);
MK(GamepadButton, GB_ST, GAMEPAD_START);
MK(GamepadButton, GB_12, GAMEPAD_12);
MK(GamepadButton, GB_GU, GAMEPAD_GUIDE);
MK(GamepadButton, GB_13, GAMEPAD_13);
MK(GamepadButton, GB_L3, GAMEPAD_L3);
MK(GamepadButton, GB_14, GAMEPAD_14);
MK(GamepadButton, GB_R3, GAMEPAD_R3);
MK(GamepadButton, GB_15, GAMEPAD_15);
MK(GamepadButton, GB_16, GAMEPAD_16);
MK(GamepadButton, GB_17, GAMEPAD_17);
MK(GamepadButton, GB_18, GAMEPAD_18);
MK(GamepadButton, GB_19, GAMEPAD_19);
MK(GamepadButton, GB_20, GAMEPAD_20);
MK(GamepadButton, GB_21, GAMEPAD_21);
MK(GamepadButton, GB_22, GAMEPAD_22);
MK(GamepadButton, GB_23, GAMEPAD_23);
MK(GamepadButton, GB_24, GAMEPAD_24);
MK(GamepadButton, GB_25, GAMEPAD_25);
MK(GamepadButton, GB_26, GAMEPAD_26);
MK(GamepadButton, GB_27, GAMEPAD_27);
MK(GamepadButton, GB_28, GAMEPAD_28);
MK(GamepadButton, GB_29, GAMEPAD_29);
MK(GamepadButton, GB_30, GAMEPAD_30);
MK(GamepadButton, GB_31, GAMEPAD_31);
MK(GamepadButton, GB_32, GAMEPAD_32);
MK(GamepadButton, GB_33, GAMEPAD_33);
MK(GamepadButton, GB_34, GAMEPAD_34);
MK(GamepadButton, GB_35, GAMEPAD_35);
MK(GamepadButton, GB_36, GAMEPAD_36);
MK(GamepadButton, GB_37, GAMEPAD_37);
MK(GamepadButton, GB_38, GAMEPAD_38);
MK(GamepadButton, GB_39, GAMEPAD_39);
MK(GamepadButton, GB_40, GAMEPAD_40);
MK(GamepadButton, GB_41, GAMEPAD_41);
MK(GamepadButton, GB_42, GAMEPAD_42);
MK(GamepadButton, GB_43, GAMEPAD_43);
MK(GamepadButton, GB_44, GAMEPAD_44);
MK(GamepadButton, GB_45, GAMEPAD_45);
MK(GamepadButton, GB_46, GAMEPAD_46);
MK(GamepadButton, GB_47, GAMEPAD_47);
MK(GamepadButton, GB_48, GAMEPAD_48);
MK(GamepadButton, GB_49, GAMEPAD_49);
MK(GamepadButton, GB_50, GAMEPAD_50);
MK(GamepadButton, GB_51, GAMEPAD_51);
MK(GamepadButton, GB_52, GAMEPAD_52);
MK(GamepadButton, GB_53, GAMEPAD_53);
MK(GamepadButton, GB_54, GAMEPAD_54);
MK(GamepadButton, GB_55, GAMEPAD_55);
MK(GamepadButton, GB_56, GAMEPAD_56);
MK(GamepadButton, GB_57, GAMEPAD_57);
MK(GamepadButton, GB_58, GAMEPAD_58);
MK(GamepadButton, GB_59, GAMEPAD_59);
MK(GamepadButton, GB_60, GAMEPAD_60);
MK(GamepadButton, GB_61, GAMEPAD_61);
MK(GamepadButton, GB_62, GAMEPAD_62);
MK(GamepadButton, GB_63, GAMEPAD_63);
MK(GamepadButton, GB_UP, GAMEPAD_UP);
MK(GamepadButton, GB_RI, GAMEPAD_RIGHT);
MK(GamepadButton, GB_DO, GAMEPAD_DOWN);
MK(GamepadButton, GB_LE, GAMEPAD_LEFT);

enum class GamepadAnalogues : uint8_t {

  GAMEPAD_AXIS_LX = 0,  // Left stick X
  GAMEPAD_AXIS_LY = 1,  // Left stick Y
  GAMEPAD_AXIS_RX = 2,  // Right stick X
  GAMEPAD_AXIS_RY = 3,  // Right stick Y
  GAMEPAD_AXIS_LT = 4,  // Left trigger
  GAMEPAD_AXIS_RT = 5   // Right trigger

};

MK(GamepadAnalogue, GA_LX, GAMEPAD_AXIS_LX);
MK(GamepadAnalogue, GA_LY, GAMEPAD_AXIS_LY);
MK(GamepadAnalogue, GA_RX, GAMEPAD_AXIS_RX);
MK(GamepadAnalogue, GA_RY, GAMEPAD_AXIS_RY);
MK(GamepadAnalogue, GA_LT, GAMEPAD_AXIS_LT);
MK(GamepadAnalogue, GA_RT, GAMEPAD_AXIS_RT);

enum class GamepadHats : uint8_t {

  DPAD_UP         = 0,
  DPAD_UP_RIGHT   = 1,
  DPAD_RIGHT      = 2,
  DPAD_DOWN_RIGHT = 3,
  DPAD_DOWN       = 4,
  DPAD_DOWN_LEFT  = 5,
  DPAD_LEFT       = 6,
  DPAD_UP_LEFT    = 7,
  DPAD_CENTRE     = 8

};

MK(GamepadHat, HAT_UP, DPAD_UP);
MK(GamepadHat, HAT_UR, DPAD_UP_RIGHT);
MK(GamepadHat, HAT_RI, DPAD_RIGHT);
MK(GamepadHat, HAT_DR, DPAD_DOWN_RIGHT);
MK(GamepadHat, HAT_DO, DPAD_DOWN);
MK(GamepadHat, HAT_DL, DPAD_DOWN_LEFT);
MK(GamepadHat, HAT_LE, DPAD_LEFT);
MK(GamepadHat, HAT_UL, DPAD_UP_LEFT);
MK(GamepadHat, HAT_CE, DPAD_CENTRE);

constexpr GamepadHat hatPress[4][9] = {
  { HAT_UP, HAT_UR, HAT_UR, HAT_UR, HAT_UP, HAT_UL, HAT_UL, HAT_UL, HAT_UP }, // UP     |  UP     UP-RIGHT  RIGHT  DOWN-RIGHT  DOWN  DOWN-LEFT   LEFT   UP-LEFT   CENTER
  { HAT_UR, HAT_UR, HAT_RI, HAT_DR, HAT_DR, HAT_DR, HAT_RI, HAT_UR, HAT_RI }, // RIGHT  |  0x00     0x01     0x02     0x03     0x04     0x05     0x06     0x07     0x08
  { HAT_DO, HAT_DR, HAT_DR, HAT_DR, HAT_DO, HAT_DL, HAT_DL, HAT_DL, HAT_DO }, // DOWN   |  The numbers in both these arrays are in this order. This means, for the hatPress array,
  { HAT_UL, HAT_UL, HAT_LE, HAT_DL, HAT_DL, HAT_DL, HAT_LE, HAT_UL, HAT_LE }};// LEFT   |  the very last number says "When holding CENTER, pressing LEFT results in direction 0x06 (LEFT)						
  
constexpr GamepadHat hatRelease[4][9] = {
  { HAT_CE, HAT_RI, HAT_RI, HAT_DR, HAT_DO, HAT_DL, HAT_LE, HAT_LE, HAT_CE}, // UP      |  { 0x08, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x08 } // UP
  { HAT_UP, HAT_UP, HAT_CE, HAT_DO, HAT_DO, HAT_DL, HAT_LE, HAT_UL, HAT_CE}, // RIGHT   |     ^
  { HAT_UP, HAT_UR, HAT_RI, HAT_RI, HAT_CE, HAT_LE, HAT_LE, HAT_UL, HAT_CE}, // DOWN    |     This means, "If you're pressing UP and you release UP, the result is 0x08 (CENTER)
  { HAT_UP, HAT_UR, HAT_RI, HAT_DR, HAT_DO, HAT_DO, HAT_CE, HAT_UP, HAT_CE}};// LEFT    |     First value, so "If pressing UP", first row of hatRelease so "and UP is released", code is the result.

class SQUIDGAMEPAD {
private:
    Transport*            transport; 
    GamepadReport         _gamepadReport;
    uint32_t              _delay_ms;

public:
    SQUIDGAMEPAD();
    
    void begin(Transport* transport, uint32_t delay_ms = 7);
    bool isConnected();
    void onConnect();
    void onDisconnect();
    
    // Gamepad methods
    size_t  press(GamepadButton button);
    size_t  release(GamepadButton button);
    void    releaseAll();
    bool    gamepadIsPressed(GamepadButton button);
    void    gamepadSetLeftStick(int16_t x, int16_t y);
    void    gamepadSetRightStick(int16_t x, int16_t y);
    void    gamepadSetTriggers(int16_t left, int16_t right);
    void    gamepadGetLeftStick(int16_t &x, int16_t &y);
    void    gamepadGetRightStick(int16_t &x, int16_t &y);
    void    gamepadSetAxis(GamepadAnalogue axis, int16_t value);
    int16_t gamepadGetAxis(GamepadAnalogue axis);
    void    gamepadSetAllAxes(int16_t values[GAMEPAD_ANALOGUE_COUNT]);
    void    sendGamepadReport();
    
    // Motion control methods
    void setAccelerometer(int16_t x, int16_t y, int16_t z);
    void setGyroscope(int16_t x, int16_t y, int16_t z);
    void setMotionData(int16_t accel[3], int16_t gyro[3]);
    void getAccelerometer(int16_t &x, int16_t &y, int16_t &z);
    void getGyroscope(int16_t &x, int16_t &y, int16_t &z);
};
#endif
#endif
