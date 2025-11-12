/**
 * @file NKRO.h
 * @brief Keyboard implementation with NKRO support
 */
 
#ifndef NKRO_H
#define NKRO_H

#include "HIDTypes.h"
#include "NimBLECharacteristic.h"
#include <stdint.h>

#define NKRO_KEY_COUNT 252 // Surprise! "N" in "N-Key Rollover" stands for "252" in my implementation.

#define NKRO_ID       0x02

static const bool enabled = true;
static const bool disabled = false;

typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys_bitmask[(NKRO_KEY_COUNT + 7) / 8];
} KeyReportNKRO;

static const uint8_t _nkroReportDescriptor[] = {
  // NKRO Extended Report (6KRO is emulated)
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x06,
  COLLECTION(1),      0x01,                      REPORT_ID(1),       NKRO_ID,                
  // Modifiers (8 bits)
  USAGE_PAGE(1),      0x07,                      USAGE_MINIMUM(1),   0xE0,
  USAGE_MAXIMUM(1),   0xE7,                      LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,
  REPORT_COUNT(1),    0x08,                      HIDINPUT(1),        0x02,
  // Reserved byte
  REPORT_COUNT(1),    0x01,                      REPORT_SIZE(1),     0x08,
  HIDINPUT(1),        0x01,
  // 252-key bitmap
  USAGE_PAGE(1),      0x07,                      USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(2),   0xFC, 0x00,                LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,
  REPORT_COUNT(2),    0xFC, 0x00,                HIDINPUT(1),        0x02,
  END_COLLECTION(0),
};

// Key codes

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

class BLENKRO {
private:
  NimBLECharacteristic* inputNKRO;
  KeyReportNKRO         _keyReportNKRO;
  bool                  _useNKRO = true; // Default to NKRO
  uint32_t              _delay_ms = 7;  
    
  uint8_t               countPressedKeys();
  uint8_t               charToKeyCode(char c, bool *needShift);
  void                  updateNKROBitmask(uint8_t k, bool pressed);
public:
  BLENKRO();
    
  void    begin(NimBLECharacteristic* nkroChar, uint32_t delay_ms = 7);
  bool    isConnected();

  size_t  press(uint8_t k);           // I went with uint8_t for normal keycodes
  size_t  press(int16_t modifier);    // I chose int16_t for modifiers
  size_t  release(uint8_t k);
  size_t  release(int16_t modifier);
  void    releaseAll();
  size_t  write(uint8_t c);
  size_t  write(int16_t modifier);
  void    useNKRO(bool state = enabled);
  void    use6KRO(bool state = enabled);
  bool    isNKROEnabled();
  void    setModifiers(uint8_t modifiers);
  uint8_t getModifiers();
  void    sendNKROReport();
    
};

#endif
