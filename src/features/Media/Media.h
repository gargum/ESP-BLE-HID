/**
 * @file Media.h
 * @brief Media key support
 */
 
#ifndef MEDIA_H
#define MEDIA_H

#include "drivers/Software/HID/SquidHIDTypes.h"
#include "drivers/Software/Log/Log.h"
#include "drivers/Software/Event/Types.h"
#include "drivers/Software/Transport/Transport.h"

#define MEDIA_KEYS_ID 0x03

typedef struct {
    uint16_t usage;
} MediaReport;

static const uint8_t _mediakeyReportDescriptor[] = {
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,                      USAGE(1),           0x01,             
  COLLECTION(1),      0x01,                      REPORT_ID(1),       MEDIA_KEYS_ID,    
  USAGE_MINIMUM(1),   0x00,                      USAGE_MAXIMUM(2),   0x3C, 0x02,       
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(2), 0x3C, 0x02,       
  REPORT_SIZE(1),     0x10,                      REPORT_COUNT(1),    0x01,             
  HIDINPUT(1),        0x00,                      END_COLLECTION(0),   
};

enum class MediaKeys : uint16_t {

  KEY_MEDIA_POWER = 0x0130,
  KEY_MEDIA_SLEEP = 0x0134,
  KEY_MEDIA_WAKE = 0x0135,
  KEY_MEDIA_NEXT_TRACK = 0x00B5,
  KEY_MEDIA_PREVIOUS_TRACK = 0x00B6,
  KEY_MEDIA_STOP = 0x00B7,
  KEY_MEDIA_PLAY_PAUSE = 0x00CD,
  KEY_MEDIA_FAST_FORWARD = 0x00B3,
  KEY_MEDIA_REWIND = 0x00B4,
  KEY_MEDIA_EJECT = 0x00B8,
  KEY_MEDIA_MUTE = 0x00E2,
  KEY_MEDIA_VOLUME_UP = 0x00E9,
  KEY_MEDIA_VOLUME_DOWN = 0x00EA,
  KEY_MEDIA_BRIGHTNESS_UP = 0x006F,
  KEY_MEDIA_BRIGHTNESS_DOWN = 0x0070,
  KEY_MEDIA_MY_COMPUTER = 0x0194,
  KEY_MEDIA_CALCULATOR = 0x0192,
  KEY_MEDIA_MAIL = 0x018A,
  KEY_MEDIA_MEDIA_SELECTION = 0x0183,
  KEY_MEDIA_CONTROL_PANEL = 0x0186,
  KEY_MEDIA_LAUNCHPAD = 0x0187,
  KEY_MEDIA_WWW_HOME = 0x0223,
  KEY_MEDIA_WWW_FAVOURITES = 0x022A,
  KEY_MEDIA_WWW_SEARCH = 0x0221,
  KEY_MEDIA_WWW_STOP = 0x0226,
  KEY_MEDIA_WWW_BACK = 0x0224,
  KEY_MEDIA_WWW_FORWARD = 0x0225,
  KEY_MEDIA_WWW_REFRESH = 0x0227

};

MK(MediaKey, KC_PWR,  KEY_MEDIA_POWER);
MK(MediaKey, KC_SLEP, KEY_MEDIA_SLEEP);
MK(MediaKey, KC_WAKE, KEY_MEDIA_WAKE);
MK(MediaKey, KC_MNXT, KEY_MEDIA_NEXT_TRACK);
MK(MediaKey, KC_MPRV, KEY_MEDIA_PREVIOUS_TRACK);
MK(MediaKey, KC_MSTP, KEY_MEDIA_STOP);
MK(MediaKey, KC_MPLY, KEY_MEDIA_PLAY_PAUSE);
MK(MediaKey, KC_MFFD, KEY_MEDIA_FAST_FORWARD);
MK(MediaKey, KC_MRWD, KEY_MEDIA_REWIND);
MK(MediaKey, KC_EJCT, KEY_MEDIA_EJECT);
MK(MediaKey, KC_MUTE, KEY_MEDIA_MUTE);
MK(MediaKey, KC_VOLU, KEY_MEDIA_VOLUME_UP);
MK(MediaKey, KC_VOLD, KEY_MEDIA_VOLUME_DOWN);
MK(MediaKey, KC_BRIU, KEY_MEDIA_BRIGHTNESS_UP);
MK(MediaKey, KC_BRID, KEY_MEDIA_BRIGHTNESS_DOWN);
MK(MediaKey, KC_MYCM, KEY_MEDIA_MY_COMPUTER);
MK(MediaKey, KC_CALC, KEY_MEDIA_CALCULATOR);
MK(MediaKey, KC_MAIL, KEY_MEDIA_MAIL);
MK(MediaKey, KC_MSEL, KEY_MEDIA_MEDIA_SELECTION);
MK(MediaKey, KC_CPNL, KEY_MEDIA_CONTROL_PANEL);
MK(MediaKey, KC_LPAD, KEY_MEDIA_LAUNCHPAD);
MK(MediaKey, KC_WHOM, KEY_MEDIA_WWW_HOME);
MK(MediaKey, KC_WFAV, KEY_MEDIA_WWW_FAVOURITES);
MK(MediaKey, KC_WSCH, KEY_MEDIA_WWW_SEARCH);
MK(MediaKey, KC_WSTP, KEY_MEDIA_WWW_STOP);
MK(MediaKey, KC_WBAK, KEY_MEDIA_WWW_BACK);
MK(MediaKey, KC_WFWD, KEY_MEDIA_WWW_FORWARD);
MK(MediaKey, KC_WREF, KEY_MEDIA_WWW_REFRESH);

class SQUIDMEDIA {
private:
    Transport*            transport; 
    MediaReport           _mediaReport;
    MediaKey              _currentMediaKey;
    uint32_t              _delay_ms;
    
public:
    SQUIDMEDIA();
    
    void begin(Transport* transport, uint32_t delay_ms = 7);
    bool isConnected();
    void onConnect();
    void onDisconnect();
    
    // Media key methods
    size_t   press(MediaKey mediaKey);
    size_t   release(MediaKey mediaKey);
    size_t   write(MediaKey mediaKey);
    MediaKey getCurrentMediaKey();
    
    void sendMediaReport();
    void releaseAll();
};

#endif
