/**
 * @file SQUIDHID.h
 * @brief Main header file for the full library
 */

#ifndef SQUIDHID_H
#define SQUIDHID_H

#include "Print.h"

#include "config.h"
#include "drivers/Log/Log.h" 
#include "drivers/Appearance.h"
#include "drivers/Event/Types.h"
#include "drivers/Transport/Transport.h"

#if TRANSPORT == USB
  #include "drivers/Transport/USB/USBTransport.h"
#endif

#if TRANSPORT == PS2
  #include "drivers/Transport/PS2/PS2Transport.h"
#endif

#if TRANSPORT == BLE
  #include "drivers/Transport/BLE/BLETransport.h"
#endif

#if KEYBOARD_ENABLE
  #include "features/NKRO/NKRO.h"
#endif

#if MEDIA_ENABLE
  #include "features/Media/Media.h"
#endif

#if STENO_ENABLE
  #include "features/Steno/Steno.h"
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

#define SQUIDHID_VERSION "0.7.4"
#define SQUIDHID_VERSION_MAJOR 0
#define SQUIDHID_VERSION_MINOR 7
#define SQUIDHID_VERSION_REVISION 4

// Scanning/Polling interval
#define SCAN_INTERVAL 1

class SQUIDHID : public Print
    , public TransportCallbacks
{
private:
  std::unique_ptr<Transport>  transport;
  uint16_t                    appearance = KEYBOARD;
  std::string                 deviceName;
  std::string                 deviceManufacturer;
  uint8_t                     batteryLevel;
  
  uint16_t vid             =  0x046D; // I picked random numbers here and it worked fine,
  uint16_t pid             =  0xC52B; // idk if these actually matter at all for anything
  uint16_t version         =  0x0310;
  
  uint8_t                     last_connected_count = 0;   // previous poll result
  uint32_t                    lastPollTime = 0;
  static const uint32_t       POLL_INTERVAL = 200;       //  unit here is milliseconds
  uint32_t                    _delay_ms = 5; 
  
  SQUIDMATRIX                 matrix;
  SQUIDKEYMAP                 keymap;
  
  #if KEYBOARD_ENABLE
    SQUIDNKRO                 nkro;
  #endif
  
  #if MEDIA_ENABLE
    SQUIDMEDIA                media;
  #endif
  
  #if MOUSE_ENABLE
    SQUIDMOUSE                mouse;
  #endif
  
  #if DIGITIZER_ENABLE
    SQUIDTABLET              digitizer;
  #endif
  
  #if STENO_ENABLE
    SQUIDSTENO steno;
  #endif
  
  #if GAMEPAD_ENABLE
    SQUIDGAMEPAD               gamepad;
  #endif
  
public:
  SQUIDHID(std::string deviceName = "SquidHID", 
           std::string deviceManufacturer = "SquidHID", 
           uint8_t batteryLevel = 100,
           TransportType type = TransportType::BLE);
  
  ~SQUIDHID();
  
  void onConnect() override;
  void onDisconnect() override;
  void onDataReceived(const uint8_t* data, size_t length) override;
  
  void begin(const squid_matrix& matrix, const squid_map& keymap);
  void begin(void);
  void update(void);
  void end(void);

  // Transport management
  void setTransport(std::unique_ptr<Transport> newTransport);
  Transport* getTransport() { return transport.get(); }

  void setAppearance(uint16_t newAppearance);
  
  size_t write(const uint8_t *buffer, size_t size);
  void   releaseAll();
  
  // BLE helper functions
  bool isConnected(void);
  void pollConnection(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);  
  void setManufacturer(std::string deviceManufacturer);
  void setDelay(uint32_t ms);

  // Misc hardware helper functions
  void setVendorId(uint16_t vid);
  void setProductId(uint16_t pid);
  void setVersion(uint16_t version);
  
  // Matrix and Keymap methods
  void    setupMatrix(const squid_matrix& matrix);
  void    setupKeymap(const squid_map& keymap);
  void    updateMatrix();
  bool    isKeyPressed(size_t switch_index);
  
  #if KEYBOARD_ENABLE
    size_t  press(NKROKey k);
    size_t  press(ModKey modifier);
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
  
  #if STENO_ENABLE
    size_t  press(StenoKey stenoKey);
    size_t  release(StenoKey stenoKey);
    void    stenoStroke(const StenoKey* keys, size_t count);
    void    sendStenoReport();
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
  
    LogLevel getLogLevel() const;
    void     setLogLevel(LogLevel level);
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

};

#endif
