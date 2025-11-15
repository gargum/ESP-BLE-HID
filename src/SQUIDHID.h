/**
 * @file SQUIDHID.h
 * @brief Main header file
 */

#if defined(CONFIG_BT_ENABLED)
#elif defined(ARDUINO_ARCH_ESP32)
  #define CONFIG_BT_ENABLED 
#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)
  #define CONFIG_BT_ENABLED
#endif
#if defined(CONFIG_BT_ENABLED)
#define NIMBLE_CFG_TRANSPORT_HCI_UART 0
#define NIMBLE_CFG_TRANSPORT_HCI_IPC 0

#include "drivers/Interface/Interface.h"
#include "drivers/Interface/NimBLE/NimBLE.h"

#include "Print.h"

#include "config.h"
#include "features/Appearance.h"
#include "drivers/Log/Log.h" 
#include "drivers/Event/Types.h"

#if KEYBOARD_ENABLE
  #include "features/NKRO/NKRO.h"
#endif

#if MEDIA_ENABLE
  #include "features/Media/Media.h"
#endif

#if GEMINIPR_ENABLE
  #include "features/Steno/GeminiPR.h"
#endif

#if GAMEPAD_ENABLE
  #include "features/Gamepad/Gamepad.h"
#endif

#if MOUSE_ENABLE
  #include "features/Mouse/Mouse.h"
#endif

#if DIGITIZER_ENABLE
  #include "features/Digitizer/Digitizer.h"
#endif

#define BLE_KEYBOARD_VERSION "0.3.4"
#define BLE_KEYBOARD_VERSION_MAJOR 0
#define BLE_KEYBOARD_VERSION_MINOR 3
#define BLE_KEYBOARD_VERSION_REVISION 4

// Scanning/Polling interval
#define SCAN_INTERVAL 10

class SQUIDHID : public Print
    , public SquidServerCallbacks
    , public SquidCharacteristicCallbacks
{
private:
  P_SVAL(SquidInterface, ble, nullptr); 
  P_VAL(SquidHIDDevice,  hid, nullptr);
  P_VAL(SquidAdvertising,advertising, nullptr); 
  SquidServer*           pServer = nullptr;
  uint16_t               appearance = HID_KEYBOARD;
  std::string            deviceName;
  std::string            deviceManufacturer;
  uint8_t                batteryLevel;
  
  uint16_t vid     =     0x046D; // I picked random numbers here and it worked fine,
  uint16_t pid     =     0xC52B; // idk if these actually matter at all for anything
  uint16_t version =     0x0310;
  
  bool initialized =     false;
  friend void            pollConnection(void * arg);
  uint8_t                last_connected_count = 0;   // previous poll result
  uint32_t               lastPollTime = 0;
  static const uint32_t  POLL_INTERVAL = 1000;       // 1 second in milliseconds


  SquidCharacteristic*   outputKeyboard = nullptr;
  uint32_t               _delay_ms = 7; 
  
  #if KEYBOARD_ENABLE
    SQUIDNKRO              nkro;
    SquidCharacteristic*   inputNKRO = nullptr;
  #endif
  
  #if MEDIA_ENABLE
    SQUIDMEDIA             media;
    SquidCharacteristic*   inputMediaKeys = nullptr;
  #endif
  
  #if MOUSE_ENABLE
    SQUIDMOUSE             mouse;
    SquidCharacteristic*   inputMouse = nullptr;
  #endif
  
  #if DIGITIZER_ENABLE
    SQUIDTABLET            digitizer;
    SquidCharacteristic*   inputDigitizer = nullptr;
  #endif
  
  #if GEMINIPR_ENABLE
    SQUIDSTENO             steno;
    // SPP UUIDs
    static const char*     SERIAL_SERVICE_UUID;
    static const char*     SERIAL_CHARACTERISTIC_UUID_TX;
    static const char*     SERIAL_CHARACTERISTIC_UUID_RX;
    P(SquidService,        serialService);
    P(SquidCharacteristic, serialInput);
    P(SquidCharacteristic, serialOutput);
  #endif
  
  #if GAMEPAD_ENABLE
    SQUIDGAMEPAD           gamepad;
    SquidCharacteristic*   inputGamepad = nullptr;
  #endif
  
  // Advertisement data pointers
  P(SquidAdvertisementData, advData);
  P(SquidAdvertisementData, scanData);
  
public:
  SQUIDHID(std::string deviceName            = "ESP32 Keyboard", 
         std::string deviceManufacturer    = "Espressif", 
         uint8_t batteryLevel              = 100,
         SquidFactory::Implementation impl = IMPL);
  
  ~SQUIDHID();
  
  void begin(void);
  void update(void);
  void end(void);

  void setAppearance(uint16_t newAppearance);
  
  size_t write(const uint8_t *buffer, size_t size);
  void   releaseAll();
  
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
    size_t  press(NKROKey k);           // I went with uint8_t for normal keycodes
    size_t  press(ModKey modifier);    // I chose int16_t for modifiers
    size_t  release(NKROKey k);
    size_t  release(ModKey modifier);
    size_t  write(uint8_t c);
    size_t  write(ModKey modifier);
    void    useNKRO(bool state = enabled);
    void    use6KRO(bool state = enabled);
    bool    isNKROEnabled();
    void    setModifiers(ModKey modifiers);
    uint8_t getModifiers();
    void    sendNKROReport();
  #endif
  
  #if MEDIA_ENABLE
    size_t press(MediaKey mediaKey);
    size_t release(MediaKey mediaKey);
    size_t write(MediaKey mediaKey);
    void   sendMediaReport();
  #endif
  
  #if MOUSE_ENABLE
    size_t press(MouseKey b = MO_BTN1);
    size_t release(MouseKey b = MO_BTN1);
    void   click(MouseKey b = MO_BTN1);
    void   move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
    bool   mouseIsPressed(MouseKey b = MO_BTN1);
  #endif
  
  #if DIGITIZER_ENABLE
    void click(uint16_t x, uint16_t y, DigitizerKey b = DI_BTN1);
    void moveTo(uint16_t x, uint16_t y, uint8_t pressure = 0, DigitizerKey buttons = DigitizerKey{0});
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
    size_t  press(StenoKey stenoKey);    // . . . And int32_t is for the steno keys
    size_t  release(StenoKey stenoKey);
    void    geminiStroke(const StenoKey* keys, size_t count);
    uint8_t stenoCharToKey(char c);
    void    sendGeminiPRReport();
    // SPP Methods
    void    sendSerialData(const uint8_t* data, size_t length);
    bool    isSerialConnected();
  #endif
  
  #if GAMEPAD_ENABLE
    size_t   press(GamepadButton button);
    size_t   release(GamepadButton button);
    bool     gamepadIsPressed(GamepadButton button);
    void     gamepadSetLeftStick(int16_t x, int16_t y);
    void     gamepadSetRightStick(int16_t x, int16_t y);
    void     gamepadSetTriggers(int16_t left, int16_t right);
    void     gamepadGetLeftStick(int16_t &x, int16_t &y);
    void     gamepadGetRightStick(int16_t &x, int16_t &y);
    void     gamepadSetAxis(GamepadAnalogue axis, int16_t value);
    int16_t  gamepadGetAxis(GamepadAnalogue axis);
    void     gamepadSetAllAxes(int16_t values[GAMEPAD_ANALOGUE_COUNT]);
    void     sendGamepadReport();
  #endif
  
    void     setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;
    void     initialize(std::function<void(const LogEntry&)> handler = nullptr);
    void     log(LogLevel level, const std::string& tag, const std::string& message);
    void     processQueue();
    void     flush();
    void     setMaxQueueSize(uint32_t size);
    size_t   getQueueSize() const;
    bool     isInitialized() const;
    bool     isQueueEmpty() const;
    
    // Platform-specific control methods
    #if defined(SQUIDHID_PLATFORM_ESP32)
    void setESP32LogLevel(esp_log_level_t level);
    #elif defined(SQUIDHID_PLATFORM_NRF52)
    void setNRF52LogLevel(nrf_log_severity_t severity);
    #endif
  
protected:
  virtual void onStarted(SquidServer *pServer) { };
  virtual void onConnect(SquidServer *pServer);
  virtual void onDisconnect(SquidServer *pServer);
  virtual void onWrite(SquidCharacteristic *me);
};

#endif // CONFIG_BT_ENABLED
