#if defined(CONFIG_BT_ENABLED)
#elif defined(ARDUINO_ARCH_ESP32)
  #define CONFIG_BT_ENABLED 
#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)
  #define CONFIG_BT_ENABLED
#endif
#if defined(CONFIG_BT_ENABLED)
#define NIMBLE_CFG_TRANSPORT_HCI_UART 0
#define NIMBLE_CFG_TRANSPORT_HCI_IPC 0

#include "NimBLE2904.h"
#include "NimBLEDevice.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"
#include "NimBLEAdvertising.h"
#include "NimBLEServer.h"
#include "NimBLEService.h"
#include "NimBLEUUID.h"
#include "NimBLEUtils.h"
#include "NimBLEUUID.h"

#define BLEDevice                  NimBLEDevice
#define BLEServerCallbacks         NimBLEServerCallbacks
#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks
#define BLEHIDDevice               NimBLEHIDDevice
#define BLECharacteristic          NimBLECharacteristic
#define BLEAdvertising             NimBLEAdvertising
#define BLEServer                  NimBLEServer

#include "Print.h"

#include "config.h"
#include "features/Appearance.h"

#if KEYBOARD_ENABLE
  #include "features/NKRO.h"
#endif

#if MEDIA_ENABLE
  #include "features/Media.h"
#endif

#if GEMINIPR_ENABLE
  #include "features/GeminiPR.h"
#endif

#if GAMEPAD_ENABLE
  #include "features/Gamepad.h"
#endif

#if POINTER_ENABLE
  #include "features/Mouse.h"
  #include "features/Digitizer.h"
#endif

#define BLE_KEYBOARD_VERSION "0.3.4"
#define BLE_KEYBOARD_VERSION_MAJOR 0
#define BLE_KEYBOARD_VERSION_MINOR 3
#define BLE_KEYBOARD_VERSION_REVISION 4

// Scanning/Polling interval
#define SCAN_INTERVAL 10

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
  
  uint16_t vid       =  0x046D; // I picked random numbers here and it worked fine,
  uint16_t pid       =  0xC52B; // idk if these actually matter at all for anything
  uint16_t version   =  0x0310;
  
  friend void           pollConnection(void * arg);
  uint8_t               last_connected_count = 0;   // previous poll result
  uint32_t              lastPollTime = 0;
  static const uint32_t POLL_INTERVAL = 1000;       // 1 second in milliseconds
  static void           securityCallback(uint32_t passkey);
  uint32_t              passkey = 0;           // PIN code (0 = no security)

  BLEAdvertising*       advertising;  
  BLECharacteristic*    outputKeyboard;
  uint32_t              _delay_ms = 7; 
  
  #if KEYBOARD_ENABLE
    BLECharacteristic*    inputNKRO;
    KeyReportNKRO         _keyReportNKRO;
    bool                  _useNKRO = true;      // Default to NKRO
    uint8_t               countPressedKeys();
    uint8_t               charToKeyCode(char c, bool *needShift);
    void                  updateNKROBitmask(uint8_t k, bool pressed);
  #endif
  
  #if MEDIA_ENABLE
    BLECharacteristic*    inputMediaKeys;
    uint32_t              _mediaKeyBitmask;
    uint32_t              mediaKeyToBitmask(uint16_t usageCode);
  #endif
  
  #if POINTER_ENABLE
    BLECharacteristic*    inputMouse;
    PointerReport         _pointerReport;
    uint8_t               _mouseButtons;
    BLECharacteristic*    inputDigitizer;
    DigitizerReport       _digitizerReport;
    bool                  _useAbsolute = false; // Default to relative mouse mode
    uint16_t              _maxX = 32767;              // Maximum X coordinate
    uint16_t              _maxY = 32767;              // Maximum Y coordinate
    bool                  _autoMode = true;           // Auto-detect mode based on method calls
    uint16_t              _screenWidth = 1920;        // Default screen width
    uint16_t              _screenHeight = 1080;       // Default screen height
    bool                  _digitizerConfigured = false;
    void                  _detectModeFromAppearance();
    bool                  _shouldUseAbsoluteMode();
  #endif
  
  #if GEMINIPR_ENABLE
    GeminiPRReport        _geminiReport;
    // SPP (Serial Port Profile) members
    BLEService*         serialService;
    BLECharacteristic*  serialInput;
    BLECharacteristic*  serialOutput;
    BLE2904*            serialOutputDescriptor;
    bool                serialConnected;
    // SPP UUIDs
    static const char* SERIAL_SERVICE_UUID;
    static const char* SERIAL_CHARACTERISTIC_UUID_TX;
    static const char* SERIAL_CHARACTERISTIC_UUID_RX;
  #endif
  
  #if GAMEPAD_ENABLE
    BLECharacteristic*    inputGamepad;
    BLECharacteristic*    outputGamepad;
    GamepadReport         _gamepadReport;
  #endif
  
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
  
  size_t write(const uint8_t *buffer, size_t size);
  void releaseAll();
  
  // BLE helper functions
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);  
  void setManufacturer(std::string deviceManufacturer);
  void setDelay(uint32_t ms);

  // Misc hardware helper functions
  void setVendorId(uint16_t vid);
  void setProductId(uint16_t pid);
  void setVersion(uint16_t version);
  
  #if KEYBOARD_ENABLE
    size_t press(uint8_t k);           // I went with uint8_t for normal keycodes
    size_t press(int16_t modifier);    // I chose int16_t for modifiers
    size_t release(uint8_t k);
    size_t release(int16_t modifier);
    size_t write(uint8_t c);
    size_t write(int16_t modifier);
    void useNKRO(bool state = enabled);
    void use6KRO(bool state = enabled);
    bool isNKROEnabled();
    void setModifiers(uint8_t modifiers);
    uint8_t getModifiers();
    void sendNKROReport();
  #endif
  
  #if MEDIA_ENABLE
    size_t press(uint16_t mediaKey);   // I picked uint16_t for media keys
    size_t release(uint16_t mediaKey);
    size_t write(uint16_t mediaKey);
    void setMediaKeyBitmask(uint32_t bitmask);
    uint32_t getMediaKeyBitmask();
    void addMediaKey(uint16_t mediaKey);
    void removeMediaKey(uint16_t mediaKey);
    void sendMediaReport();
  #endif
  
  #if POINTER_ENABLE
    size_t press(char b = MOUSE_LEFT); // Finally, char is for mouse clicks . . .
    size_t release(char b = MOUSE_LEFT);
    void mouseReleaseAll();
    void click(char b = MOUSE_LEFT);
    void click(uint16_t x, uint16_t y, char b = MOUSE_LEFT);
    void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
    bool mouseIsPressed(char b = MOUSE_LEFT);
    void moveTo(uint16_t x, uint16_t y, uint8_t pressure = 0, uint8_t buttons = 0);
    void beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure = 127);
    void updateStroke(uint16_t x, uint16_t y, uint16_t pressure);
    void endStroke(uint16_t x, uint16_t y);
    void useAbsoluteMode(bool state = true);
    bool isAbsoluteMode();
    void useAutoMode(bool state = true);
    void setDigitizerRange(uint16_t maxX, uint16_t maxY);
    bool isAutoModeEnabled();
    void sendDigitizerReport();
  #endif
  
  #if GEMINIPR_ENABLE
    size_t press(int32_t stenoKey);    // . . . And int32_t is for the steno keys
    size_t release(int32_t stenoKey);
    void geminiStroke(const int32_t* keys, size_t count);
    uint8_t stenoCharToKey(char c);
    void sendGeminiPRReport();
    // SPP Methods
    void sendSerialData(const uint8_t* data, size_t length);
    bool isSerialConnected();
  #endif
  
  #if GAMEPAD_ENABLE
    size_t press(int8_t button);       // Next, int8_t is for the gamepad buttons
    size_t release(int8_t button);
    bool gamepadIsPressed(int8_t button);
    void gamepadSetLeftStick(int16_t x, int16_t y);
    void gamepadSetRightStick(int16_t x, int16_t y);
    void gamepadSetTriggers(int16_t left, int16_t right);
    void gamepadGetLeftStick(int16_t &x, int16_t &y);
    void gamepadGetRightStick(int16_t &x, int16_t &y);
    void gamepadSetAxis(int8_t axis, int16_t value);
    int16_t gamepadGetAxis(int8_t axis);
    void gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]);
    void sendGamepadReport();
  #endif
  
protected:
  virtual void onStarted(BLEServer *pServer) { };
  virtual uint32_t onPassKeyRequest();
  virtual void onAuthenticationComplete(ble_gap_conn_desc* desc);
  virtual bool onSecurityRequest();
  virtual void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
  virtual void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason);
  virtual void onWrite(NimBLECharacteristic* me);
};

#endif // CONFIG_BT_ENABLED
