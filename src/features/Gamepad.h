//  -----------------------------
// | Gamepad Feature - Constants |
//  -----------------------------

#include "HIDTypes.h"

#define GAMEPAD_BUTTON_COUNT 64
#define GAMEPAD_AXIS_COUNT 6

#define GAMEPAD_ID    0x07

typedef struct {
  uint32_t buttons[2];
  int16_t axes[GAMEPAD_AXIS_COUNT];
  int8_t hat;
} GamepadReport;

static const uint8_t _gamepadReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x05,             
  COLLECTION(1),      0x01,                      REPORT_ID(1),       GAMEPAD_ID,       
  // 64 buttons in bitfield
  USAGE_PAGE(1),      0x09,                      USAGE_MINIMUM(1),   0x01,             
  USAGE_MAXIMUM(1),   0x40,                      LOGICAL_MINIMUM(1), 0x00,             
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,             
  REPORT_COUNT(1),    0x40,                      HIDINPUT(1),        0x02,             
  // Left Stick X/Y
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x01,             
  COLLECTION(1),      0x00,                      USAGE(1),           0x30,             
  USAGE(1),           0x31,                      LOGICAL_MINIMUM(2), 0x01, 0x80,       
  LOGICAL_MAXIMUM(2), 0xFF, 0x7F,                REPORT_SIZE(1),     0x10,             
  REPORT_COUNT(1),    0x02,                      HIDINPUT(1),        0x02,             
  END_COLLECTION(0),                    
  // Right Stick X/Y
  USAGE(1),           0x01,                      COLLECTION(1),      0x00,             
  USAGE(1),           0x33,                      USAGE(1),           0x34,             
  LOGICAL_MINIMUM(2), 0x01, 0x80,                LOGICAL_MAXIMUM(2), 0xFF, 0x7F,       
  REPORT_SIZE(1),     0x10,                      REPORT_COUNT(1),    0x02,             
  HIDINPUT(1),        0x02,                      END_COLLECTION(0),                    
  // Triggers
  USAGE(1),           0x32,                      USAGE(1),           0x35,             
  LOGICAL_MINIMUM(2), 0x00, 0x00,                LOGICAL_MAXIMUM(2), 0xFF, 0x7F,       
  REPORT_SIZE(1),     0x10,                      REPORT_COUNT(1),    0x02,             
  HIDINPUT(1),        0x02,             
  // Hat Switch
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x39,             
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x07,             
  REPORT_SIZE(1),     0x08,                      REPORT_COUNT(1),    0x01,             
  HIDINPUT(1),        0x02,         
  END_COLLECTION(0),                     
};

// Gamepad codes

const int8_t GAMEPAD_0 = 1;
const int8_t GAMEPAD_SO = 1;
const int8_t GAMEPAD_1 = 2;
const int8_t GAMEPAD_EA = 2;
const int8_t GAMEPAD_2 = 3;
const int8_t GAMEPAD_3 = 4;
const int8_t GAMEPAD_WE = 4;
const int8_t GAMEPAD_4 = 5;
const int8_t GAMEPAD_NO = 5;
const int8_t GAMEPAD_5 = 6;
const int8_t GAMEPAD_6 = 7;
const int8_t GAMEPAD_L1 = 7;
const int8_t GAMEPAD_7 = 8;
const int8_t GAMEPAD_R1 = 8;
const int8_t GAMEPAD_8 = 9;
const int8_t GAMEPAD_9 = 10;
const int8_t GAMEPAD_10 = 11;
const int8_t GAMEPAD_BA = 11;
const int8_t GAMEPAD_11 = 12;
const int8_t GAMEPAD_ST = 12;
const int8_t GAMEPAD_12 = 13;
const int8_t GAMEPAD_GU = 13;
const int8_t GAMEPAD_13 = 14;
const int8_t GAMEPAD_L3 = 14;
const int8_t GAMEPAD_14 = 15;
const int8_t GAMEPAD_R3 = 15;
const int8_t GAMEPAD_15 = 16;
const int8_t GAMEPAD_16 = 17;
const int8_t GAMEPAD_17 = 18;
const int8_t GAMEPAD_18 = 19;
const int8_t GAMEPAD_19 = 20;
const int8_t GAMEPAD_20 = 21;
const int8_t GAMEPAD_21 = 22;
const int8_t GAMEPAD_22 = 23;
const int8_t GAMEPAD_23 = 24;
const int8_t GAMEPAD_24 = 25;
const int8_t GAMEPAD_25 = 26;
const int8_t GAMEPAD_26 = 27;
const int8_t GAMEPAD_27 = 28;
const int8_t GAMEPAD_28 = 29;
const int8_t GAMEPAD_29 = 30;
const int8_t GAMEPAD_30 = 31;
const int8_t GAMEPAD_31 = 32;
const int8_t GAMEPAD_32 = 33;
const int8_t GAMEPAD_33 = 34;
const int8_t GAMEPAD_34 = 35;
const int8_t GAMEPAD_35 = 36;
const int8_t GAMEPAD_36 = 37;
const int8_t GAMEPAD_37 = 38;
const int8_t GAMEPAD_38 = 39;
const int8_t GAMEPAD_39 = 40;
const int8_t GAMEPAD_40 = 41;
const int8_t GAMEPAD_41 = 42;
const int8_t GAMEPAD_42 = 43;
const int8_t GAMEPAD_43 = 44;
const int8_t GAMEPAD_44 = 45;
const int8_t GAMEPAD_45 = 46;
const int8_t GAMEPAD_46 = 47;
const int8_t GAMEPAD_47 = 48;
const int8_t GAMEPAD_48 = 49;
const int8_t GAMEPAD_49 = 50;
const int8_t GAMEPAD_50 = 51;
const int8_t GAMEPAD_51 = 52;
const int8_t GAMEPAD_52 = 53;
const int8_t GAMEPAD_53 = 54;
const int8_t GAMEPAD_54 = 55;
const int8_t GAMEPAD_55 = 56;
const int8_t GAMEPAD_56 = 57;
const int8_t GAMEPAD_57 = 58;
const int8_t GAMEPAD_58 = 59;
const int8_t GAMEPAD_59 = 60;
const int8_t GAMEPAD_60 = 61;
const int8_t GAMEPAD_61 = 62;
const int8_t GAMEPAD_62 = 63;
const int8_t GAMEPAD_63 = 64;
const int8_t GAMEPAD_64 = 65;
const int8_t GAMEPAD_UP = 65;
const int8_t GAMEPAD_65 = 66;
const int8_t GAMEPAD_RI = 66;
const int8_t GAMEPAD_66 = 67;
const int8_t GAMEPAD_DO = 67;
const int8_t GAMEPAD_67 = 68;
const int8_t GAMEPAD_LE = 68;

const int8_t GB_00 = 1;
const int8_t GB_SO = 1;
const int8_t GB_01 = 2;
const int8_t GB_EA = 2;
const int8_t GB_02 = 3;
const int8_t GB_03 = 4;
const int8_t GB_WE = 4;
const int8_t GB_04 = 5;
const int8_t GB_NO = 5;
const int8_t GB_05 = 6;
const int8_t GB_06 = 7;
const int8_t GB_L1 = 7;
const int8_t GB_07 = 8;
const int8_t GB_R1 = 8;
const int8_t GB_08 = 9;
const int8_t GB_09 = 10;
const int8_t GB_10 = 11;
const int8_t GB_BA = 11;
const int8_t GB_11 = 12;
const int8_t GB_ST = 12;
const int8_t GB_12 = 13;
const int8_t GB_GU = 13;
const int8_t GB_13 = 14;
const int8_t GB_L3 = 14;
const int8_t GB_14 = 15;
const int8_t GB_R3 = 15;
const int8_t GB_15 = 16;
const int8_t GB_16 = 17;
const int8_t GB_17 = 18;
const int8_t GB_18 = 19;
const int8_t GB_19 = 20;
const int8_t GB_20 = 21;
const int8_t GB_21 = 22;
const int8_t GB_22 = 23;
const int8_t GB_23 = 24;
const int8_t GB_24 = 25;
const int8_t GB_25 = 26;
const int8_t GB_26 = 27;
const int8_t GB_27 = 28;
const int8_t GB_28 = 29;
const int8_t GB_29 = 30;
const int8_t GB_30 = 31;
const int8_t GB_31 = 32;
const int8_t GB_32 = 33;
const int8_t GB_33 = 34;
const int8_t GB_34 = 35;
const int8_t GB_35 = 36;
const int8_t GB_36 = 37;
const int8_t GB_37 = 38;
const int8_t GB_38 = 39;
const int8_t GB_39 = 40;
const int8_t GB_40 = 41;
const int8_t GB_41 = 42;
const int8_t GB_42 = 43;
const int8_t GB_43 = 44;
const int8_t GB_44 = 45;
const int8_t GB_45 = 46;
const int8_t GB_46 = 47;
const int8_t GB_47 = 48;
const int8_t GB_48 = 49;
const int8_t GB_49 = 50;
const int8_t GB_50 = 51;
const int8_t GB_51 = 52;
const int8_t GB_52 = 53;
const int8_t GB_53 = 54;
const int8_t GB_54 = 55;
const int8_t GB_55 = 56;
const int8_t GB_56 = 57;
const int8_t GB_57 = 58;
const int8_t GB_58 = 59;
const int8_t GB_59 = 60;
const int8_t GB_60 = 61;
const int8_t GB_61 = 62;
const int8_t GB_62 = 63;
const int8_t GB_63 = 64;

const int8_t AXIS_LX = 0;  // Left stick X
const int8_t AXIS_LY = 1;  // Left stick Y
const int8_t AXIS_RX = 2;  // Right stick X
const int8_t AXIS_RY = 3;  // Right stick Y
const int8_t AXIS_LT = 4;  // Left trigger
const int8_t AXIS_RT = 5;  // Right trigger

const int8_t GA_LX = 0;
const int8_t GA_LY = 1;
const int8_t GA_RX = 2;
const int8_t GA_RY = 3;
const int8_t GA_LT = 4;
const int8_t GA_RT = 5;

const int8_t HAT_UP = 0;
const int8_t HAT_UR = 1;
const int8_t HAT_RI = 2;
const int8_t HAT_DR = 3;
const int8_t HAT_DO = 4;
const int8_t HAT_DL = 5;
const int8_t HAT_LE = 6;
const int8_t HAT_UL = 7;
const int8_t HAT_CE = 8;

const int8_t GB_UP = 65;
const int8_t GB_RI = 66;
const int8_t GB_DO = 67;
const int8_t GB_LE = 68;

const int8_t hatPress[4][9] = {
  { HAT_UP, HAT_UR, HAT_UR, HAT_UR, HAT_UP, HAT_UL, HAT_UL, HAT_UL, HAT_UP }, // UP     |  UP     UP-RIGHT  RIGHT  DOWN-RIGHT  DOWN  DOWN-LEFT   LEFT   UP-LEFT   CENTER
  { HAT_UR, HAT_UR, HAT_RI, HAT_DR, HAT_DR, HAT_DR, HAT_RI, HAT_UR, HAT_RI }, // RIGHT  |  0x00     0x01     0x02     0x03     0x04     0x05     0x06     0x07     0x08
  { HAT_DO, HAT_DR, HAT_DR, HAT_DR, HAT_DO, HAT_DL, HAT_DL, HAT_DL, HAT_DO }, // DOWN   |  The numbers in both these arrays are in this order. This means, for the hatPress array,
  { HAT_UL, HAT_UL, HAT_LE, HAT_DL, HAT_DL, HAT_DL, HAT_LE, HAT_UL, HAT_LE }};// LEFT   |  the very last number says "When holding CENTER, pressing LEFT results in direction 0x06 (LEFT)						
  
const int8_t hatRelease[4][9] = {
  { HAT_CE, HAT_RI, HAT_RI, HAT_DR, HAT_DO, HAT_DL, HAT_LE, HAT_LE, HAT_CE}, // UP      |  { 0x08, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x08 } // UP
  { HAT_UP, HAT_UP, HAT_CE, HAT_DO, HAT_DO, HAT_DL, HAT_LE, HAT_UL, HAT_CE}, // RIGHT   |     ^
  { HAT_UP, HAT_UR, HAT_RI, HAT_RI, HAT_CE, HAT_LE, HAT_LE, HAT_UL, HAT_CE}, // DOWN    |     This means, "If you're pressing UP and you release UP, the result is 0x08 (CENTER)
  { HAT_UP, HAT_UR, HAT_RI, HAT_DR, HAT_DO, HAT_DO, HAT_CE, HAT_UP, HAT_CE}};// LEFT    |     First value, so "If pressing UP", first row of hatRelease so "and UP is released", code is the result.
