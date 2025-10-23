// uncomment the following line to use NimBLE library
// #define USE_NIMBLE

#ifndef ESP32_BLE_KEYBOARD_H
#define ESP32_BLE_KEYBOARD_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)

#include "NimBLEDevice.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"
#include "NimBLEAdvertising.h"
#include "NimBLEServer.h"

#define BLEDevice                  NimBLEDevice
#define BLEServerCallbacks         NimBLEServerCallbacks
#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks
#define BLEHIDDevice               NimBLEHIDDevice
#define BLECharacteristic          NimBLECharacteristic
#define BLEAdvertising             NimBLEAdvertising
#define BLEServer                  NimBLEServer

#else

#include "BLEHIDDevice.h"
#include "BLECharacteristic.h"

#endif // USE_NIMBLE

#include "Print.h"

#define BLE_KEYBOARD_VERSION "0.0.4"
#define BLE_KEYBOARD_VERSION_MAJOR 0
#define BLE_KEYBOARD_VERSION_MINOR 0
#define BLE_KEYBOARD_VERSION_REVISION 4

// NKRO configuration
#define NKRO_KEY_COUNT 252 // Surprise! "N" in "N-Key Rollover" stands for "252" in my implementation.

// Pointer configuration
#define ABSOLUTE_MIN 0
#define ABSOLUTE_MAX 32767

// Gamepad configuration
#define GAMEPAD_BUTTON_COUNT 64
#define GAMEPAD_AXIS_COUNT 6

//  NKRO report structure
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys_bitmask[(NKRO_KEY_COUNT + 7) / 8];  // Bitmask for keys
} KeyReportNKRO;
// Mouse report structure
typedef struct {
  uint8_t buttons;
  int8_t x;
  int8_t y;
  int8_t wheel;
  int8_t hWheel;
} MouseReport;
// Digitizer report structure
typedef struct {
  uint8_t buttons;
  uint16_t x;
  uint16_t y;
  uint16_t pressure;         // Pressure value (0 = min, 1023 = max)
  uint8_t tipSwitch : 1;     // 1 = touching, 0 = not touching
  uint8_t padding : 7;       // Padding for remaining bits
  int8_t wheel;
  int8_t hWheel;
} AbsoluteReport;
// Gamepad report structure
typedef struct {
  uint32_t buttons[2];
  int16_t axes[GAMEPAD_AXIS_COUNT];
  uint8_t hat;
} GamepadReport;

static const bool enabled = true;
static const bool disabled = false;

// Appearance codes
//
// These make it easier to set what the ESP32 advertises itself as. Arranged in order of how much they frighten me.
#define GENERIC_HID           0x03C0
#define KEYBOARD              0x03C1
#define MOUSE                 0x03C2
#define JOYSTICK              0x03C3
#define GAMEPAD               0x03C4
#define DIGITIZER             0x03C5
#define DIGITAL_PEN           0x03C7 
#define HEADPHONES            0x0943
#define DISPLAY               0x0141
#define REMOTE_CONTROL        0x0181
#define REMOTE_PRESENTATION   0x0182
#define KEYRING               0x0242
#define DESKTOP               0x0081
#define SERVER                0x0082
#define LAPTOP                0x0083
#define TABLET                0x0087
#define PHONE                 0x0041
#define SMARTWATCH            0x00C2
#define CYCLING_COMPUTER      0x0481
#define RUNNING_WALKING       0x044C
#define WEARABLE              0x0086
#define WEARABLE_IN_SHOE      0x0441
#define WEARABLE_ON_SHOE      0x0442
#define WEARABLE_ON_HIP       0x0443
#define CLOCK                 0x0104
#define BARCODE_SCANNER       0x03C8
#define CARD_READER           0x03C6
#define OUTDOOR_SPORTS        0x1440 // Idk wtf the intended use case for this one is
#define LOCATION_DISPLAY      0x1441
#define LOCATION_POD          0x1443
#define WEIGHT_SCALE          0x0C81
#define EAR_THERMOMETER       0x0301
#define BLOOD_PRESSURE        0x0381
#define PULSE_OXIMETER        0x0C41
#define GLUCOSE_METER         0x0412
#define GLUCOSE_CONTINUOUS    0x0D01
#define MEDICATION_DELIVERY   0x0D81
#define INSULIN_PEN           0x0D48
#define INSULIN_PUMP          0x0D41
#define WHEELCHAIR            0x0CC1
#define MOBILITY_SCOOTER      0x0CC2
#define IOT_GATEWAY           0x008D


// Mouse codes
const char MOUSE_LEFT    = 1;
const char MOUSE_RIGHT   = 2;
const char MOUSE_MIDDLE  = 4;
const char MOUSE_BACK    = 8;
const char MOUSE_FORWARD = 16;
const char MOUSE_ALL     = (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE);

// Key codes
//
// Those numbers on the right are the real HID codes. It's designed that way to make it easier to understand and modify the code.
const uint8_t KEY_A = 0x04; 
const uint8_t KEY_B = 0x05;          
const uint8_t KEY_C = 0x06;          
const uint8_t KEY_D = 0x07;          
const uint8_t KEY_E = 0x08;          
const uint8_t KEY_F = 0x09;          
const uint8_t KEY_G = 0x0A;          
const uint8_t KEY_H = 0x0B;          
const uint8_t KEY_I = 0x0C;          
const uint8_t KEY_J = 0x0D;          
const uint8_t KEY_K = 0x0E;          
const uint8_t KEY_L = 0x0F;          
const uint8_t KEY_M = 0x10;          
const uint8_t KEY_N = 0x11;          
const uint8_t KEY_O = 0x12;          
const uint8_t KEY_P = 0x13;          
const uint8_t KEY_Q = 0x14;          
const uint8_t KEY_R = 0x15;          
const uint8_t KEY_S = 0x16;          
const uint8_t KEY_T = 0x17;          
const uint8_t KEY_U = 0x18;          
const uint8_t KEY_V = 0x19; 
const uint8_t KEY_W = 0x1A; 
const uint8_t KEY_X = 0x1B; 
const uint8_t KEY_Y = 0x1C;
const uint8_t KEY_Z = 0x1D;

const uint8_t KC_A = 0x04; 
const uint8_t KC_B = 0x05;          
const uint8_t KC_C = 0x06;          
const uint8_t KC_D = 0x07;          
const uint8_t KC_E = 0x08;          
const uint8_t KC_F = 0x09;          
const uint8_t KC_G = 0x0A;          
const uint8_t KC_H = 0x0B;          
const uint8_t KC_I = 0x0C;          
const uint8_t KC_J = 0x0D;          
const uint8_t KC_K = 0x0E;          
const uint8_t KC_L = 0x0F;          
const uint8_t KC_M = 0x10;          
const uint8_t KC_N = 0x11;          
const uint8_t KC_O = 0x12;          
const uint8_t KC_P = 0x13;          
const uint8_t KC_Q = 0x14;          
const uint8_t KC_R = 0x15;          
const uint8_t KC_S = 0x16;          
const uint8_t KC_T = 0x17;          
const uint8_t KC_U = 0x18;          
const uint8_t KC_V = 0x19; 
const uint8_t KC_W = 0x1A; 
const uint8_t KC_X = 0x1B; 
const uint8_t KC_Y = 0x1C;
const uint8_t KC_Z = 0x1D;

const uint8_t KEY_1 = 0x1e;          
const uint8_t KEY_2 = 0x1f;          
const uint8_t KEY_3 = 0x20;          
const uint8_t KEY_4 = 0x21;          
const uint8_t KEY_5 = 0x22;          
const uint8_t KEY_6 = 0x23;          
const uint8_t KEY_7 = 0x24;          
const uint8_t KEY_8 = 0x25;          
const uint8_t KEY_9 = 0x26;          
const uint8_t KEY_0 = 0x27;          

const uint8_t KC_1 = 0x1e;          
const uint8_t KC_2 = 0x1f;          
const uint8_t KC_3 = 0x20;          
const uint8_t KC_4 = 0x21;          
const uint8_t KC_5 = 0x22;          
const uint8_t KC_6 = 0x23;          
const uint8_t KC_7 = 0x24;          
const uint8_t KC_8 = 0x25;          
const uint8_t KC_9 = 0x26;          
const uint8_t KC_0 = 0x27;     

const uint8_t KEY_F1 = 0xC2;
const uint8_t KEY_F2 = 0xC3;
const uint8_t KEY_F3 = 0xC4;
const uint8_t KEY_F4 = 0xC5;
const uint8_t KEY_F5 = 0xC6;
const uint8_t KEY_F6 = 0xC7;
const uint8_t KEY_F7 = 0xC8;
const uint8_t KEY_F8 = 0xC9;
const uint8_t KEY_F9 = 0xCA;
const uint8_t KEY_F10 = 0xCB;
const uint8_t KEY_F11 = 0xCC;
const uint8_t KEY_F12 = 0xCD;
const uint8_t KEY_F13 = 0xF0;
const uint8_t KEY_F14 = 0xF1;
const uint8_t KEY_F15 = 0xF2;
const uint8_t KEY_F16 = 0xF3;
const uint8_t KEY_F17 = 0xF4;
const uint8_t KEY_F18 = 0xF5;
const uint8_t KEY_F19 = 0xF6;
const uint8_t KEY_F20 = 0xF7;
const uint8_t KEY_F21 = 0xF8;
const uint8_t KEY_F22 = 0xF9;
const uint8_t KEY_F23 = 0xFA;
const uint8_t KEY_F24 = 0xFB;

const uint8_t KC_F1   = 0xC2;
const uint8_t KC_F2   = 0xC3;
const uint8_t KC_F3   = 0xC4;
const uint8_t KC_F4   = 0xC5;
const uint8_t KC_F5   = 0xC6;
const uint8_t KC_F6   = 0xC7;
const uint8_t KC_F7   = 0xC8;
const uint8_t KC_F8   = 0xC9;
const uint8_t KC_F9   = 0xCA;
const uint8_t KC_F10  = 0xCB;
const uint8_t KC_F11  = 0xCC;
const uint8_t KC_F12  = 0xCD;
const uint8_t KC_F13  = 0xF0;
const uint8_t KC_F14  = 0xF1;
const uint8_t KC_F15  = 0xF2;
const uint8_t KC_F16  = 0xF3;
const uint8_t KC_F17  = 0xF4;
const uint8_t KC_F18  = 0xF5;
const uint8_t KC_F19  = 0xF6;
const uint8_t KC_F20  = 0xF7;
const uint8_t KC_F21  = 0xF8;
const uint8_t KC_F22  = 0xF9;
const uint8_t KC_F23  = 0xFA;
const uint8_t KC_F24  = 0xFB;

const uint8_t KEY_ENTER = 0xB0;
const uint8_t KEY_ESC = 0xB1;
const uint8_t KEY_BACKSPACE = 0xB2;
const uint8_t KEY_TAB = 0xB3;
const uint8_t KEY_SPACE = 0x2C;
const uint8_t KEY_MINUS = 0x2D;
const uint8_t KEY_EQUAL = 0x2E;
const uint8_t KEY_LEFT_BRACKET = 0x2F;
const uint8_t KEY_RIGHT_BRACKET = 0x30;
const uint8_t KEY_BACKSLASH = 0x31;
const uint8_t KEY_NON_US_HASH = 0x32;
const uint8_t KEY_SEMICOLON = 0x33;
const uint8_t KEY_APOSTROPHE = 0x34;
const uint8_t KEY_GRAVE = 0x35;
const uint8_t KEY_COMMA = 0x36;
const uint8_t KEY_PERIOD = 0x37;
const uint8_t KEY_FORWARD_SLASH = 0x38;
const uint8_t KEY_NON_US_BACKSLASH = 0x64;

const uint8_t KC_ENT  = 0xB0;
const uint8_t KC_ESC  = 0xB1;
const uint8_t KC_BSPC = 0xB2;
const uint8_t KC_TAB  = 0xB3;
const uint8_t KC_SPC  = 0x2C;
const uint8_t KC_MINS = 0x2D;
const uint8_t KC_EQL  = 0x2E;
const uint8_t KC_LBRC = 0x2F;
const uint8_t KC_RBRC = 0x30;
const uint8_t KC_BSLS = 0x31;
const uint8_t KC_NUHS = 0x32;
const uint8_t KC_SCLN = 0x33;
const uint8_t KC_QUOT = 0x34;
const uint8_t KC_GRV  = 0x35;
const uint8_t KC_COMM = 0x36;
const uint8_t KC_DOT  = 0x37;
const uint8_t KC_SLSH = 0x38;
const uint8_t KC_NUBS = 0x64;

const uint8_t KEY_CAPS_LOCK = 0x39;
const uint8_t KEY_SCROLL_LOCK = 0x47;
const uint8_t KEY_NUM_LOCK = 0x53;
const uint8_t KEY_LOCKING_CAPS_LOCK = 0x82;
const uint8_t KEY_LOCKING_NUM_LOCK = 0x83;
const uint8_t KEY_LOCKING_SCROLL_LOCK = 0x84;

const uint8_t KC_CAPS = 0x39;
const uint8_t KC_NUM  = 0x53;
const uint8_t KC_SCRL = 0x47;
const uint8_t KC_BRMD = 0x47;
const uint8_t KC_LCAP = 0x82;
const uint8_t KC_LNUM = 0x83;
const uint8_t KC_LSCR = 0x84;

const int16_t KEY_LEFT_CTRL   = 0x0100;
const int16_t KEY_LEFT_SHIFT  = 0x0200;
const int16_t KEY_LEFT_ALT    = 0x0400;
const int16_t KEY_LEFT_GUI    = 0x0800;
const int16_t KEY_RIGHT_CTRL  = 0x1000;
const int16_t KEY_RIGHT_SHIFT = 0x2000;
const int16_t KEY_RIGHT_ALT   = 0x4000;
const int16_t KEY_RIGHT_GUI   = 0x8000;

const int16_t KC_LCTL  = 0x0100;
const int16_t KC_LSFT  = 0x0200;
const int16_t KC_LALT  = 0x0400;
const int16_t KC_LOPT  = 0x0400;
const int16_t KC_LGUI  = 0x0800;
const int16_t KC_LCMD  = 0x0800;
const int16_t KC_LWIN  = 0x0800;
const int16_t KC_RCTL  = 0x1000;
const int16_t KC_RSFT  = 0x2000;
const int16_t KC_RALT  = 0x4000;
const int16_t KC_ROPT  = 0x4000;
const int16_t KC_ALGR  = 0x4000;
const int16_t KC_RGUI  = 0x8000;
const int16_t KC_RCMD  = 0x8000;
const int16_t KC_RWIN  = 0x8000;

const uint8_t KEY_RO = 0x87;
const uint8_t KEY_KATAKANAHIRAGANA = 0x88;
const uint8_t KEY_YEN = 0x89;
const uint8_t KEY_HENKAN = 0x8A;
const uint8_t KEY_MUHENKAN = 0x8B;
const uint8_t KEY_TOUTEN = 0x8C;
const uint8_t KEY_INTERNATIONAL_7 = 0x8D;
const uint8_t KEY_INTERNATIONAL_8 = 0x8E;
const uint8_t KEY_INTERNATIONAL_9 = 0x8F;
const uint8_t KEY_HANGEUL = 0x90;
const uint8_t KEY_HANJA = 0x91;
const uint8_t KEY_KATAKANA = 0x92;
const uint8_t KEY_HIRAGANA = 0x93;
const uint8_t KEY_ZENKAKUHANKAKU = 0x94;
const uint8_t KEY_LANG_6 = 0x95;
const uint8_t KEY_LANG_7 = 0x96;
const uint8_t KEY_LANG_8 = 0x97;
const uint8_t KEY_LANG_9 = 0x98;

const uint8_t KC_INT1 = 0x87;
const uint8_t KC_INT2 = 0x88;
const uint8_t KC_INT3 = 0x89;
const uint8_t KC_INT4 = 0x8A;
const uint8_t KC_INT5 = 0x8B;
const uint8_t KC_INT6 = 0x8C;
const uint8_t KC_INT7 = 0x8D;
const uint8_t KC_INT8 = 0x8E;
const uint8_t KC_INT9 = 0x8F;
const uint8_t KC_LNG1 = 0x90;
const uint8_t KC_LNG2 = 0x91;
const uint8_t KC_LNG3 = 0x92;
const uint8_t KC_LNG4 = 0x93;
const uint8_t KC_LNG5 = 0x94;
const uint8_t KC_LNG6 = 0x95;
const uint8_t KC_LNG7 = 0x96;
const uint8_t KC_LNG8 = 0x97;
const uint8_t KC_LNG9 = 0x98;

const uint8_t KEY_UP_ARROW = 0xDA;
const uint8_t KEY_DOWN_ARROW = 0xD9;
const uint8_t KEY_LEFT_ARROW = 0xD8;
const uint8_t KEY_RIGHT_ARROW = 0xD7;

const uint8_t KC_UP   = 0xDA;
const uint8_t KC_DOWN = 0xD9;
const uint8_t KC_LEFT = 0xD8;
const uint8_t KC_RGHT = 0xD7;

const uint8_t KEY_INSERT = 0xD1;
const uint8_t KEY_PRTSC = 0xCE;
const uint8_t KEY_DELETE = 0xD4;
const uint8_t KEY_PAGE_UP = 0xD3;
const uint8_t KEY_PAGE_DOWN = 0xD6;
const uint8_t KEY_HOME = 0xD2;
const uint8_t KEY_END = 0xD5;
const uint8_t KEY_SYSREQ = 0x46;
const uint8_t KEY_PAUSE = 0x48;
const uint8_t KEY_APPLICATION = 0x65;
const uint8_t KEY_KB_POWER = 0x66;
const uint8_t KEY_OPEN = 0x74;
const uint8_t KEY_EXEC = 0x74;
const uint8_t KEY_HELP = 0x75;
const uint8_t KEY_PROPS = 0x76;
const uint8_t KEY_FRONT = 0x77;
const uint8_t KEY_KB_STOP = 0x78;
const uint8_t KEY_AGAIN = 0x79;
const uint8_t KEY_UNDO = 0x7A;
const uint8_t KEY_CUT = 0x7B;
const uint8_t KEY_COPY = 0x7C;
const uint8_t KEY_PASTE = 0x7D;
const uint8_t KEY_FIND = 0x7E;
const uint8_t KEY_ALT_ERASE = 0x99;
const uint8_t KEY_ATTENTION = 0x9A;
const uint8_t KEY_CANCEL = 0x9B;
const uint8_t KEY_CLEAR = 0x9C;
const uint8_t KEY_PRIOR = 0x9D;
const uint8_t KEY_RETURN = 0x9E;
const uint8_t KEY_SEPARATOR = 0x9F;
const uint8_t KEY_OUT = 0xA0;
const uint8_t KEY_OPER = 0xA1;
const uint8_t KEY_CLEAR_AGAIN = 0xA2;
const uint8_t KEY_CRSEL_PROPS = 0xA3;
const uint8_t KEY_EXSEL = 0xA4;

const uint8_t KC_INS  = 0xD1;
const uint8_t KC_PSCR = 0xCE;
const uint8_t KC_DEL  = 0xD4;
const uint8_t KC_PGUP = 0xD3;
const uint8_t KC_PGDN = 0xD6;
const uint8_t KC_HOME = 0xD2;
const uint8_t KC_END  = 0xD5;
const uint8_t KC_SYRQ = 0x46;
const uint8_t KC_PAUS = 0x48;
const uint8_t KC_BRK  = 0x48;
const uint8_t KC_BRMU = 0x48;
const uint8_t KC_APP  = 0x65;
const uint8_t KC_KBPR = 0x66;
const uint8_t KC_OPEN = 0x74;
const uint8_t KC_EXEC = 0x74;
const uint8_t KC_HELP = 0x75;
const uint8_t KC_MENU = 0x76;
const uint8_t KC_SLCT = 0x77;
const uint8_t KC_STOP = 0x78;
const uint8_t KC_AGIN = 0x79;
const uint8_t KC_UNDO = 0x7A;
const uint8_t KC_CUT  = 0x7B;
const uint8_t KC_COPY = 0x7C;
const uint8_t KC_PSTE = 0x7D;
const uint8_t KC_FIND = 0x7E;
const uint8_t KC_ERAS = 0x99;
const uint8_t KC_ATTN = 0x9A;
const uint8_t KC_CNCL = 0x9B;
const uint8_t KC_CLR  = 0x9C;
const uint8_t KC_PRIR = 0x9D;
const uint8_t KC_RETN = 0x9E;
const uint8_t KC_SEPR = 0x9F;
const uint8_t KC_OUT  = 0xA0;
const uint8_t KC_OPER = 0xA1;
const uint8_t KC_CLAG = 0xA2;
const uint8_t KC_CRSL = 0xA3;
const uint8_t KC_EXSL = 0xA4;

const uint8_t KEY_NUM_SLASH = 0x54;
const uint8_t KEY_NUM_ASTERISK = 0x55;
const uint8_t KEY_NUM_MINUS = 0x56;
const uint8_t KEY_NUM_PLUS = 0x57;
const uint8_t KEY_NUM_ENTER = 0x58;
const uint8_t KEY_NUM_1 = 0x59;
const uint8_t KEY_NUM_2 = 0x5A;
const uint8_t KEY_NUM_3 = 0x5B;
const uint8_t KEY_NUM_4 = 0x5C;
const uint8_t KEY_NUM_5 = 0x5D;
const uint8_t KEY_NUM_6 = 0x5E;
const uint8_t KEY_NUM_7 = 0x5F;
const uint8_t KEY_NUM_8 = 0x60;
const uint8_t KEY_NUM_9 = 0x61;
const uint8_t KEY_NUM_0 = 0x62;
const uint8_t KEY_NUM_PERIOD = 0x63;
const uint8_t KEY_NUM_EQUAL = 0x67;
const uint8_t KEY_NUM_COMMA = 0x85;

const uint8_t KC_PSLS = 0x54;
const uint8_t KC_PAST = 0x55;
const uint8_t KC_PMNS = 0x56;
const uint8_t KC_PPLS = 0x57;
const uint8_t KC_PENT = 0x58;
const uint8_t KC_P1   = 0x59;
const uint8_t KC_P2   = 0x5A;
const uint8_t KC_P3   = 0x5B;
const uint8_t KC_P4   = 0x5C;
const uint8_t KC_P5   = 0x5D;
const uint8_t KC_P6   = 0x5E;
const uint8_t KC_P7   = 0x5F;
const uint8_t KC_P8   = 0x60;
const uint8_t KC_P9   = 0x61;
const uint8_t KC_P0   = 0x62;
const uint8_t KC_PDOT = 0x63;
const uint8_t KC_PEQL = 0x67;
const uint8_t KC_PCMM = 0x85;

// Media keys are supposed use 16 bit HID codes for some reason, so I'm just following that rule here

const uint16_t KEY_MEDIA_POWER = 0x0130;
const uint16_t KEY_MEDIA_SLEEP = 0x0134;
const uint16_t KEY_MEDIA_WAKE = 0x0135;
const uint16_t KEY_MEDIA_NEXT_TRACK = 0x00B5;
const uint16_t KEY_MEDIA_PREVIOUS_TRACK = 0x00B6;
const uint16_t KEY_MEDIA_STOP = 0x00B7;
const uint16_t KEY_MEDIA_PLAY_PAUSE = 0x00CD;
const uint16_t KEY_MEDIA_FAST_FORWARD = 0x00B3;
const uint16_t KEY_MEDIA_REWIND = 0x00B4;
const uint16_t KEY_MEDIA_EJECT = 0x00B8;
const uint16_t KEY_MEDIA_MUTE = 0x00E2;
const uint16_t KEY_MEDIA_VOLUME_UP = 0x00E9;
const uint16_t KEY_MEDIA_VOLUME_DOWN = 0x00EA;
const uint16_t KEY_MEDIA_BRIGHTNESS_UP = 0x006F;
const uint16_t KEY_MEDIA_BRIGHTNESS_DOWN = 0x0070;
const uint16_t KEY_MEDIA_MY_COMPUTER = 0x0194;
const uint16_t KEY_MEDIA_CALCULATOR = 0x0192;
const uint16_t KEY_MEDIA_MAIL = 0x018A;
const uint16_t KEY_MEDIA_MEDIA_SELECTION = 0x0183;
const uint16_t KEY_MEDIA_CONTROL_PANEL = 0x0186;
const uint16_t KEY_MEDIA_LAUNCHPAD = 0x0187;
const uint16_t KEY_MEDIA_WWW_HOME = 0x0223;
const uint16_t KEY_MEDIA_WWW_FAVORITES = 0x022A;
const uint16_t KEY_MEDIA_WWW_SEARCH = 0x0221;
const uint16_t KEY_MEDIA_WWW_STOP = 0x0226;
const uint16_t KEY_MEDIA_WWW_BACK = 0x0224;
const uint16_t KEY_MEDIA_WWW_FORWARD = 0x0225;
const uint16_t KEY_MEDIA_WWW_REFRESH = 0x0227;

const uint16_t KC_PWR  = 0x0130;
const uint16_t KC_SLEP = 0x0134;
const uint16_t KC_WAKE = 0x0135;
const uint16_t KC_MNXT = 0x00B5;
const uint16_t KC_MPRV = 0x00B6;
const uint16_t KC_MFFD = 0x00B3;
const uint16_t KC_MRWD = 0x00B4;
const uint16_t KC_MSTP = 0x00B7;
const uint16_t KC_MPLY = 0x00CD;
const uint16_t KC_MUTE = 0x00E2;
const uint16_t KC_VOLU = 0x00E9;
const uint16_t KC_VOLD = 0x00EA;
const uint16_t KC_WHOM = 0x0223;
const uint16_t KC_MYCM = 0x0194;
const uint16_t KC_CALC = 0x0192;
const uint16_t KC_WFAV = 0x022A;
const uint16_t KC_WSCH = 0x0221;
const uint16_t KC_WSTP = 0x0226;
const uint16_t KC_WREF = 0x0227;
const uint16_t KC_WBAK = 0x0224;
const uint16_t KC_WFWD = 0x0225;
const uint16_t KC_MSEL = 0x0183;
const uint16_t KC_MAIL = 0x018A;
const uint16_t KC_EJCT = 0x00B8;
const uint16_t KC_BRIU = 0x006F;
const uint16_t KC_BRID = 0x0070;
const uint16_t KC_CPNL = 0x0186;
const uint16_t KC_LPAD = 0x0187;

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

const int8_t HAT_CENTER = 0x08;
const int8_t HAT_UP = 0x00;
const int8_t HAT_UP_RIGHT = 0x01;
const int8_t HAT_RIGHT = 0x02;
const int8_t HAT_DOWN_RIGHT = 0x03;
const int8_t HAT_DOWN = 0x04;
const int8_t HAT_DOWN_LEFT = 0x05;
const int8_t HAT_LEFT = 0x06;
const int8_t HAT_UP_LEFT = 0x07;

const int8_t GB_UP = 65;
const int8_t GB_RI = 66;
const int8_t GB_DO = 67;
const int8_t GB_LE = 68;

static const int8_t hatPress[4][9] = {
  { 0x00, 0x01, 0x01, 0x01, 0x00, 0x07, 0x07, 0x07, 0x00 }, // UP
  { 0x01, 0x01, 0x02, 0x03, 0x03, 0x03, 0x02, 0x01, 0x02 }, // RIGHT
  { 0x04, 0x03, 0x03, 0x03, 0x04, 0x05, 0x05, 0x05, 0x04 }, // DOWN
  { 0x07, 0x07, 0x06, 0x05, 0x05, 0x05, 0x06, 0x07, 0x06 }  // LEFT
};
  
static const int8_t hatRelease[4][9] = {
  { 0x08, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x08}, // UP
  { 0x00, 0x00, 0x08, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08}, // RIGHT
  { 0x00, 0x01, 0x02, 0x02, 0x08, 0x06, 0x06, 0x07, 0x08}, // DOWN
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x08, 0x00, 0x08}  // LEFT
};

class BleKeyboard : public Print
#if !defined(USE_NIMBLE)
    , public BLEServerCallbacks
    , public BLECharacteristicCallbacks
    , public BLESecurityCallbacks
#else
    , public NimBLEServerCallbacks
    , public NimBLECharacteristicCallbacks
#endif
{
private:
#if defined(USE_NIMBLE)
  void               setPnpInfo();
#endif
  BLEHIDDevice*      hid;
  uint16_t           appearance = HID_KEYBOARD;
  std::string        devicePurpose = "Keyboard";
  BLECharacteristic* outputKeyboard;
  BLECharacteristic* inputMediaKeys;
  BLECharacteristic* inputNKRO;
  BLECharacteristic* inputMouse;
  BLECharacteristic* inputAbsolute;
  BLECharacteristic* inputGamepad;
  BLEAdvertising*    advertising;
  KeyReportNKRO      _keyReportNKRO;
  uint8_t            _mouseButtons;
  MouseReport        _mouseReport;
  AbsoluteReport     _absoluteReport;
  GamepadReport      _gamepadReport;
  uint32_t           _mediaKeyBitmask;
  std::string        deviceName;
  std::string        deviceManufacturer;
  std::string        securityPin;
  uint8_t            batteryLevel;
  bool               isPinSecurityEnabled() const;
  bool               connected = false;
  uint32_t           _delay_ms = 7;
  bool               _useNKRO = true;  // Default to NKRO
  bool               _useAbsolute = false;
  
  //I picked random numbers here and it worked fine, idk if these actually matter
  //I let you declare these values because QMK allows that and it was easy enough to add
  uint16_t vid       = 0x05ac;
  uint16_t pid       = 0x820a;
  uint16_t version   = 0x0210;
 
  void setStaticPasskey();
  
  void delay_ms(uint64_t ms);
  uint32_t mediaKeyToBitmask(uint16_t usageCode);
  void sendNKROReport();
  void updateNKROBitmask(uint8_t k, bool pressed);
  uint8_t countPressedKeys();
  void sendGamepadReport();

public:
  BleKeyboard(std::string deviceName = "ESP32 Keyboard", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  
  ~BleKeyboard();
  
  void begin(void);
  void end(void);
  
  void setAppearance(uint16_t newAppearance);
  void setDevicePurpose(const std::string& purpose);
  
  void setSecurityPin(const std::string& pin);
  void clearSecurityPin();
  uint32_t onPassKeyRequest();
  void onPassKeyNotify(uint32_t pass_key);
  bool onConfirmPIN(uint32_t pass_key);
#if defined(USE_NIMBLE)
  void onAuthenticationComplete(ble_gap_conn_desc* desc);
#else
  bool onSecurityRequest();
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl);
#endif
  
  void sendReport();
  // The library differentiates between keys, modifiers, and media keys by storing them using 3 different integer types
  size_t press(uint8_t k);           // I went with uint8_t for normal keycodes
  size_t press(int16_t modifier);    // I chose int16_t for modifiers
  size_t press(uint16_t mediaKey);   // I picked uint16_t for media keys
  void   press(int8_t button);       // Next, int8_t is for the gamepad buttons
  void   press(char b = MOUSE_LEFT); // Finally, char is for mouse clicks
  void   press(uint16_t x, uint16_t y, char b = MOUSE_LEFT);  // Feeding in coordinates makes it assume you're trying to use absolute pointer mode
  
  size_t release(uint8_t k);
  size_t release(int16_t modifier);
  size_t release(uint16_t mediaKey);
  void   release(int8_t button);
  void   release(char b = MOUSE_LEFT);
  void   release(uint16_t x, uint16_t y, char b = MOUSE_LEFT);
  
  size_t write(uint8_t c);
  size_t write(int16_t modifier);
  size_t write(uint16_t mediaKey);
  size_t write(const uint8_t *buffer, size_t size);
  
  void releaseAll(void);
  void gamepadReleaseAll(void);
  void mouseReleaseAll();
  
  // NKRO/6KRO mode switching
  void useNKRO(bool state = enabled);
  void use6KRO(bool state = enabled);
  bool isNKROEnabled();
  
  // BLE helper functions
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);  
  void setManufacturer(std::string deviceManufacturer);
  void setDelay(uint32_t ms);

  // Misc hardware helper functions
  void set_vendor_id(uint16_t vid);
  void set_product_id(uint16_t pid);
  void set_version(uint16_t version);
  
  // Modifier key helper functions
  void setModifiers(uint8_t modifiers);
  uint8_t getModifiers();
  
  // Media key helper functions
  void setMediaKeyBitmask(uint32_t bitmask);
  uint32_t getMediaKeyBitmask();
  void addMediaKey(uint16_t mediaKey);
  void removeMediaKey(uint16_t mediaKey);

  // Pointer helper functions (Duplicates are so the same commands can support relative or absolute depending upon whether you feed in coordinates)
  void click(char b = MOUSE_LEFT);
  void click(uint16_t x, uint16_t y, char b = MOUSE_LEFT);
  void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
  void moveTo(uint16_t x, uint16_t y, signed char wheel = 0, signed char hWheel = 0);
  bool mouseIsPressed(char b = MOUSE_LEFT);
  
  // Absolute pointer helpers
  void useAbsolute(bool enable = true);
  void useRelative(bool enable = true);
  void setAbsoluteRange(uint16_t minVal = 0, uint16_t maxVal = 32767);
  bool isAbsoluteEnabled();
  
   // Pressure-sensitive drawing helpers
  void moveToWithPressure(uint16_t x, uint16_t y, uint16_t pressure = 512, bool touching = true);
  void clickWithPressure(uint16_t x, uint16_t y, uint16_t pressure = 1023, uint8_t button = MOUSE_LEFT);
  void beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure = 1);
  void updateStroke(uint16_t x, uint16_t y, uint16_t pressure);
  void endStroke(uint16_t x, uint16_t y);
  uint16_t getPressure() const;
  bool getTipSwitch() const;
  void setPressure(uint16_t pressure);  // Pressure values are 0-1023
  void setTipSwitch(bool state);
  
  // Gamepad helpers
  bool gamepadIsPressed(int8_t button);
  void gamepadSetLeftStick(int16_t x, int16_t y);
  void gamepadSetRightStick(int16_t x, int16_t y);
  void gamepadSetTriggers(int16_t left, int16_t right);
  void gamepadGetLeftStick(int16_t &x, int16_t &y);
  void gamepadGetRightStick(int16_t &x, int16_t &y);
  void gamepadSetAxis(int8_t axis, int16_t value);
  int16_t gamepadGetAxis(int8_t axis);
  void gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]);
  
protected:
  virtual void onStarted(BLEServer *pServer) { };
  virtual void onMouseStarted(BLEServer *pServer) { };
#if !defined(USE_NIMBLE)
  virtual void onConnect(BLEServer* pServer) override;
  virtual void onDisconnect(BLEServer* pServer) override;
  virtual void onWrite(BLECharacteristic* me) override;
#else
  virtual void onConnect(BLEServer* pServer);
  virtual void onDisconnect(BLEServer* pServer);
  virtual void onWrite(BLECharacteristic* me);
#endif
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_KEYBOARD_H
