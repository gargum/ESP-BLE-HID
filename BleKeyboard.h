// uncomment the following line to use NimBLE library
//#define USE_NIMBLE

#ifndef ESP32_BLE_KEYBOARD_H
#define ESP32_BLE_KEYBOARD_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)

#include "NimBLECharacteristic.h"
#include "NimBLEHIDDevice.h"

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

const uint8_t KEY_RETURN = 0xB0;
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

const uint8_t KC_RET  = 0xB0;
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

const uint8_t KEY_LEFT_CTRL   = 0x01;
const uint8_t KEY_LEFT_SHIFT  = 0x02;
const uint8_t KEY_LEFT_ALT    = 0x04;
const uint8_t KEY_LEFT_GUI    = 0x08;
const uint8_t KEY_RIGHT_CTRL  = 0x10;
const uint8_t KEY_RIGHT_SHIFT = 0x20;
const uint8_t KEY_RIGHT_ALT   = 0x40;
const uint8_t KEY_RIGHT_GUI   = 0x80;

const uint8_t KC_LCTL  = 0x01;
const uint8_t KC_LSFT  = 0x02;
const uint8_t KC_LALT  = 0x04;
const uint8_t KC_LOPT  = 0x04;
const uint8_t KC_LGUI  = 0x08;
const uint8_t KC_LCMD  = 0x08;
const uint8_t KC_LWIN  = 0x08;
const uint8_t KC_RCTL  = 0x10;
const uint8_t KC_RSFT  = 0x20;
const uint8_t KC_RALT  = 0x40;
const uint8_t KC_ROPT  = 0x40;
const uint8_t KC_ALGR  = 0x40;
const uint8_t KC_RGUI  = 0x80;
const uint8_t KC_RCMD  = 0x80;
const uint8_t KC_RWIN  = 0x80;

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

const uint8_t KC_INS  = 0xD1;
const uint8_t KC_PSCR = 0xCE;
const uint8_t KC_DEL  = 0xD4;
const uint8_t KC_PGUP = 0xD3;
const uint8_t KC_PGDN = 0xD6;
const uint8_t KC_HOME = 0xD2;
const uint8_t KC_END  = 0xD5;

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

typedef uint8_t MediaKeyReport[2];

const MediaKeyReport KEY_MEDIA_NEXT_TRACK = {1, 0};
const MediaKeyReport KEY_MEDIA_PREVIOUS_TRACK = {2, 0};
const MediaKeyReport KEY_MEDIA_STOP = {4, 0};
const MediaKeyReport KEY_MEDIA_PLAY_PAUSE = {8, 0};
const MediaKeyReport KEY_MEDIA_MUTE = {16, 0};
const MediaKeyReport KEY_MEDIA_VOLUME_UP = {32, 0};
const MediaKeyReport KEY_MEDIA_VOLUME_DOWN = {64, 0};
const MediaKeyReport KEY_MEDIA_WWW_HOME = {128, 0};
const MediaKeyReport KEY_MEDIA_LOCAL_MACHINE_BROWSER = {0, 1}; // Opens "My Computer" on Windows
const MediaKeyReport KEY_MEDIA_CALCULATOR = {0, 2};
const MediaKeyReport KEY_MEDIA_WWW_BOOKMARKS = {0, 4};
const MediaKeyReport KEY_MEDIA_WWW_SEARCH = {0, 8};
const MediaKeyReport KEY_MEDIA_WWW_STOP = {0, 16};
const MediaKeyReport KEY_MEDIA_WWW_BACK = {0, 32};
const MediaKeyReport KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION = {0, 64}; // Media Selection
const MediaKeyReport KEY_MEDIA_EMAIL_READER = {0, 128};

const MediaKeyReport KC_MNXT = {1, 0};
const MediaKeyReport KC_MPRV = {2, 0};
const MediaKeyReport KC_MSTP = {4, 0};
const MediaKeyReport KC_MPLY = {8, 0};
const MediaKeyReport KC_MUTE = {16, 0};
const MediaKeyReport KC_VOLU = {32, 0};
const MediaKeyReport KC_VOLD = {64, 0};
const MediaKeyReport KC_WHOM = {128, 0};
const MediaKeyReport KC_MYCM = {0, 1}; // Opens "My Computer" on Windows
const MediaKeyReport KC_CALC = {0, 2};
const MediaKeyReport KC_WFAV = {0, 4};
const MediaKeyReport KC_WSCH = {0, 8};
const MediaKeyReport KC_WSTP = {0, 16};
const MediaKeyReport KC_WBAK = {0, 32};
const MediaKeyReport KC_MSEL = {0, 64}; // Media Selection
const MediaKeyReport KC_MAIL = {0, 128};

//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

class BleKeyboard : public Print, public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
  BLEHIDDevice* hid;
  BLECharacteristic* inputKeyboard;
  BLECharacteristic* outputKeyboard;
  BLECharacteristic* inputMediaKeys;
  BLEAdvertising*    advertising;
  KeyReport          _keyReport;
  MediaKeyReport     _mediaKeyReport;
  std::string        deviceName;
  std::string        deviceManufacturer;
  uint8_t            batteryLevel;
  bool               connected = false;
  uint32_t           _delay_ms = 7;
  void delay_ms(uint64_t ms);

  uint16_t vid       = 0x05ac;
  uint16_t pid       = 0x820a;
  uint16_t version   = 0x0210;

public:
  BleKeyboard(std::string deviceName = "ESP32 Keyboard", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);
  void end(void);
  void sendReport(KeyReport* keys);
  void sendReport(MediaKeyReport* keys);
  size_t press(uint8_t k);
  size_t press(const MediaKeyReport k);
  size_t release(uint8_t k);
  size_t release(const MediaKeyReport k);
  size_t write(uint8_t c);
  size_t write(const MediaKeyReport c);
  size_t write(const uint8_t *buffer, size_t size);
  void releaseAll(void);
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);  
  void setDelay(uint32_t ms);

  void set_vendor_id(uint16_t vid);
  void set_product_id(uint16_t pid);
  void set_version(uint16_t version);
protected:
  virtual void onStarted(BLEServer *pServer) { };
  virtual void onConnect(BLEServer* pServer) override;
  virtual void onDisconnect(BLEServer* pServer) override;
  virtual void onWrite(BLECharacteristic* me) override;

};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_KEYBOARD_H
