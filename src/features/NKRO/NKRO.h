/**
 * @file NKRO.h
 * @brief Keyboard implementation with NKRO support
 */
 
#ifndef NKRO_H
#define NKRO_H

#include "HIDTypes.h"
#include "NimBLECharacteristic.h"
#include <stdint.h>
#include "../Event/Types.h"

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

enum class NKROKeys : uint8_t {

  KEY_A = 0x04,
  KEY_B = 0x05,         
  KEY_C = 0x06,         
  KEY_D = 0x07,         
  KEY_E = 0x08,         
  KEY_F = 0x09,         
  KEY_G = 0x0A,         
  KEY_H = 0x0B,         
  KEY_I = 0x0C,         
  KEY_J = 0x0D,         
  KEY_K = 0x0E,         
  KEY_L = 0x0F,         
  KEY_M = 0x10,         
  KEY_N = 0x11,         
  KEY_O = 0x12,         
  KEY_P = 0x13,         
  KEY_Q = 0x14,         
  KEY_R = 0x15,          
  KEY_S = 0x16,
  KEY_T = 0x17,
  KEY_U = 0x18,
  KEY_V = 0x19,
  KEY_W = 0x1A,
  KEY_X = 0x1B,
  KEY_Y = 0x1C,
  KEY_Z = 0x1D,
  KEY_1 = 0x1E,
  KEY_2 = 0x1F,
  KEY_3 = 0x20,
  KEY_4 = 0x21,
  KEY_5 = 0x22,
  KEY_6 = 0x23,
  KEY_7 = 0x24,
  KEY_8 = 0x25,
  KEY_9 = 0x26,
  KEY_0 = 0x27,
  KEY_F1 = 0xC2,
  KEY_F2 = 0xC3,
  KEY_F3 = 0xC4,
  KEY_F4 = 0xC5,
  KEY_F5 = 0xC6,
  KEY_F6 = 0xC7,
  KEY_F7 = 0xC8,
  KEY_F8 = 0xC9,
  KEY_F9 = 0xCA,
  KEY_F10 = 0xCB,
  KEY_F11 = 0xCC,
  KEY_F12 = 0xCD,
  KEY_F13 = 0xF0,
  KEY_F14 = 0xF1,
  KEY_F15 = 0xF2,
  KEY_F16 = 0xF3,
  KEY_F17 = 0xF4,
  KEY_F18 = 0xF5,
  KEY_F19 = 0xF6,
  KEY_F20 = 0xF7,
  KEY_F21 = 0xF8,
  KEY_F22 = 0xF9,
  KEY_F23 = 0xFA,
  KEY_F24 = 0xFB,
  KEY_ENTER = 0xB0,
  KEY_ESC = 0xB1,
  KEY_BACKSPACE = 0xB2,
  KEY_TAB = 0xB3,
  KEY_SPACE = 0x2C,
  KEY_MINUS = 0x2D,
  KEY_EQUAL = 0x2E,
  KEY_LEFT_BRACKET = 0x2F,
  KEY_RIGHT_BRACKET = 0x30,
  KEY_BACKSLASH = 0x31,
  KEY_NON_US_HASH = 0x32,
  KEY_SEMICOLON = 0x33,
  KEY_APOSTROPHE = 0x34,
  KEY_GRAVE = 0x35,
  KEY_COMMA = 0x36,
  KEY_PERIOD = 0x37,
  KEY_FORWARD_SLASH = 0x38,
  KEY_NON_US_BACKSLASH = 0x64,
  KEY_CAPS_LOCK = 0x39,
  KEY_NUM_LOCK = 0x53,
  KEY_SCROLL_LOCK = 0x47,
  KEY_LOCKING_CAPS_LOCK = 0x82,
  KEY_LOCKING_NUM_LOCK = 0x83,
  KEY_LOCKING_SCROLL_LOCK = 0x84,
  KEY_RO = 0x87,
  KEY_KATAKANAHIRAGANA = 0x88,
  KEY_YEN = 0x89,
  KEY_HENKAN = 0x8A,
  KEY_MUHENKAN = 0x8B,
  KEY_TOUTEN = 0x8C,
  KEY_INTERNATIONAL_7 = 0x8D,
  KEY_INTERNATIONAL_8 = 0x8E,
  KEY_INTERNATIONAL_9 = 0x8F,
  KEY_HANGEUL = 0x90,
  KEY_HANJA = 0x91,
  KEY_KATAKANA = 0x92,
  KEY_HIRAGANA = 0x93,
  KEY_ZENKAKUHANKAKU = 0x94,
  KEY_LANG_6 = 0x95,
  KEY_LANG_7 = 0x96,
  KEY_LANG_8 = 0x97,
  KEY_LANG_9 = 0x98,
  KEY_UP_ARROW = 0xDA,
  KEY_DOWN_ARROW = 0xD9,
  KEY_LEFT_ARROW = 0xD8,
  KEY_RIGHT_ARROW = 0xD7,
  KEY_INSERT = 0xD1,
  KEY_PRTSC = 0xCE,
  KEY_DELETE = 0xD4,
  KEY_PAGE_UP = 0xD3,
  KEY_PAGE_DOWN = 0xD6,
  KEY_HOME = 0xD2,
  KEY_END = 0xD5,
  KEY_SYSREQ = 0x46,
  KEY_PAUSE = 0x48,
  KEY_APPLICATION = 0x65,
  KEY_KB_POWER = 0x66,
  KEY_OPEN = 0x74,
  KEY_EXEC = 0x74,
  KEY_HELP = 0x75,
  KEY_PROPS = 0x76,
  KEY_FRONT = 0x77,
  KEY_KB_STOP = 0x78,
  KEY_AGAIN = 0x79,
  KEY_UNDO = 0x7A,
  KEY_CUT = 0x7B,
  KEY_COPY = 0x7C,
  KEY_PASTE = 0x7D,
  KEY_FIND = 0x7E,
  KEY_ALT_ERASE = 0x99,
  KEY_ATTENTION = 0x9A,
  KEY_CANCEL = 0x9B,
  KEY_CLEAR = 0x9C,
  KEY_PRIOR = 0x9D,
  KEY_RETURN = 0x9E,
  KEY_SEPARATOR = 0x9F,
  KEY_OUT = 0xA0,
  KEY_OPER = 0xA1,
  KEY_CLEAR_AGAIN = 0xA2,
  KEY_CRSEL_PROPS = 0xA3,
  KEY_EXSEL = 0xA4,
  KEY_NUM_SLASH = 0x54,
  KEY_NUM_ASTERISK = 0x55,
  KEY_NUM_MINUS = 0x56,
  KEY_NUM_PLUS = 0x57,
  KEY_NUM_ENTER = 0x58,
  KEY_NUM_1 = 0x59,
  KEY_NUM_2 = 0x5A,
  KEY_NUM_3 = 0x5B,
  KEY_NUM_4 = 0x5C,
  KEY_NUM_5 = 0x5D,
  KEY_NUM_6 = 0x5E,
  KEY_NUM_7 = 0x5F,
  KEY_NUM_8 = 0x60,
  KEY_NUM_9 = 0x61,
  KEY_NUM_0 = 0x62,
  KEY_NUM_PERIOD = 0x63,
  KEY_NUM_EQUAL = 0x67,
  KEY_NUM_COMMA = 0x85

};

MK(NKROKey, KC_A,    KEY_A);
MK(NKROKey, KC_B,    KEY_B);
MK(NKROKey, KC_C,    KEY_C);
MK(NKROKey, KC_D,    KEY_D);
MK(NKROKey, KC_E,    KEY_E);
MK(NKROKey, KC_F,    KEY_F);
MK(NKROKey, KC_G,    KEY_G);
MK(NKROKey, KC_H,    KEY_H);
MK(NKROKey, KC_I,    KEY_I);
MK(NKROKey, KC_J,    KEY_J);
MK(NKROKey, KC_K,    KEY_K);
MK(NKROKey, KC_L,    KEY_L);
MK(NKROKey, KC_M,    KEY_M);
MK(NKROKey, KC_N,    KEY_N);
MK(NKROKey, KC_O,    KEY_O);
MK(NKROKey, KC_P,    KEY_P);
MK(NKROKey, KC_Q,    KEY_Q);
MK(NKROKey, KC_R,    KEY_R);
MK(NKROKey, KC_S,    KEY_S);
MK(NKROKey, KC_T,    KEY_T);
MK(NKROKey, KC_U,    KEY_U);
MK(NKROKey, KC_V,    KEY_V);
MK(NKROKey, KC_W,    KEY_W);
MK(NKROKey, KC_X,    KEY_X);
MK(NKROKey, KC_Y,    KEY_Y);
MK(NKROKey, KC_Z,    KEY_Z);
MK(NKROKey, KC_1,    KEY_1);
MK(NKROKey, KC_2,    KEY_2);
MK(NKROKey, KC_3,    KEY_3);
MK(NKROKey, KC_4,    KEY_4);
MK(NKROKey, KC_5,    KEY_5);
MK(NKROKey, KC_6,    KEY_6);
MK(NKROKey, KC_7,    KEY_7);
MK(NKROKey, KC_8,    KEY_8);
MK(NKROKey, KC_9,    KEY_9);
MK(NKROKey, KC_0,    KEY_0);
MK(NKROKey, KC_F1,   KEY_F1);
MK(NKROKey, KC_F2,   KEY_F2);
MK(NKROKey, KC_F3,   KEY_F3);
MK(NKROKey, KC_F4,   KEY_F4);
MK(NKROKey, KC_F5,   KEY_F5);
MK(NKROKey, KC_F6,   KEY_F6);
MK(NKROKey, KC_F7,   KEY_F7);
MK(NKROKey, KC_F8,   KEY_F8);
MK(NKROKey, KC_F9,   KEY_F9);
MK(NKROKey, KC_F10,  KEY_F10);
MK(NKROKey, KC_F11,  KEY_F11);
MK(NKROKey, KC_F12,  KEY_F12);
MK(NKROKey, KC_F13,  KEY_F13);
MK(NKROKey, KC_F14,  KEY_F14);
MK(NKROKey, KC_F15,  KEY_F15);
MK(NKROKey, KC_F16,  KEY_F16);
MK(NKROKey, KC_F17,  KEY_F17);
MK(NKROKey, KC_F18,  KEY_F18);
MK(NKROKey, KC_F19,  KEY_F19);
MK(NKROKey, KC_F20,  KEY_F20);
MK(NKROKey, KC_F21,  KEY_F21);
MK(NKROKey, KC_F22,  KEY_F22);
MK(NKROKey, KC_F23,  KEY_F23);
MK(NKROKey, KC_F24,  KEY_F24);
MK(NKROKey, KC_ENT,  KEY_ENTER);
MK(NKROKey, KC_ESC,  KEY_ESC);
MK(NKROKey, KC_BSPC, KEY_BACKSPACE);
MK(NKROKey, KC_TAB,  KEY_TAB);
MK(NKROKey, KC_SPC,  KEY_SPACE);
MK(NKROKey, KC_MINS, KEY_MINUS);
MK(NKROKey, KC_EQL,  KEY_EQUAL);
MK(NKROKey, KC_LBRC, KEY_LEFT_BRACKET);
MK(NKROKey, KC_RBRC, KEY_RIGHT_BRACKET);
MK(NKROKey, KC_BSLS, KEY_BACKSLASH);
MK(NKROKey, KC_NUHS, KEY_NON_US_HASH);
MK(NKROKey, KC_SCLN, KEY_SEMICOLON);
MK(NKROKey, KC_QUOT, KEY_APOSTROPHE);
MK(NKROKey, KC_GRV,  KEY_GRAVE);
MK(NKROKey, KC_COMM, KEY_COMMA);
MK(NKROKey, KC_DOT,  KEY_PERIOD);
MK(NKROKey, KC_SLSH, KEY_FORWARD_SLASH);
MK(NKROKey, KC_NUBS, KEY_NON_US_BACKSLASH);
MK(NKROKey, KC_CAPS, KEY_CAPS_LOCK);
MK(NKROKey, KC_NUM,  KEY_NUM_LOCK);
MK(NKROKey, KC_SCRL, KEY_SCROLL_LOCK);
MK(NKROKey, KC_BRMD, KEY_SCROLL_LOCK);
MK(NKROKey, KC_LCAP, KEY_LOCKING_CAPS_LOCK);
MK(NKROKey, KC_LNUM, KEY_LOCKING_NUM_LOCK);
MK(NKROKey, KC_LSCR, KEY_LOCKING_SCROLL_LOCK);
MK(NKROKey, KC_INT1, KEY_RO);
MK(NKROKey, KC_INT2, KEY_KATAKANAHIRAGANA);
MK(NKROKey, KC_INT3, KEY_YEN);
MK(NKROKey, KC_INT4, KEY_HENKAN);
MK(NKROKey, KC_INT5, KEY_MUHENKAN);
MK(NKROKey, KC_INT6, KEY_TOUTEN);
MK(NKROKey, KC_INT7, KEY_INTERNATIONAL_7);
MK(NKROKey, KC_INT8, KEY_INTERNATIONAL_8);
MK(NKROKey, KC_INT9, KEY_INTERNATIONAL_9);
MK(NKROKey, KC_LNG1, KEY_HANGEUL);
MK(NKROKey, KC_LNG2, KEY_HANJA);
MK(NKROKey, KC_LNG3, KEY_KATAKANA);
MK(NKROKey, KC_LNG4, KEY_HIRAGANA);
MK(NKROKey, KC_LNG5, KEY_ZENKAKUHANKAKU);
MK(NKROKey, KC_LNG6, KEY_LANG_6);
MK(NKROKey, KC_LNG7, KEY_LANG_7);
MK(NKROKey, KC_LNG8, KEY_LANG_8);
MK(NKROKey, KC_LNG9, KEY_LANG_9);
MK(NKROKey, KC_UP,   KEY_UP_ARROW);
MK(NKROKey, KC_DOWN, KEY_DOWN_ARROW);
MK(NKROKey, KC_LEFT, KEY_LEFT_ARROW);
MK(NKROKey, KC_RGHT, KEY_RIGHT_ARROW);
MK(NKROKey, KC_INS,  KEY_INSERT);
MK(NKROKey, KC_PSCR, KEY_PRTSC);
MK(NKROKey, KC_DEL,  KEY_DELETE);
MK(NKROKey, KC_PGUP, KEY_PAGE_UP);
MK(NKROKey, KC_PGDN, KEY_PAGE_DOWN);
MK(NKROKey, KC_HOME, KEY_HOME);
MK(NKROKey, KC_END,  KEY_END);
MK(NKROKey, KC_SYRQ, KEY_SYSREQ);
MK(NKROKey, KC_PAUS, KEY_PAUSE);
MK(NKROKey, KC_BRK,  KEY_PAUSE);
MK(NKROKey, KC_BRMU, KEY_PAUSE);
MK(NKROKey, KC_APP,  KEY_APPLICATION);
MK(NKROKey, KC_KBPR, KEY_KB_POWER);
MK(NKROKey, KC_OPEN, KEY_OPEN);
MK(NKROKey, KC_EXEC, KEY_EXEC);
MK(NKROKey, KC_HELP, KEY_PAUSE);
MK(NKROKey, KC_MENU, KEY_PROPS);
MK(NKROKey, KC_SLCT, KEY_PAUSE);
MK(NKROKey, KC_STOP, KEY_KB_STOP);
MK(NKROKey, KC_AGIN, KEY_AGAIN);
MK(NKROKey, KC_UNDO, KEY_UNDO);
MK(NKROKey, KC_CUT,  KEY_CUT);
MK(NKROKey, KC_COPY, KEY_COPY);
MK(NKROKey, KC_PSTE, KEY_PASTE);
MK(NKROKey, KC_FIND, KEY_FIND);
MK(NKROKey, KC_ERAS, KEY_ALT_ERASE);
MK(NKROKey, KC_ATTN, KEY_ATTENTION);
MK(NKROKey, KC_CNCL, KEY_CANCEL);
MK(NKROKey, KC_CLR,  KEY_CLEAR);
MK(NKROKey, KC_PRIR, KEY_PAUSE);
MK(NKROKey, KC_RETN, KEY_RETURN);
MK(NKROKey, KC_SEPR, KEY_SEPARATOR);
MK(NKROKey, KC_OUT,  KEY_OUT);
MK(NKROKey, KC_OPER, KEY_OPER);
MK(NKROKey, KC_CLAG, KEY_CLEAR_AGAIN);
MK(NKROKey, KC_CRSL, KEY_CRSEL_PROPS);
MK(NKROKey, KC_EXSL, KEY_EXSEL);
MK(NKROKey, KC_PSLS, KEY_NUM_SLASH);
MK(NKROKey, KC_PAST, KEY_NUM_ASTERISK);
MK(NKROKey, KC_PMNS, KEY_NUM_MINUS);
MK(NKROKey, KC_PPLS, KEY_NUM_PLUS);
MK(NKROKey, KC_PENT, KEY_NUM_ENTER);
MK(NKROKey, KC_P1,   KEY_NUM_1);
MK(NKROKey, KC_P2,   KEY_NUM_2);
MK(NKROKey, KC_P3,   KEY_NUM_3);
MK(NKROKey, KC_P4,   KEY_NUM_4);
MK(NKROKey, KC_P5,   KEY_NUM_5);
MK(NKROKey, KC_P6,   KEY_NUM_6);
MK(NKROKey, KC_P7,   KEY_NUM_7);
MK(NKROKey, KC_P8,   KEY_NUM_8);
MK(NKROKey, KC_P9,   KEY_NUM_9);
MK(NKROKey, KC_P0,   KEY_NUM_0);
MK(NKROKey, KC_PDOT, KEY_NUM_PERIOD);
MK(NKROKey, KC_PEQL, KEY_NUM_EQUAL);
MK(NKROKey, KC_PCMM, KEY_NUM_COMMA);

enum class ModKeys : uint16_t {

  KEY_LEFT_CTRL   = 0x0100,
  KEY_LEFT_SHIFT  = 0x0200,
  KEY_LEFT_ALT    = 0x0400,
  KEY_LEFT_GUI    = 0x0800,
  KEY_RIGHT_CTRL  = 0x1000,
  KEY_RIGHT_SHIFT = 0x2000,
  KEY_RIGHT_ALT   = 0x4000,
  KEY_RIGHT_GUI   = 0x8000

};

MK(ModKey, KC_LCTL, KEY_LEFT_CTRL);
MK(ModKey, KC_LSFT, KEY_LEFT_SHIFT);
MK(ModKey, KC_LALT, KEY_LEFT_ALT);
MK(ModKey, KC_LOPT, KEY_LEFT_ALT);
MK(ModKey, KC_LGUI, KEY_LEFT_GUI);
MK(ModKey, KC_LCMD, KEY_LEFT_GUI);
MK(ModKey, KC_LWIN, KEY_LEFT_GUI);
MK(ModKey, KC_RCTL, KEY_RIGHT_CTRL);
MK(ModKey, KC_RSFT, KEY_RIGHT_SHIFT);
MK(ModKey, KC_RALT, KEY_RIGHT_ALT);
MK(ModKey, KC_ROPT, KEY_RIGHT_ALT);
MK(ModKey, KC_ALGR, KEY_RIGHT_ALT);
MK(ModKey, KC_RGUI, KEY_RIGHT_GUI);
MK(ModKey, KC_RCMD, KEY_RIGHT_GUI);
MK(ModKey, KC_RWIN, KEY_RIGHT_GUI);

class BLENKRO {
private:
  NimBLECharacteristic* inputNKRO;
  KeyReportNKRO         _keyReportNKRO;
  bool                  _useNKRO = true; // Default to NKRO
  uint32_t              _delay_ms = 7;  
    
  uint8_t               countPressedKeys();
  uint8_t               charToKeyCode(char c, bool *needShift);
  void                  updateNKROBitmask(NKROKey k, bool pressed);
public:
  BLENKRO();
    
  void    begin(NimBLECharacteristic* nkroChar, uint32_t delay_ms = 7);
  bool    isConnected();

  size_t  press(NKROKey k);           // I went with uint8_t for normal keycodes
  size_t  press(ModKey modifier);    // I chose int16_t for modifiers
  size_t  release(NKROKey k);
  size_t  release(ModKey modifier);
  void    releaseAll();
  size_t  write(uint8_t c);
  size_t  write(ModKey modifier);
  void    useNKRO(bool state = enabled);
  void    use6KRO(bool state = enabled);
  bool    isNKROEnabled();
  void    setModifiers(ModKey modifiers);
  uint8_t getModifiers();
  void    sendNKROReport();
    
};

#endif
