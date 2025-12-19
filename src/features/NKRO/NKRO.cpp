/**
 * @file NKRO.cpp
 * @brief Implementation of the NKRO keyboard
 */

#include "NKRO.h"

SQUIDNKRO::SQUIDNKRO() 
    : transport(nullptr), _useNKRO(true) {
    memset(&_nkroReport, 0, sizeof(_nkroReport));
}

void SQUIDNKRO::begin(Transport* trans, uint32_t delay_ms) {
    transport = trans;
    _delay_ms = delay_ms;
    memset(&_nkroReport, 0, sizeof(_nkroReport));
    SQUID_LOG_DEBUG(NKRO_TAG, "NKRO subsystem initialized with transport");
}

bool SQUIDNKRO::isConnected() {
    return transport ? transport->isConnected() : false;
}

void SQUIDNKRO::onConnect() {
    SQUID_LOG_DEBUG(NKRO_TAG, "NKRO connected");
}

void SQUIDNKRO::onDisconnect() {
    SQUID_LOG_DEBUG(NKRO_TAG, "NKRO disconnected");
}

void SQUIDNKRO::splitShiftedKey(ShiftedKey shiftedKey, uint8_t* keycode, uint8_t* modifier) {
    uint16_t value = static_cast<uint16_t>(shiftedKey);
    *keycode = value & 0xFF;
    *modifier = (value >> 8) & 0xFF;
}

size_t SQUIDNKRO::press(NKROKey k) {
    // Convert to underlying type for arithmetic operations
    uint8_t keyValue = static_cast<uint8_t>(k);
    if (keyValue >= 136) { keyValue = keyValue - 136; }
    
    if (keyValue != 0) {
        // Check if we're already at 6 non-modifier keys
        if (!_useNKRO && countPressedKeys() >= 6) {
            SQUID_LOG_WARN(NKRO_TAG, "6KRO limit reached, ignoring key press: 0x%02X", keyValue);
            return 0;
        }
        
        // Update the bitmask - ONLY for regular keys
        updateNKROBitmask(NKROKey{keyValue}, true);
        SQUID_LOG_DEBUG(NKRO_TAG, "Key pressed: 0x%02X", keyValue);
    }
    
    sendNKROReport();
    return 1;
}

size_t SQUIDNKRO::press(ModKey modifier) {
  // Convert to underlying type for bit operations
  uint16_t modifierValue = static_cast<uint16_t>(modifier);
  uint8_t hidModifier = 0;
  
  if (modifierValue >= 0x0100 && modifierValue <= 0x8000 && ((modifierValue & (modifierValue - 1)) == 0)) {
    hidModifier = modifierValue >> 8;
  } else {
    SQUID_LOG_WARN(NKRO_TAG, "Invalid modifier pressed: 0x%04X", modifierValue);
    return 0; // Invalid modifier
  }
  
  _nkroReport.modifiers |= hidModifier;
  SQUID_LOG_DEBUG(NKRO_TAG, "Modifier pressed: 0x%02X", hidModifier);
  sendNKROReport();
  return 1;
}

size_t SQUIDNKRO::press(ShiftedKey shiftedKey) {
    uint8_t keycode, modifier;
    splitShiftedKey(shiftedKey, &keycode, &modifier);
    
    // Apply the modifier first
    if (modifier != 0) {
        press(ModKey{modifier << 8});
    }
    
    // Then press the key
    return press(NKROKey{keycode});
}

size_t SQUIDNKRO::release(NKROKey k) {
  // Convert to underlying type for arithmetic operations
  uint8_t keyValue = static_cast<uint8_t>(k);
  if (keyValue >= 136) {
    keyValue = keyValue - 136;
  }
  
  if (keyValue != 0) {
    updateNKROBitmask(NKROKey{keyValue}, false);
    SQUID_LOG_DEBUG(NKRO_TAG, "Key released: 0x%02X", keyValue);
  }
  
  sendNKROReport();
  return 1;
}

size_t SQUIDNKRO::release(ModKey modifier) {
  // Convert to underlying type for bit operations
  uint16_t modifierValue = static_cast<uint16_t>(modifier);
  uint8_t hidModifier = 0;
  
  if (modifierValue >= 0x0100 && modifierValue <= 0x8000 && ((modifierValue & (modifierValue - 1)) == 0)) {
    hidModifier = modifierValue >> 8;
  } else {
    SQUID_LOG_WARN(NKRO_TAG, "Invalid modifier released: 0x%04X", modifierValue);
    return 0; // Invalid modifier
  }
  
  _nkroReport.modifiers &= ~hidModifier;
  SQUID_LOG_DEBUG(NKRO_TAG, "Modifier released: 0x%02X", hidModifier);
  sendNKROReport();
  return 1;
}

size_t SQUIDNKRO::release(ShiftedKey shiftedKey) {
    uint8_t keycode, modifier;
    splitShiftedKey(shiftedKey, &keycode, &modifier);
    
    // Release the key first
    release(NKROKey{keycode});
    
    // Then release the modifier
    if (modifier != 0) {
        release(ModKey{modifier << 8});
    }
    
    return 1;
}

void SQUIDNKRO::releaseAll() {
    memset(&_nkroReport, 0, sizeof(_nkroReport));
    SQUID_LOG_DEBUG(NKRO_TAG, "All keys released");
    sendNKROReport();
}

size_t SQUIDNKRO::write(uint8_t c) {
    bool shift;
    // Convert to char for the character processing function
    char charValue = static_cast<char>(c);
    uint8_t key = SQUIDNKRO::charToKeyCode(charValue, &shift);
    
    if (key == 0) {
        SQUID_LOG_DEBUG(NKRO_TAG, "Character not supported: 0x%02X ('%c')", 
                     static_cast<uint8_t>(c), isprint(charValue) ? charValue : '.');
        return 0; // character not supported
    }

    SQUID_LOG_DEBUG(NKRO_TAG, "Writing character: 0x%02X ('%c') with%s shift", 
                 key, charValue, shift ? "" : "out");
    
    if (shift) press(KC_LSFT);           // hold shift
    press(NKROKey{key});                 // key-down
    release(NKROKey{key});               // key-up
    if (shift) release(KC_LSFT);         // release shift
    return 1;
}

size_t SQUIDNKRO::write(ModKey modifier) {
  SQUID_LOG_DEBUG(NKRO_TAG, "Writing modifier: 0x%04X", static_cast<uint16_t>(modifier));
  size_t p = press(modifier);  // Modifier down
  release(modifier);           // Modifier up
  return p;
}

size_t SQUIDNKRO::write(ShiftedKey shiftedKey) {
    size_t result = press(shiftedKey);
    release(shiftedKey);
    return result;
}

void SQUIDNKRO::useNKRO(bool state) {
  _useNKRO = state; // state = enabled, therefore _useNKRO = true/enabled
  SQUID_LOG_INFO(NKRO_TAG, "Switched to %s mode", _useNKRO ? "NKRO" : "6KRO");
}

void SQUIDNKRO::use6KRO(bool state) {
  _useNKRO = !state; // state = enabled, therefore _useNKRO = not true/enabled = false
  SQUID_LOG_INFO(NKRO_TAG, "Switched to %s mode", _useNKRO ? "NKRO" : "6KRO");
}

bool SQUIDNKRO::isNKROEnabled() {
  bool enabled = _useNKRO;
  SQUID_LOG_DEBUG(NKRO_TAG, "NKRO enabled check: %s", enabled ? "true" : "false");
  return enabled;
}

uint8_t SQUIDNKRO::countPressedKeys() {
  uint8_t count = 0;
  // Only count non-modifier keys in the main key area
  // This ensures modifiers don't count toward the 6-key limit
  for (int i = 0; i < (NKRO_KEY_COUNT / 8); i++) {
    uint8_t byte = _nkroReport.keys_bitmask[i];
    count += __builtin_popcount(byte);
  }
  SQUID_LOG_DEBUG(NKRO_TAG, "Pressed keys count: %u", count);
  return count;
}

void SQUIDNKRO::updateNKROBitmask(NKROKey k, bool pressed) {
  // Convert to underlying type for bitmask operations
  uint8_t keyValue = static_cast<uint8_t>(k);
  
  if (keyValue < NKRO_KEY_COUNT) {
    uint8_t bitmaskIndex = keyValue / 8;
    uint8_t bitOffset = keyValue % 8;
    
    if (pressed) {
      _nkroReport.keys_bitmask[bitmaskIndex] |= (1 << bitOffset);
    } else {
      _nkroReport.keys_bitmask[bitmaskIndex] &= ~(1 << bitOffset);
    }
    
    SQUID_LOG_DEBUG(NKRO_TAG, "Bitmask updated - Key: 0x%02X, Index: %u, Bit: %u, Action: %s", 
                  keyValue, bitmaskIndex, bitOffset, pressed ? "set" : "cleared");
  } else {
    SQUID_LOG_WARN(NKRO_TAG, "Key out of range for bitmask update: 0x%02X", keyValue);
  }
}

void SQUIDNKRO::setModifiers(ModKey modifiers) {
    // Convert to underlying type for the report
    _nkroReport.modifiers = static_cast<uint8_t>(modifiers >> 8);
    SQUID_LOG_DEBUG(NKRO_TAG, "Modifiers set to: 0x%02X", _nkroReport.modifiers);
    sendNKROReport();
}

uint8_t SQUIDNKRO::getModifiers() {
    uint8_t modifiers = _nkroReport.modifiers;
    SQUID_LOG_DEBUG(NKRO_TAG, "Modifiers retrieved: 0x%02X", modifiers);
    return modifiers;
}

uint8_t SQUIDNKRO::charToKeyCode(char c, bool *needShift) {
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

    if (c >= 'a' && c <= 'z') {           return (c - 'a') + 0x04; }  // 0x04…0x1D
    if (c >= 'A' && c <= 'Z') { *needShift = true;  return (c - 'A') + 0x04; }

    switch (c)
    {
    case '\n': case '\r':                return static_cast<uint8_t>(KC_ENT);          // 0xB0
    case '\t':                           return static_cast<uint8_t>(KC_TAB);           // 0xB3
    case ' ':                            return static_cast<uint8_t>(KC_SPC);           // 0x2C
    case '-': case '_':
        if (c == '_') *needShift = true;
        return static_cast<uint8_t>(KC_MINS);                                          // 0x2D
    case '=': case '+':
        if (c == '+') *needShift = true;
        return static_cast<uint8_t>(KC_EQL);                                           // 0x2E
    case '[': case '{':
        if (c == '{') *needShift = true;
        return static_cast<uint8_t>(KC_LBRC);                                          // 0x2F
    case ']': case '}':
        if (c == '}') *needShift = true;
        return static_cast<uint8_t>(KC_RBRC);                                          // 0x30
    case '\\': case '|':
        if (c == '|') *needShift = true;
        return static_cast<uint8_t>(KC_BSLS);                                          // 0x31
    case ';': case ':':
        if (c == ':') *needShift = true;
        return static_cast<uint8_t>(KC_SCLN);                                          // 0x33
    case '\'': case '"':
        if (c == '"') *needShift = true;
        return static_cast<uint8_t>(KC_QUOT);                                          // 0x34
    case '`': case '~':
        if (c == '~') *needShift = true;
        return static_cast<uint8_t>(KC_GRV);                                           // 0x35
    case ',': case '<':
        if (c == '<') *needShift = true;
        return static_cast<uint8_t>(KC_COMM);                                          // 0x36
    case '.': case '>':
        if (c == '>') *needShift = true;
        return static_cast<uint8_t>(KC_DOT);                                           // 0x37
    case '/': case '?':
        if (c == '?') *needShift = true;
        return static_cast<uint8_t>(KC_SLSH);                                          // 0x38
    default:                             return 0;                                     // non-printable
    }
}

void SQUIDNKRO::sendNKROReport() {
    if (!isConnected() || !transport) {
        SQUID_LOG_DEBUG(NKRO_TAG, "Cannot send keyboard report - not connected or no transport");
        return;
    }
    
    if (_useNKRO) {
        bool result = transport->sendReport(NKRO_ID, (uint8_t*)&_nkroReport, sizeof(NKROReport));
        if (!result) {
          SQUID_LOG_ERROR(NKRO_TAG, "Failed to send NKRO report via transport");
        } else {
          SQUID_LOG_DEBUG(NKRO_TAG, "NKRO report sent successfully");
      }
    } else {
        // 6KRO conversion
        uint8_t bootReport[8];
        bootReport[0] = _nkroReport.modifiers;
        bootReport[1] = 0;
        
        int keyIndex = 2;
        for (int i = 0; i < NKRO_KEY_COUNT && keyIndex < 8; i++) {
            if ((_nkroReport.keys_bitmask[i/8] >> (i%8)) & 1) {
                bootReport[keyIndex++] = i;
            }
        }
        while (keyIndex < 8) {
            bootReport[keyIndex++] = 0;
        }
        
        bool result = transport->sendReport(NKRO_ID, bootReport, 8);
        if (!result) {
          SQUID_LOG_ERROR(NKRO_TAG, "Failed to send 6KRO report via transport");
        } else {
          SQUID_LOG_DEBUG(NKRO_TAG, "6KRO report sent successfully");
      }
    }
    
    delay(_delay_ms);
}
