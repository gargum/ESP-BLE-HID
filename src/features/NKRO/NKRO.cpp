/**
 * @file NKRO.cpp
 * @brief Implementation of the NKRO keyboard
 */

#include "NKRO.h"
#include "NimBLEDevice.h"
#include "../Log/Log.h"

static const char* NKRO_TAG = "BLENKRO";

BLENKRO::BLENKRO() 
    : inputNKRO(nullptr), _useNKRO(true) {
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
}

void BLENKRO::begin(NimBLECharacteristic* nkroChar, uint32_t delay_ms) {
    inputNKRO = nkroChar;
    _delay_ms = delay_ms;
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
    BLE_LOG_DEBUG(NKRO_TAG, "NKRO subsystem initialized with delay: %lu ms", delay_ms);
}

bool BLENKRO::isConnected() {
    bool connected = (inputNKRO && NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0);
    BLE_LOG_DEBUG(NKRO_TAG, "Connection check: %s", connected ? "connected" : "disconnected");
    return connected;
}

size_t BLENKRO::press(NKROKey k) {
    // Convert to underlying type for arithmetic operations
    uint8_t keyValue = static_cast<uint8_t>(k);
    if (keyValue >= 136) { keyValue = keyValue - 136; }
    
    if (keyValue != 0) {
        // Check if we're already at 6 non-modifier keys
        if (!_useNKRO && countPressedKeys() >= 6) {
            BLE_LOG_WARN(NKRO_TAG, "6KRO limit reached, ignoring key press: 0x%02X", keyValue);
            return 0;
        }
        
        // Update the bitmask - ONLY for regular keys
        updateNKROBitmask(NKROKey{keyValue}, true);
        BLE_LOG_DEBUG(NKRO_TAG, "Key pressed: 0x%02X", keyValue);
    }
    
    sendNKROReport();
    return 1;
}

size_t BLENKRO::press(ModKey modifier) {
  // Convert to underlying type for bit operations
  uint16_t modifierValue = static_cast<uint16_t>(modifier);
  uint8_t hidModifier = 0;
  
  if (modifierValue >= 0x0100 && modifierValue <= 0x8000 && ((modifierValue & (modifierValue - 1)) == 0)) {
    hidModifier = modifierValue >> 8;
  } else {
    BLE_LOG_WARN(NKRO_TAG, "Invalid modifier pressed: 0x%04X", modifierValue);
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers |= hidModifier;
  BLE_LOG_DEBUG(NKRO_TAG, "Modifier pressed: 0x%02X", hidModifier);
  sendNKROReport();
  return 1;
}

size_t BLENKRO::release(NKROKey k) {
  // Convert to underlying type for arithmetic operations
  uint8_t keyValue = static_cast<uint8_t>(k);
  if (keyValue >= 136) {
    keyValue = keyValue - 136;
  }
  
  if (keyValue != 0) {
    updateNKROBitmask(NKROKey{keyValue}, false);
    BLE_LOG_DEBUG(NKRO_TAG, "Key released: 0x%02X", keyValue);
  }
  
  sendNKROReport();
  return 1;
}

size_t BLENKRO::release(ModKey modifier) {
  // Convert to underlying type for bit operations
  uint16_t modifierValue = static_cast<uint16_t>(modifier);
  uint8_t hidModifier = 0;
  
  if (modifierValue >= 0x0100 && modifierValue <= 0x8000 && ((modifierValue & (modifierValue - 1)) == 0)) {
    hidModifier = modifierValue >> 8;
  } else {
    BLE_LOG_WARN(NKRO_TAG, "Invalid modifier released: 0x%04X", modifierValue);
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers &= ~hidModifier;
  BLE_LOG_DEBUG(NKRO_TAG, "Modifier released: 0x%02X", hidModifier);
  sendNKROReport();
  return 1;
}

void BLENKRO::releaseAll() {
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
    BLE_LOG_DEBUG(NKRO_TAG, "All keys released");
    sendNKROReport();
}

size_t BLENKRO::write(uint8_t c) {
    bool shift;
    // Convert to char for the character processing function
    char charValue = static_cast<char>(c);
    uint8_t key = BLENKRO::charToKeyCode(charValue, &shift);
    
    if (key == 0) {
        BLE_LOG_DEBUG(NKRO_TAG, "Character not supported: 0x%02X ('%c')", 
                     static_cast<uint8_t>(c), isprint(charValue) ? charValue : '.');
        return 0; // character not supported
    }

    BLE_LOG_DEBUG(NKRO_TAG, "Writing character: 0x%02X ('%c') with%s shift", 
                 key, charValue, shift ? "" : "out");
    
    if (shift) press(KC_LSFT);           // hold shift
    press(NKROKey{key});                 // key-down
    release(NKROKey{key});               // key-up
    if (shift) release(KC_LSFT);         // release shift
    return 1;
}

size_t BLENKRO::write(ModKey modifier) {
  BLE_LOG_DEBUG(NKRO_TAG, "Writing modifier: 0x%04X", static_cast<uint16_t>(modifier));
  size_t p = press(modifier);  // Modifier down
  release(modifier);           // Modifier up
  return p;
}

void BLENKRO::useNKRO(bool state) {
  _useNKRO = state; // state = enabled, therefore _useNKRO = true/enabled
  BLE_LOG_INFO(NKRO_TAG, "Switched to %s mode", _useNKRO ? "NKRO" : "6KRO");
}

void BLENKRO::use6KRO(bool state) {
  _useNKRO = !state; // state = enabled, therefore _useNKRO = not true/enabled = false
  BLE_LOG_INFO(NKRO_TAG, "Switched to %s mode", _useNKRO ? "NKRO" : "6KRO");
}

bool BLENKRO::isNKROEnabled() {
  bool enabled = _useNKRO;
  BLE_LOG_DEBUG(NKRO_TAG, "NKRO enabled check: %s", enabled ? "true" : "false");
  return enabled;
}

uint8_t BLENKRO::countPressedKeys() {
  uint8_t count = 0;
  // Only count non-modifier keys in the main key area
  // This ensures modifiers don't count toward the 6-key limit
  for (int i = 0; i < (NKRO_KEY_COUNT / 8); i++) {
    uint8_t byte = _keyReportNKRO.keys_bitmask[i];
    count += __builtin_popcount(byte);
  }
  BLE_LOG_DEBUG(NKRO_TAG, "Pressed keys count: %u", count);
  return count;
}

void BLENKRO::updateNKROBitmask(NKROKey k, bool pressed) {
  // Convert to underlying type for bitmask operations
  uint8_t keyValue = static_cast<uint8_t>(k);
  
  if (keyValue < NKRO_KEY_COUNT) {
    uint8_t bitmaskIndex = keyValue / 8;
    uint8_t bitOffset = keyValue % 8;
    
    if (pressed) {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] |= (1 << bitOffset);
    } else {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] &= ~(1 << bitOffset);
    }
    
    BLE_LOG_DEBUG(NKRO_TAG, "Bitmask updated - Key: 0x%02X, Index: %u, Bit: %u, Action: %s", 
                  keyValue, bitmaskIndex, bitOffset, pressed ? "set" : "cleared");
  } else {
    BLE_LOG_WARN(NKRO_TAG, "Key out of range for bitmask update: 0x%02X", keyValue);
  }
}

void BLENKRO::setModifiers(ModKey modifiers) {
    // Convert to underlying type for the report
    _keyReportNKRO.modifiers = static_cast<uint8_t>(modifiers >> 8);
    BLE_LOG_DEBUG(NKRO_TAG, "Modifiers set to: 0x%02X", _keyReportNKRO.modifiers);
    sendNKROReport();
}

uint8_t BLENKRO::getModifiers() {
    uint8_t modifiers = _keyReportNKRO.modifiers;
    BLE_LOG_DEBUG(NKRO_TAG, "Modifiers retrieved: 0x%02X", modifiers);
    return modifiers;
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

void BLENKRO::sendNKROReport() {
    if (isConnected() && inputNKRO) {
        // If in NKRO mode, send the extended report (ID 2)
        if (_useNKRO) {
            inputNKRO->setValue((uint8_t*)&_keyReportNKRO, sizeof(KeyReportNKRO));
            BLE_LOG_DEBUG(NKRO_TAG, "NKRO report sent - Modifiers: 0x%02X, Keys pressed: %u", 
                         _keyReportNKRO.modifiers, countPressedKeys());
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
            BLE_LOG_DEBUG(NKRO_TAG, "6KRO report sent - Modifiers: 0x%02X, Keys: [%u, %u, %u, %u, %u, %u]", 
                         bootReport[0], bootReport[2], bootReport[3], bootReport[4], 
                         bootReport[5], bootReport[6], bootReport[7]);
        }
        
        if (inputNKRO->notify()) {
            BLE_LOG_DEBUG(NKRO_TAG, "Report notification sent successfully");
        } else {
            BLE_LOG_WARN(NKRO_TAG, "Failed to send report notification");
        }
        
        delay(_delay_ms);
    } else {
        BLE_LOG_DEBUG(NKRO_TAG, "Cannot send report - not connected or no input characteristic");
    }
}
