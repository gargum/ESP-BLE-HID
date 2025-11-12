/**
 * @file Media.h
 * @brief Media key support
 */
 
#ifndef MEDIA_H
#define MEDIA_H

#include "HIDTypes.h"
#include "NimBLECharacteristic.h"
#include <stdint.h>

#define MEDIA_KEYS_ID 0x03

static const uint8_t _mediakeyReportDescriptor[] = {
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,                      USAGE(1),           0x01,             
  COLLECTION(1),      0x01,                      REPORT_ID(1),       MEDIA_KEYS_ID,    
  USAGE_MINIMUM(1),   0x00,                      USAGE_MAXIMUM(2),   0x3C, 0x02,       
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(2), 0x3C, 0x02,       
  REPORT_SIZE(1),     0x10,                      REPORT_COUNT(1),    0x01,             
  HIDINPUT(1),        0x00,                      END_COLLECTION(0),   
};

// Media key codes

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

class BLEMEDIA {
private:
    NimBLECharacteristic* inputMediaKeys;
    uint16_t _currentMediaKey;  // Changed from uint32_t bitmask to uint16_t single key
    uint32_t _delay_ms;
    
public:
    BLEMEDIA();
    
    void begin(NimBLECharacteristic* mediaChar, uint32_t delay_ms = 7);
    bool isConnected();
    
    // Media key methods - simplified interface
    size_t press(uint16_t mediaKey);
    size_t release(uint16_t mediaKey);
    size_t write(uint16_t mediaKey);
    uint16_t getCurrentMediaKey();
    
    void sendMediaReport();
    void releaseAll();
};

#endif
