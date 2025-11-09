#ifndef BLE_KEYBOARD_H_ 
#define BLE_KEYBOARD_H_
#if defined(CONFIG_BT_ENABLED)
#elif defined(ARDUINO_ARCH_ESP32)
  #define CONFIG_BT_ENABLED 
  #define NIMBLE_CFG_TRANSPORT_HCI_UART 0
  #define NIMBLE_CFG_TRANSPORT_HCI_IPC 0
#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)
  #define CONFIG_BT_ENABLED
  #define NIMBLE_CFG_TRANSPORT_HCI_UART 0
  #define NIMBLE_CFG_TRANSPORT_HCI_IPC 0
#endif
#if defined(CONFIG_BT_ENABLED)

#include "NimBLEDevice.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"
#include "NimBLEAdvertising.h"
#include "NimBLEServer.h"
#include "NimBLEUUID.h"

#define BLEDevice                  NimBLEDevice
#define BLEServerCallbacks         NimBLEServerCallbacks
#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks
#define BLEHIDDevice               NimBLEHIDDevice
#define BLECharacteristic          NimBLECharacteristic
#define BLEAdvertising             NimBLEAdvertising
#define BLEServer                  NimBLEServer

#include "Print.h"
#include "keycodes.h"

#define BLE_KEYBOARD_VERSION "0.3.4"
#define BLE_KEYBOARD_VERSION_MAJOR 0
#define BLE_KEYBOARD_VERSION_MINOR 3
#define BLE_KEYBOARD_VERSION_REVISION 4

// NKRO configuration
#define NKRO_KEY_COUNT 252 // Surprise! "N" in "N-Key Rollover" stands for "252" in my implementation.

// Gamepad configuration
#define GAMEPAD_BUTTON_COUNT 64
#define GAMEPAD_AXIS_COUNT 6

// Scanning/Polling interval
#define SCAN_INTERVAL 10

// Report IDs:
#define KEYBOARD_ID   0x01
#define NKRO_ID       0x02
#define MEDIA_KEYS_ID 0x03
#define MOUSE_ID      0x04
#define DIGITIZER_ID  0x05
#define GEMINIPR_ID   0x06
#define GAMEPAD_ID    0x07

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

// NKRO report structure
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys_bitmask[(NKRO_KEY_COUNT + 7) / 8];
} KeyReportNKRO;

// Mouse report structure
typedef struct {
  uint8_t buttons;
  // Relative mode fields (used when _useAbsolute = false)
  int8_t relX;
  int8_t relY;
  int8_t wheel;
  int8_t hWheel;
} PointerReport;

// GeminiPR packet structure
typedef struct {
  uint8_t byte0;  // Fn  to #6
  uint8_t byte1;  // S1- to H-
  uint8_t byte2;  // R-  to res2
  uint8_t byte3;  // pwr to -R
  uint8_t byte4;  // -P  to -D
  uint8_t byte5;  // #7  to -Z
} GeminiPRReport;

// Digitizer report structure
typedef struct {
    uint8_t buttons;
    uint8_t flags;     // In range, tip switch, etc.
    uint16_t x;
    uint16_t y;
    uint8_t pressure;  // 0-127 for Android compatibility
} DigitizerReport;

typedef struct {
  uint32_t buttons[2];
  int8_t hat;
  int16_t axes[GAMEPAD_AXIS_COUNT];
} GamepadReport;

// Button bitmask constants (for the 'buttons' parameter)
const uint8_t DIGITIZER_BTN1 = 0x01;    // Button 1 (tip button, if present)
const uint8_t DIGITIZER_BTN2 = 0x02;    // Button 2 (barrel/side button)
const uint8_t DIGITIZER_BTN3 = 0x04;    // Button 3 (eraser)
// Flag constants (internal, for 'flags' field)
const uint8_t DIGITIZER_FLAG_IN_RANGE     = 0x01;  // Bit 0
const uint8_t DIGITIZER_FLAG_TIP_SWITCH   = 0x02;  // Bit 1
const uint8_t DIGITIZER_FLAG_INVERT       = 0x04;  // Bit 2 (eraser)
const uint8_t DIGITIZER_FLAG_BARREL_SW    = 0x08;  // Bit 3

static const bool enabled = true;
static const bool disabled = false;

class BLEHID : public Print
    , public NimBLEServerCallbacks
    , public NimBLECharacteristicCallbacks
{
private:
  BLEHIDDevice*         hid;
  uint16_t              appearance = HID_KEYBOARD;
  std::string           deviceName;
  std::string           deviceManufacturer;
  uint8_t               batteryLevel;
  
  BLECharacteristic*    outputKeyboard;
  BLECharacteristic*    inputMediaKeys;
  BLECharacteristic*    inputNKRO;
  BLECharacteristic*    inputMouse;
  BLECharacteristic*    inputGeminiPR;
  BLECharacteristic*    inputDigitizer;
  BLECharacteristic*    inputGamepad;
  
  KeyReportNKRO         _keyReportNKRO;
  PointerReport         _pointerReport;
  GeminiPRReport        _geminiReport;
  DigitizerReport       _digitizerReport;
  GamepadReport         _gamepadReport;
  
  uint32_t              passkey = 0;           // PIN code (0 = no security)

  BLEAdvertising*       advertising;
  uint32_t              _mediaKeyBitmask;
  uint8_t               _mouseButtons;
  uint32_t              _delay_ms = 7;
  bool                  _useNKRO = true;      // Default to NKRO
  bool                  _useAbsolute = false; // Default to relative mouse mode

  uint16_t vid       =  0x05AC; // I picked random numbers here and it worked fine,
  uint16_t pid       =  0x820A; // idk if these actually matter at all for anything
  uint16_t version   =  0x0310;
  
  friend void           pollConnection(void * arg);
  uint8_t               last_connected_count = 0;   // previous poll result
  uint32_t              lastPollTime = 0;
  static const uint32_t POLL_INTERVAL = 1000;       // 1 second in milliseconds
  static void           securityCallback(uint32_t passkey); 
  void                  updateNKROBitmask(uint8_t k, bool pressed);
  uint8_t               countPressedKeys();
  uint32_t              mediaKeyToBitmask(uint16_t usageCode);
  uint16_t              _maxX = 32767;              // Maximum X coordinate
  uint16_t              _maxY = 32767;              // Maximum Y coordinate
  bool                  _autoMode = true;           // Auto-detect mode based on method calls
  uint16_t              _screenWidth = 1920;        // Default screen width
  uint16_t              _screenHeight = 1080;       // Default screen height
  bool                  _digitizerConfigured = false;
  void                  _detectModeFromAppearance();
  bool                  _shouldUseAbsoluteMode();

public:
  BLEHID(std::string deviceName = "ESP32 Keyboard", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  
  ~BLEHID();
  
  void begin(void);
  void update(void);
  void end(void);
  
 // Security methods
  void setPIN(const char* pin);                 // Set 6-digit PIN like "123456"
  void setPIN(uint32_t pin);                    // Set numeric PIN
  void disableSecurity(bool enable = true);     // Enable/disable security
  bool isSecurityEnabled() const;               // Check if security is enabled
  
  void setAppearance(uint16_t newAppearance);
  
  // The library differentiates between keys, modifiers, and media keys by storing them using 3 different integer types
  size_t press(uint8_t k);           // I went with uint8_t for normal keycodes
  size_t press(int16_t modifier);    // I chose int16_t for modifiers
  size_t press(uint16_t mediaKey);   // I picked uint16_t for media keys
  size_t press(char b = MOUSE_LEFT); // Finally, char is for mouse clicks . . .
  size_t press(int32_t stenoKey);    // . . . And int32_t is for the steno keys
  size_t press(int8_t button);       // Next, int8_t is for the gamepad buttons
  
  size_t release(uint8_t k);
  size_t release(int16_t modifier);
  size_t release(uint16_t mediaKey);
  size_t release(char b = MOUSE_LEFT);
  size_t release(int32_t stenoKey);
  size_t release(int8_t button);
  
  size_t write(uint8_t c);
  size_t write(int16_t modifier);
  size_t write(uint16_t mediaKey);
  size_t write(const uint8_t *buffer, size_t size);
  
  void releaseAll();
  void mouseReleaseAll();
  void gamepadReleaseAll();
  
  void sendNKROReport();
  void sendMediaReport();
  void sendDigitizerReport();
  void sendGeminiPRReport();
  void sendGamepadReport();
  
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

  // Pointer helper functions
  void click(char b = MOUSE_LEFT);
  void click(uint16_t x, uint16_t y, char b = MOUSE_LEFT);
  void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
  bool mouseIsPressed(char b = MOUSE_LEFT);
  
  // Digitizer helper functions
  void moveTo(uint16_t x, uint16_t y, uint8_t pressure = 0, uint8_t buttons = 0);
  void beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure = 127);
  void updateStroke(uint16_t x, uint16_t y, uint16_t pressure);
  void endStroke(uint16_t x, uint16_t y);
  void useAbsoluteMode(bool state = true);
  bool isAbsoluteMode();
  void useAutoMode(bool state = true);
  void setDigitizerRange(uint16_t maxX, uint16_t maxY);
  bool isAutoModeEnabled();
  
  // GeminiPR helper functions
  void geminiStroke(const int32_t* keys, size_t count);
  uint8_t stenoCharToKey(char c);
  
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
  virtual uint32_t onPassKeyRequest();
  virtual void onAuthenticationComplete(ble_gap_conn_desc* desc);
  virtual bool onSecurityRequest();
  virtual void onMouseStarted(BLEServer *pServer) { };
  virtual void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
  virtual void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason);
  virtual void onWrite(NimBLECharacteristic* me);
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_KEYBOARD_H
