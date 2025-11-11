#include "NKRO.h"
#include "NimBLEDevice.h"

static const char* NKRO_TAG = "BLENKRO";

BLENKRO::BLENKRO() 
    : inputNKRO(nullptr), _useNKRO(true) {
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
}

void BLENKRO::begin(NimBLECharacteristic* nkroChar, uint32_t delay_ms) {
    inputNKRO = nkroChar;
    _delay_ms = delay_ms;
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
}

bool BLENKRO::isConnected() {
    if (!inputNKRO) return false;
    return NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0;
}

size_t BLENKRO::press(uint8_t k) {
    if (k >= 136) { k = k - 136; }
    
    if (k != 0) {
        // Check if we're already at 6 non-modifier keys
        if (!_useNKRO && countPressedKeys() >= 6) {
            // setWriteError();  // REMOVE THIS LINE - not available in BLENKRO
            return 0;
        }
        
        // Update the bitmask - ONLY for regular keys
        updateNKROBitmask(k, true);
    }
    
    sendNKROReport();
    return 1;
}

size_t BLENKRO::press(int16_t modifier) {
  uint8_t hidModifier = 0;
  if (modifier >= 0x0100 && modifier <= 0x8000 && ((modifier & (modifier - 1)) == 0)) {
    hidModifier = modifier >> 8;
  } else {
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers |= hidModifier;
  sendNKROReport();
  return 1;
}

size_t BLENKRO::release(uint8_t k) {
  if (k >= 136) {
    k = k - 136;
  }
  
  if (k != 0) {
    updateNKROBitmask(k, false);
  }
  
  sendNKROReport();
  return 1;
}

size_t BLENKRO::release(int16_t modifier) {
  uint8_t hidModifier = 0;
  if (modifier >= 0x0100 && modifier <= 0x8000 && ((modifier & (modifier - 1)) == 0)) {
    hidModifier = modifier >> 8;
  } else {
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers &= ~hidModifier;
  sendNKROReport();
  return 1;
}

void BLENKRO::releaseAll() {
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
    sendNKROReport();
}

size_t BLENKRO::write(uint8_t c) {
    bool shift;
    uint8_t key = BLENKRO::charToKeyCode((char)c, &shift);
    if (key == 0) return 0;                     // character not supported

    if (shift) press(KEY_LEFT_SHIFT);           // hold shift
    press(key);                                 // key-down
    release(key);                               // key-up
    if (shift) release(KEY_LEFT_SHIFT);         // release shift
    return 1;
}

size_t BLENKRO::write(int16_t modifier) {
  uint16_t p = press(modifier);  // Modifier down
  release(modifier);             // Modifier up
  return p;
}

void BLENKRO::useNKRO(bool state) {
  _useNKRO = state; // state = enabled, therefore _useNKRO = true/enabled
  Serial.printf("[%s] Switched to %s mode\n", NKRO_TAG, _useNKRO ? "NKRO" : "6KRO");
}

void BLENKRO::use6KRO(bool state) {
  _useNKRO = !state; // state = enabled, therefore _useNKRO = not true/enabled = false
  Serial.printf("[%s] Switched to %s mode\n", NKRO_TAG, _useNKRO ? "NKRO" : "6KRO");
}

bool BLENKRO::isNKROEnabled() {
  return _useNKRO;
}

uint8_t BLENKRO::countPressedKeys() {
  uint8_t count = 0;
  // Only count non-modifier keys in the main key area
  // This ensures modifiers don't count toward the 6-key limit
  for (int i = 0; i < (NKRO_KEY_COUNT / 8); i++) {
    uint8_t byte = _keyReportNKRO.keys_bitmask[i];
    count += __builtin_popcount(byte);
  }
  return count;
}

void BLENKRO::updateNKROBitmask(uint8_t k, bool pressed) {
  if (k < NKRO_KEY_COUNT) {
    uint8_t bitmaskIndex = k / 8;
    uint8_t bitOffset = k % 8;
    
    if (pressed) {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] |= (1 << bitOffset);
    } else {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] &= ~(1 << bitOffset);
    }
  }
}

void BLENKRO::setModifiers(uint8_t modifiers) {
    _keyReportNKRO.modifiers = modifiers;
    sendNKROReport();
}

uint8_t BLENKRO::getModifiers() {
    return _keyReportNKRO.modifiers;
}

uint8_t BLENKRO::charToKeyCode(char c, bool *needShift) {
    *needShift = false;

    if (c >= '0' && c <= '9')            return (c - '0') + 0x27;   // 0x27…0x30
    if (c == ')')  { *needShift = true;  return 0x27; }            // shift-0
    if (c == '!')  { *needShift = true;  return 0x1e; }            // shift-1
    if (c == '@')  { *needShift = true;  return 0x1f; }            // shift-2
    if (c == '#')  { *needShift = true;  return 0x20; }            // shift-3
    if (c == '$')  { *needShift = true;  return 0x21; }            // shift-4
    if (c == '%')  { *needShift = true;  return 0x22; }            // shift-5
    if (c == '^')  { *needShift = true;  return 0x23; }            // shift-6
    if (c == '&')  { *needShift = true;  return 0x24; }            // shift-7
    if (c == '*')  { *needShift = true;  return 0x25; }            // shift-8
    if (c == '(')  { *needShift = true;  return 0x26; }            // shift-9

    if (c >= 'a' && c <= 'z')            return (c - 'a') + 0x04;   // 0x04…0x1D
    if (c >= 'A' && c <= 'Z') { *needShift = true;  return (c - 'A') + 0x04; }

    switch (c)
    {
    case '\n': case '\r':                return KEY_ENTER;          // 0xB0
    case '\t':                           return KEY_TAB;            // 0xB3
    case ' ':                            return KEY_SPACE;          // 0x2C
    case '-': case '_':
        if (c == '_') *needShift = true;
        return KEY_MINUS;                                      // 0x2D
    case '=': case '+':
        if (c == '+') *needShift = true;
        return KEY_EQUAL;                                      // 0x2E
    case '[': case '{':
        if (c == '{') *needShift = true;
        return KEY_LEFT_BRACKET;                               // 0x2F
    case ']': case '}':
        if (c == '}') *needShift = true;
        return KEY_RIGHT_BRACKET;                              // 0x30
    case '\\': case '|':
        if (c == '|') *needShift = true;
        return KEY_BACKSLASH;                                  // 0x31
    case ';': case ':':
        if (c == ':') *needShift = true;
        return KEY_SEMICOLON;                                  // 0x33
    case '\'': case '"':
        if (c == '"') *needShift = true;
        return KEY_APOSTROPHE;                                 // 0x34
    case '`': case '~':
        if (c == '~') *needShift = true;
        return KEY_GRAVE;                                      // 0x35
    case ',': case '<':
        if (c == '<') *needShift = true;
        return KEY_COMMA;                                      // 0x36
    case '.': case '>':
        if (c == '>') *needShift = true;
        return KEY_PERIOD;                                     // 0x37
    case '/': case '?':
        if (c == '?') *needShift = true;
        return KEY_FORWARD_SLASH;                              // 0x38
    default:                             return 0;                // non-printable
    }
}

void BLENKRO::sendNKROReport() {
    if (isConnected() && inputNKRO) {
        // If in NKRO mode, send the extended report (ID 2)
        if (_useNKRO) {
            inputNKRO->setValue((uint8_t*)&_keyReportNKRO, sizeof(KeyReportNKRO));
        } else {
            // 6KRO mode - extract first 6 pressed keys and send boot report
            uint8_t bootReport[8];
            bootReport[0] = _keyReportNKRO.modifiers;
            bootReport[1] = 0; // reserved
            
            // Find up to 6 pressed keys
            int keyIndex = 2;
            for (int i = 0; i < NKRO_KEY_COUNT && keyIndex < 8; i++) {
                if ((_keyReportNKRO.keys_bitmask[i/8] >> (i%8)) & 1) {
                    bootReport[keyIndex++] = i;
                }
            }
            
            // Fill remaining with 0
            while (keyIndex < 8) {
                bootReport[keyIndex++] = 0;
            }
            
            inputNKRO->setValue(bootReport, 8);
        }
        inputNKRO->notify();
        delay(_delay_ms);
    }
}
