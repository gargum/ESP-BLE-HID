/**
 * @file SQUIDHID.cpp
 * @brief Implementation of the full library
 */

#include "SQUIDHID.h"

#define KEYBOARD_ID   0x01

static const uint8_t _basicReportDescriptor[] = {
  // ------------------------------------------------- Keyboard
  USAGE_PAGE(1),      0x01,                      USAGE(1),           0x06,
  COLLECTION(1),      0x01,                      REPORT_ID(1),       KEYBOARD_ID,

  // Modifiers (8 bits)
  USAGE_PAGE(1),      0x07,                      USAGE_MINIMUM(1),   0xE0,
  USAGE_MAXIMUM(1),   0xE7,                      LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_SIZE(1),     0x01,
  REPORT_COUNT(1),    0x08,                      HIDINPUT(1),        0x02,

  // Reserved byte
  REPORT_COUNT(1),    0x01,                      REPORT_SIZE(1),     0x08,
  HIDINPUT(1),        0x01,

  // Key array (6 bytes)
  REPORT_COUNT(1),    0x06,                      REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x65,
  USAGE_PAGE(1),      0x07,                      USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,                      HIDINPUT(1),        0x00,

  // 5 LEDs
  USAGE_PAGE(1),      0x08,                      USAGE_MINIMUM(1),   0x01,
  USAGE_MAXIMUM(1),   0x05,                      LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,                      REPORT_COUNT(1),    0x05,
  REPORT_SIZE(1),     0x01,                      HIDOUTPUT(1),       0x02,

  // 3-bit padding
  REPORT_COUNT(1),    0x03,                      REPORT_SIZE(1),     0x01,
  HIDOUTPUT(1),       0x03,                      END_COLLECTION(0),
};

const size_t       descriptorSize = sizeof(_basicReportDescriptor) 
  
#if KEYBOARD_ENABLE
  +  sizeof(_nkroReportDescriptor)  
#endif

#if MEDIA_ENABLE
  +  sizeof(_mediakeyReportDescriptor)
#endif

#if SPACEMOUSE_ENABLE
  +  sizeof(_spacemouseReportDescriptor)
#else

#if MOUSE_ENABLE
  +  sizeof(_mouseReportDescriptor)
#endif

#if DIGITIZER_ENABLE
  +  sizeof(_digitizerReportDescriptor)
#endif

#if GAMEPAD_ENABLE
  +  sizeof(_gamepadReportDescriptor)  
#endif
#endif

#if STENO_ENABLE
  +  sizeof(_stenoReportDescriptor)
#endif
  ;                      
                        
static uint8_t     _hidReportDescriptor[descriptorSize];
static const char* LOG_TAG = "SQUIDHID";
bool               getInitialized = false;

SQUIDHID*          _activeSQUIDHIDInstance = nullptr;

class HIDDescriptorInitializer {
public:
    HIDDescriptorInitializer() {
        uint8_t* current = _hidReportDescriptor;
        
        memcpy(current, _basicReportDescriptor, sizeof(_basicReportDescriptor));
        current += sizeof(_basicReportDescriptor);
        
        #if KEYBOARD_ENABLE
        memcpy(current, _nkroReportDescriptor, sizeof(_nkroReportDescriptor));
        current += sizeof(_nkroReportDescriptor);
        #endif
        
        #if MEDIA_ENABLE
        memcpy(current, _mediakeyReportDescriptor, sizeof(_mediakeyReportDescriptor));
        current += sizeof(_mediakeyReportDescriptor);
        #endif
        
        #if SPACEMOUSE_ENABLE
        memcpy(current, _spacemouseReportDescriptor, sizeof(_spacemouseReportDescriptor));
        current += sizeof(_spacemouseReportDescriptor);
        #else
        
        #if MOUSE_ENABLE
        memcpy(current, _mouseReportDescriptor, sizeof(_mouseReportDescriptor));
        current += sizeof(_mouseReportDescriptor);
        #endif
        
        #if DIGITIZER_ENABLE
        memcpy(current, _digitizerReportDescriptor, sizeof(_digitizerReportDescriptor));
        current += sizeof(_digitizerReportDescriptor);
        #endif
        
        #if GAMEPAD_ENABLE
        memcpy(current, _gamepadReportDescriptor, sizeof(_gamepadReportDescriptor));
        current += sizeof(_gamepadReportDescriptor);
        #endif
        #endif
        
        #if STENO_ENABLE
        memcpy(current, _stenoReportDescriptor, sizeof(_stenoReportDescriptor));
        current += sizeof(_stenoReportDescriptor);
        #endif
        
        SQUID_LOG_DEBUG("HID", "Complete HID descriptor built - Total size: %zu", descriptorSize);
    }
};

static HIDDescriptorInitializer _hidDescriptorInitializer;

// This is a "constructor". It takes that class from the SQUIDHID.h file, and turns it into "objects" that can actually be used.
SQUIDHID::SQUIDHID(std::string deviceName, std::string deviceManufacturer, 
                   uint8_t batteryLevel, TransportType type) 
    : deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) 
    , matrix()
    , keymap()
    #if LED_ENABLE
    , leds(nullptr)
    , ledPin(6)
    , ledCount(0)
    , ledType(NEO_GRB)
    #endif
    #if OLED_ENABLE
    , oledDisplay(nullptr)
    , oledInitialized(false)
    #endif
    , lastPollTime(0) 
{
    // Factory method for creating transport
    auto createTransport = [this, type]() -> std::unique_ptr<Transport> {
        switch (type) {
            #if TRANSPORT == USB
            case TransportType::USB:
                return std::make_unique<USBTransport>();
            #endif
                
            #if TRANSPORT == BLE
            case TransportType::BLE:
                return std::make_unique<BLETransport>();
            #endif
                
            default:
                #if TRANSPORT == USB
                    return std::make_unique<USBTransport>();
                #elif TRANSPORT == BLE
                    return std::make_unique<BLETransport>();
                #else
                    #error "No valid transport configured"
                #endif
        }
    };
    
    transport = createTransport();
    
    // Common transport setup
    if (transport) {
        transport->setDeviceInfo(deviceName.c_str(), deviceManufacturer.c_str(), vid, pid, version);
        transport->setBatteryLevel(batteryLevel);
        transport->setAppearance(appearance);
        transport->setCallbacks(this);
    }
    
    SQUIDLOGS::getInstance().initialize(); 
    _activeSQUIDHIDInstance = this;
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID instance created with %s transport", 
                   type == TransportType::USB ? "USB" : "BLE");
}

SQUIDHID::~SQUIDHID() {
    #if LED_ENABLE
    if (leds) {
        delete leds;
        leds = nullptr;
    }
    #endif
    
    #if OLED_ENABLE
    if (oledDisplay) {
        delete oledDisplay;
        oledDisplay = nullptr;
    }
    #endif
    
    #if MCP_ENABLE
      if (mcpExpander) {
        delete mcpExpander;
        mcpExpander = nullptr;
      }
    #endif
    
    if (_activeSQUIDHIDInstance == this) {
        _activeSQUIDHIDInstance = nullptr;
    }
}

void SQUIDHID::begin(const squid_matrix& matrix, const std::vector<std::vector<LayerKeymapEntry>>& layers) {
    // Initialize transport
    begin();
    
    // Setup matrix and keymap with layers
    setupMatrix(matrix);
    setupKeymap(layers);
    
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID started with matrix and layered keymap");
}

void SQUIDHID::begin(void) {
    if (!transport) {
        SQUID_LOG_ERROR(LOG_TAG, "No transport configured");
        return;
    }
    
    SQUID_LOG_INFO(LOG_TAG, "Starting SQUIDHID with transport...");
    SQUID_LOG_DEBUG(LOG_TAG, "Setting HID report map...");
    
    // The transport needs the report map before begin() for the USB transport layer
    if (descriptorSize > 0) {
        transport->setReportMap((uint8_t *)_hidReportDescriptor, descriptorSize);
    } else {
        SQUID_LOG_ERROR(LOG_TAG, "No HID descriptor built!");
        return;
    }
    
    // Initialize transport
    SQUID_LOG_DEBUG(LOG_TAG, "Initializing transport...");
    if (!transport->begin()) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to initialize transport");
        return;
    }
    
    // Initialize LEDs if configured
    #if LED_ENABLE
    if (ledCount > 0 && leds) {
        if (leds->begin()) {
            SQUID_LOG_INFO(LOG_TAG, "LEDs initialized on pin %d with %d LEDs", ledPin, ledCount);
            // Turn off all LEDs initially
            leds->clear();
            leds->show();
        } else {
            SQUID_LOG_ERROR(LOG_TAG, "Failed to initialize LEDs");
        }
    }
    #endif
    
    // Initialize OLED if configured and pins are set
    #if OLED_ENABLE
    if (!oledDisplay) {
    // Auto-initialize OLED with pins and dimensions from config.h
    initializeOLED(SDA_PIN, SCL_PIN, OLED_WIDTH, OLED_HEIGHT);
    SQUID_LOG_INFO(LOG_TAG, "Auto-initialized OLED %dx%d with SDA:%d, SCL:%d", 
                   OLED_WIDTH, OLED_HEIGHT, SDA_PIN, SCL_PIN);
    }
    
    if (oledDisplay && !oledInitialized) {
        oledDisplay->begin();
        oledInitialized = true;
        oledDisplay->clear(OLED::BLACK);
        oledDisplay->display();
        
        // Show startup screen
        oledShowSquidLogo();
        oledDisplay->display();
        delay(1000); // Show logo for 1 second
        
        oledClear();
        
        SQUID_LOG_INFO(LOG_TAG, "OLED display initialized");
    }
    #endif
    
    // Start advertising
    SQUID_LOG_DEBUG(LOG_TAG, "Starting advertising...");
    if (!transport->connect()) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to start advertising");
        return;
    }
    
    #if MCP_ENABLE
      // Auto-initialize MCP if enabled in config
      if (!mcpInitialized) {
        #if defined(SDA_PIN) && defined(SCL_PIN) && I2C_ENABLE
          initializeMCP_I2C(); // Use default I2C address
          SQUID_LOG_INFO(LOG_TAG, "Auto-initialized MCP with SDA:%d, SCL:%d", SDA_PIN, SCL_PIN);
        #elif defined(CS_PIN) && SPI_ENABLE
          initializeMCP_SPI(CS_PIN);
          SQUID_LOG_INFO(LOG_TAG, "Auto-initialized MCP with SPI CS:%d", CS_PIN);
        #endif
      }
    #endif
    
    // Initialize features
    SQUID_LOG_DEBUG(LOG_TAG, "Initializing feature modules...");
    #if KEYBOARD_ENABLE
    nkro.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "NKRO keyboard support enabled");
    #endif
    
    #if MEDIA_ENABLE
    media.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "Media key support enabled");
    #endif
    
    #if SPACEMOUSE_ENABLE
    spacemouse.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "Spacemouse support enabled");
    #else
    
    #if MOUSE_ENABLE
    mouse.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "Mouse support enabled");
    #endif
    
    #if DIGITIZER_ENABLE
    digitizer.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "Digitizer support enabled");
    #endif
    
    #if GAMEPAD_ENABLE
    gamepad.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "Gamepad support enabled");
    
    #endif
    #endif

    #if STENO_ENABLE
    steno.begin(transport.get(), _delay_ms);
    SQUID_LOG_DEBUG(LOG_TAG, "PloverHID support enabled");
    #endif
    
    lastPollTime = millis();
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID started successfully - Waiting for connections...");
}

// Update/polling function
void SQUIDHID::update() {
    static uint32_t lastUpdateTime        = 0;
    static uint32_t lastPollTime          = 0;
    static uint32_t lastLogProcessTime    = 0;
    uint32_t currentTime                  = millis();
    
    if (currentTime - lastLogProcessTime >= 10) {
        lastLogProcessTime = currentTime;
        SQUID_LOG_PROCESS();
    }
  
    // Update transport layer
    if (transport) {
        transport->update();
    }
    
    if (currentTime - lastUpdateTime >= SCAN_INTERVAL) {
        lastUpdateTime = currentTime;
        
        if (_activeSQUIDHIDInstance) {
            uint32_t currentPollTime = millis();
            // Handle millis() rollover
            if (currentPollTime < lastPollTime) {
                lastPollTime = currentPollTime;
                return;
            }
            matrix.update();
        }
    }
    
    keymap.update();
    
    if (currentTime - lastPollTime >= POLL_INTERVAL) {
        lastPollTime = currentTime;
        pollConnection();
    }
    
    #if LED_ENABLE
    // Only update LEDs if they're dirty and ready to show
    if (ledsDirty && leds && leds->canShow()) {
        leds->show();
        ledsDirty = false;
    }
    #endif
    
    #if OLED_ENABLE
    // Only update OLED if it's dirty
    if (oledDirty && oledDisplay && oledInitialized) {
        oledDisplay->display();
        oledDirty = false;
    }
    #endif
}

void SQUIDHID::end(void) {
    if (transport) {
        transport->end();
    }
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID stopped");
}

//
// ----------------------------------------- Global Function Block
//

bool SQUIDHID::isMCPPin(uint8_t pin) const {
  // MCP pins are defined as A0-A7 = 100-107, B0-B7 = 108-115
  // I'm aliasing that to 0x80-0x8F in hopes that smaller numbers mean it'll run more efficiently
  return (pin >= 0x80 && pin <= 0x8F);
}

// This converts the A0-B7 pin definitions to MCP pin number 0-15
uint8_t SQUIDHID::toMCPPin(uint8_t pin) const {
  return pin - 0x80;
}

// Unified pinMode method 
void SQUIDHID::pinMode(uint8_t pin, uint8_t mode) {
  if (isMCPPin(pin)) {
    #if MCP_ENABLE
    if (mcpInitialized && mcpExpander) {
      uint8_t mcpPin = toMCPPin(pin);
      mcpExpander->pinMode(mcpPin, mode);
    } else {
      SQUID_LOG_WARN("SQUIDHID", "MCP expander not initialized, cannot set pin mode for MCP pin %d", pin);
    }
    #endif
  } else {
    // Regular Arduino pin
    ::pinMode(pin, mode);
  }
}

// Unified digitalWrite method
void SQUIDHID::digitalWrite(uint8_t pin, uint8_t value) {
  if (isMCPPin(pin)) {
    #if MCP_ENABLE
    if (mcpInitialized && mcpExpander) {
      uint8_t mcpPin = toMCPPin(pin);
      mcpExpander->digitalWrite(mcpPin, value);
    } else {
      SQUID_LOG_WARN("SQUIDHID", "MCP expander not initialized, cannot write to MCP pin %d", pin);
    }
    #endif
  } else {
    // Regular Arduino pin
    ::digitalWrite(pin, value);
  }
}

// Unified digitalRead method
uint8_t SQUIDHID::digitalRead(uint8_t pin) {
  if (isMCPPin(pin)) {
    #if MCP_ENABLE
    if (mcpInitialized && mcpExpander) {
      uint8_t mcpPin = toMCPPin(pin);
      return mcpExpander->digitalRead(mcpPin);
    } else {
      SQUID_LOG_WARN("SQUIDHID", "MCP expander not initialized, cannot read from MCP pin %d", pin);
      return LOW;
    }
    #endif
  } else {
    // Regular Arduino pin
    return ::digitalRead(pin);
  }
  return LOW;
}

#if MCP_ENABLE
#if I2C_ENABLE
bool SQUIDHID::initializeMCP_I2C(uint8_t i2c_addr, TwoWire *wire) {
  if (mcpExpander) {
    delete mcpExpander;
  }
  
  mcpExpander = new MCP23XXX();
  mcpInitialized = mcpExpander->begin_I2C(i2c_addr, wire);
  
  if (mcpInitialized) {
    SQUID_LOG_INFO("SQUIDHID", "MCP23XXX expander initialized via I2C at address 0x%02X", i2c_addr);
  } else {
    SQUID_LOG_ERROR("SQUIDHID", "Failed to initialize MCP23XXX expander via I2C");
    delete mcpExpander;
    mcpExpander = nullptr;
  }
  
  return mcpInitialized;
}
#endif
#if SPI_ENABLE
bool SQUIDHID::initializeMCP_SPI(uint8_t cs_pin, SPIClass *theSPI, uint8_t hw_addr) {
  if (mcpExpander) {
    delete mcpExpander;
  }
  
  mcpExpander = new MCP23XXX();
  mcpInitialized = mcpExpander->begin_SPI(cs_pin, theSPI, hw_addr);
  
  if (mcpInitialized) {
    SQUID_LOG_INFO("SQUIDHID", "MCP23XXX expander initialized via SPI with CS pin %d", cs_pin);
  } else {
    SQUID_LOG_ERROR("SQUIDHID", "Failed to initialize MCP23XXX expander via SPI");
    delete mcpExpander;
    mcpExpander = nullptr;
  }
  
  return mcpInitialized;
}
#endif
#endif

void SQUIDHID::onConnect() {
    SQUID_LOG_INFO(LOG_TAG, "Transport connected");
    
    // Notify feature modules about connection
    #if KEYBOARD_ENABLE
    nkro.onConnect();
    #endif
    #if MEDIA_ENABLE
    media.onConnect();
    #endif
    #if SPACEMOUSE_ENABLE
    spacemouse.onConnect();
    #else
    #if MOUSE_ENABLE
    mouse.onConnect();
    #endif
    #if DIGITIZER_ENABLE
    digitizer.onConnect();
    #endif
    #if GAMEPAD_ENABLE
    gamepad.onConnect();
    #endif
    #endif
    #if STENO_ENABLE
    steno.onConnect();
    #endif
    
}

bool SQUIDHID::isConnected(void) {
    return transport ? transport->isConnected() : false;
}

void SQUIDHID::pollConnection() {
    if (!transport) return;
    
    uint8_t cnt = transport->isConnected() ? 1 : 0;
    
    if (last_connected_count && !cnt) {   // Connection just dropped
        SQUID_LOG_WARN(LOG_TAG, "Poller: link lost - restarting advertising");
        
        // Over-the-top delay to ensure BLE stack is ready
        delay(100);
        
        if (isConnected()) {  // This will update the internal state (hopefully)
            SQUID_LOG_INFO(LOG_TAG, "Poller: Connection restored");
        } else {
            // Force reconnection attempt
            transport->disconnect();
            delay(50);
            transport->connect();
        }
    }
    last_connected_count = cnt;
}

void SQUIDHID::onDisconnect() {
    SQUID_LOG_INFO(LOG_TAG, "Transport disconnected");
    
    // Notify feature modules about disconnection
    #if KEYBOARD_ENABLE
    nkro.onDisconnect();
    #endif
    #if MEDIA_ENABLE
    media.onDisconnect();
    #endif
    #if SPACEMOUSE_ENABLE
    spacemouse.onDisconnect();
    #else
    #if MOUSE_ENABLE
    mouse.onDisconnect();
    #endif
    #if DIGITIZER_ENABLE
    digitizer.onDisconnect();
    #endif
    #if GAMEPAD_ENABLE
    gamepad.onDisconnect();
    #endif
    #endif
    #if STENO_ENABLE
    steno.onDisconnect();
    #endif

}

void SQUIDHID::addCombo(const KeyComboConfig& combo) { keymap.addCombo(combo); }

void SQUIDHID::setCombos(const std::vector<KeyComboConfig>& combos) { keymap.setCombos(combos); }

void SQUIDHID::clearCombos() { keymap.clearCombos(); }

void SQUIDHID::setComboTimeout(uint16_t timeout_ms) { keymap.setComboTimeout(timeout_ms); }

void SQUIDHID::releaseAll() {
  #if KEYBOARD_ENABLE
  nkro.releaseAll();
  #endif
  
  #if MEDIA_ENABLE
  media.releaseAll();
  #endif
  
  #if SPACEMOUSE_ENABLE
  spacemouse.releaseAll();
  #else
  
  #if MOUSE_ENABLE
  mouse.releaseAll();
  #endif
  
  #if GAMEPAD_ENABLE
  gamepad.releaseAll();
  
  #endif
  #endif
  
  #if STENO_ENABLE
  steno.releaseAll();
  #endif
}

void SQUIDHID::onDataReceived(const uint8_t* data, size_t length) {
    SQUID_LOG_DEBUG(LOG_TAG, "Received %zu bytes from transport", length);
    
    // Handle HID output reports (host -> device)
    if (length > 0) {
        uint8_t reportId = data[0];
        
    }
}

void SQUIDHID::setTransport(std::unique_ptr<Transport> newTransport) {
    if (transport) {
        transport->end();
    }
    transport = std::move(newTransport);
    if (transport) {
        transport->setDeviceInfo(deviceName.c_str(), deviceManufacturer.c_str(), vid, pid, version);
        transport->setBatteryLevel(batteryLevel);
        transport->setCallbacks(this);
    }
}

void SQUIDHID::setAppearance(uint16_t newAppearance) {
    this->appearance = newAppearance;
    
    // Pass appearance to transport layer
    if (transport) {
        transport->setAppearance(newAppearance);
    }
}

void SQUIDHID::setBatteryLevel(uint8_t level) {
    uint8_t oldLevel = this->batteryLevel;
    this->batteryLevel = level;
    
    if (transport) {
        transport->setBatteryLevel(level);
    }
    
    if (this->batteryLevel != oldLevel) {
        SQUID_LOG_INFO(LOG_TAG, "Battery level set: %d%% -> %d%%", oldLevel, this->batteryLevel);
    }
}

//must be called before begin in order to set the name
void SQUIDHID::setName(std::string deviceName) { this->deviceName = deviceName; }

//must be called before begin in order to set the manufacturer
void SQUIDHID::setManufacturer(std::string deviceManufacturer) { this->deviceManufacturer = deviceManufacturer; }

// Sets the waiting time (in milliseconds) between multiple keystrokes
void SQUIDHID::setDelay(uint32_t ms) { _delay_ms = ms; }

void SQUIDHID::setVendorId(uint16_t vid) { this->vid = vid; }

void SQUIDHID::setProductId(uint16_t pid) { this->pid = pid; }

void SQUIDHID::setVersion(uint16_t version) { this->version = version; }

void SQUIDHID::setupMatrix(const squid_matrix& matrix) {
    auto key_event_callback = [this](size_t switch_index, bool pressed) {
        this->keymap.handleKeyEvent(switch_index, pressed);
    };
    
    auto pinModeFunc = [this](uint8_t pin, uint8_t mode) {
        this->pinMode(pin, mode);
    };
    
    auto digitalWriteFunc = [this](uint8_t pin, uint8_t value) {
        this->digitalWrite(pin, value);
    };
    
    auto digitalReadFunc = [this](uint8_t pin) -> uint8_t {
        return this->digitalRead(pin);
    };
    
    // The unified GPIO functions are being used for matrix scanning because it makes weirder matrices easier to define
    this->matrix.begin(matrix, key_event_callback, pinModeFunc, digitalWriteFunc, digitalReadFunc);
    SQUID_LOG_INFO(LOG_TAG, "Keyboard matrix configured with %zu switches", matrix.size());
}

void SQUIDHID::setupKeymap(const std::vector<std::vector<LayerKeymapEntry>>& layers) {
    auto press_callback = [this](const KeymapEntry& key_entry) {
        switch (key_entry.type) {
            case KeypressType::NKRO_KEY:
                this->nkro.press(key_entry.key.nkro_key);
                break;
            case KeypressType::MOD_KEY:
                this->nkro.press(key_entry.key.mod_key);
                break;
            case KeypressType::SHIFTED_KEY:
                this->nkro.press(key_entry.key.shifted_key);
                break;
            case KeypressType::MEDIA_KEY:
                #if MEDIA_ENABLE
                this->media.press(key_entry.key.media_key);
                #endif
                break;
            case KeypressType::SPACEMOUSE_KEY:
                #if SPACEMOUSE_ENABLE
                this->spacemouse.press(key_entry.key.spacemouse_key);
                #else
                break;
            case KeypressType::MOUSE_KEY:
                #if MOUSE_ENABLE
                this->mouse.press(key_entry.key.mouse_key);
                #endif
                break;
            case KeypressType::GAMEPAD_BUTTON:
                #if GAMEPAD_ENABLE
                this->gamepad.press(key_entry.key.gamepad_button);
                #endif
                #endif
                break;
            case KeypressType::STENO_KEY:
                #if STENO_ENABLE
                this->steno.press(key_entry.key.steno_key);
                #endif
                break;
            default:
                break;
        }
    };
    
    auto release_callback = [this](const KeymapEntry& key_entry) {
        switch (key_entry.type) {
            case KeypressType::NKRO_KEY:
                this->nkro.release(key_entry.key.nkro_key);
                break;
            case KeypressType::MOD_KEY:
                this->nkro.release(key_entry.key.mod_key);
                break;
            case KeypressType::SHIFTED_KEY:
                this->nkro.release(key_entry.key.shifted_key);
                break;
            case KeypressType::MEDIA_KEY:
                #if MEDIA_ENABLE
                this->media.release(key_entry.key.media_key);
                #endif
                break;
            case KeypressType::SPACEMOUSE_KEY:
                #if SPACEMOUSE_ENABLE
                this->spacemouse.release(key_entry.key.spacemouse_key);
                #else
                break;
            case KeypressType::MOUSE_KEY:
                #if MOUSE_ENABLE
                this->mouse.release(key_entry.key.mouse_key);
                #endif
                break;
            case KeypressType::GAMEPAD_BUTTON:
                #if GAMEPAD_ENABLE
                this->gamepad.release(key_entry.key.gamepad_button);
                #endif
                #endif
                break;
            case KeypressType::STENO_KEY:
                #if STENO_ENABLE
                this->steno.release(key_entry.key.steno_key);
                #endif
                break;
            default:
                break;
        }
    };
    
    auto layer_change_callback = [this](uint8_t layer) {
        SQUID_LOG_INFO("LAYER", "Active layer changed to %d", layer);
        
        #if OLED_ENABLE
        this->oledShowLayerInfo(layer);
        #endif
    };
    
    keymap.begin(layers, press_callback, release_callback, layer_change_callback);
    
    SQUID_LOG_INFO(LOG_TAG, "Layered keymap configured with %zu layers", layers.size());
}

void SQUIDHID::setDefaultLayer(uint8_t layer) {
    keymap.setDefaultLayer(layer);
}

void SQUIDHID::momentaryLayer(uint8_t layer, bool pressed) {
    keymap.momentaryLayer(layer, pressed);
}

void SQUIDHID::toggleLayer(uint8_t layer) {
    keymap.toggleLayer(layer);
}

uint8_t SQUIDHID::getActiveLayer() const {
    return keymap.getActiveLayer();
}

bool SQUIDHID::isLayerActive(uint8_t layer) const {
    return keymap.isLayerActive(layer);
}

void SQUIDHID::updateMatrix() {
    this->matrix.update();
    keymap.update();
}

//
// ----------------------------------------- NKRO Keyboard Block
//

#if KEYBOARD_ENABLE
size_t SQUIDHID::press(NKROKey k) { return nkro.press(k); }

size_t SQUIDHID::press(ModKey modifier) { return nkro.press(modifier); }

size_t SQUIDHID::press(ShiftedKey shiftedKey) { return nkro.press(shiftedKey); }

size_t SQUIDHID::release(NKROKey k) { return nkro.release(k); }

size_t SQUIDHID::release(ModKey modifier) { return nkro.release(modifier); }

size_t SQUIDHID::release(ShiftedKey shiftedKey) { return nkro.release(shiftedKey); }

size_t SQUIDHID::write(uint8_t c) { return nkro.write(c); }

size_t SQUIDHID::write(ModKey modifier) { return nkro.write(modifier); }

size_t SQUIDHID::write(ShiftedKey shiftedKey) { return nkro.write(shiftedKey); }

void SQUIDHID::useNKRO(bool state) { nkro.useNKRO(state); }

void SQUIDHID::use6KRO(bool state) { nkro.use6KRO(state); }

bool SQUIDHID::isNKROEnabled() { return nkro.isNKROEnabled(); }

void SQUIDHID::setModifiers(ModKey modifiers) { nkro.setModifiers(modifiers); }

uint8_t SQUIDHID::getModifiers() { return nkro.getModifiers(); }

void SQUIDHID::sendNKROReport() { nkro.sendNKROReport(); }
#endif

//
// ----------------------------------------- Media key Block
//

#if MEDIA_ENABLE
size_t SQUIDHID::press(MediaKey mediaKey) { return media.press(mediaKey); }

size_t SQUIDHID::release(MediaKey mediaKey) { return media.release(mediaKey); }

size_t SQUIDHID::write(MediaKey mediaKey) { return media.write(mediaKey); }

void SQUIDHID::sendMediaReport() { media.sendMediaReport(); }
#endif

//
// ----------------------------------------- Spacemouse Block
//

#if SPACEMOUSE_ENABLE
void SQUIDHID::move(int16_t tx, int16_t ty, int16_t tz, int16_t rx, int16_t ry, int16_t rz) { spacemouse.move(tx, ty, tz, rx, ry, rz); }

void SQUIDHID::translate(int16_t tx, int16_t ty, int16_t tz) { spacemouse.translate(tx, ty, tz); }

void SQUIDHID::rotate(int16_t rx, int16_t ry, int16_t rz) { spacemouse.rotate(rx, ry, rz); }

void SQUIDHID::press(SpacemouseKey button) { spacemouse.press(button); }

void SQUIDHID::release(SpacemouseKey button) { spacemouse.release(button); }

bool SQUIDHID::spacemouseIsPressed(SpacemouseKey button) { return spacemouse.isPressed(button); }

void SQUIDHID::sendSpacemouseReport() { spacemouse.sendReport(); }


// 3DConnexion reports interfere with other pointers, so I'm using the 3DConnexion report to emulate the other pointers whenever necessary to get around that
#if MOUSE_ENABLE
void SQUIDHID::click(SpacemouseKey b) { spacemouse.click(b); }

void SQUIDHID::move(int16_t x, int16_t y, int16_t wheel, int16_t hWheel) { spacemouse.move(x, y, wheel, hWheel); }

void SQUIDHID::sendMouseReport() { spacemouse.sendReport(); }
#endif

#if DIGITIZER_ENABLE
void SQUIDHID::click(uint16_t x, uint16_t y, SpacemouseKey b) { spacemouse.click(x, y, b); }

void SQUIDHID::moveTo(uint16_t x, uint16_t y, uint8_t pressure, SpacemouseKey buttons) { spacemouse.moveTo(x, y, pressure, buttons); }

void SQUIDHID::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) { spacemouse.beginStroke(x, y, initialPressure); }

void SQUIDHID::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) { spacemouse.updateStroke(x, y, pressure); }

void SQUIDHID::endStroke(uint16_t x, uint16_t y) { spacemouse.endStroke(x, y); }

void SQUIDHID::setDigitizerRange(uint16_t maxX, uint16_t maxY) { spacemouse.setDigitizerRange(maxX, maxY); }

void SQUIDHID::sendDigitizerReport() { spacemouse.sendReport(); }
#endif

#if GAMEPAD_ENABLE
void SQUIDHID::setLeftStick(int16_t x, int16_t y) { spacemouse.gamepadSetLeftStick(x, y); }

void SQUIDHID::setRightStick(int16_t x, int16_t y) { spacemouse.gamepadSetRightStick(x, y); }

void SQUIDHID::setTriggers(int16_t left, int16_t right) { spacemouse.gamepadSetTriggers(left, right); }

void SQUIDHID::gamepadSetAxis(SpacemouseAnalogue axis, int16_t value) { spacemouse.gamepadSetAxis(axis, value); }

int16_t SQUIDHID::gamepadGetAxis(SpacemouseAnalogue axis) { return spacemouse.gamepadGetAxis(axis); }

void SQUIDHID::gamepadSetAllAxes(int16_t values[6]) { spacemouse.gamepadSetAllAxes(values); }

void SQUIDHID::sendGamepadReport() { spacemouse.sendReport(); }
#endif
#else

//
// ----------------------------------------- Mouse Block
//

#if MOUSE_ENABLE
size_t SQUIDHID::press(MouseKey b) { return mouse.press(b); }

size_t SQUIDHID::release(MouseKey b) { return mouse.release(b); }

void SQUIDHID::click(MouseKey b) { mouse.click(b); }

void SQUIDHID::move(int8_t x, int8_t y, int8_t wheel, int8_t hWheel) { mouse.move(x, y, wheel, hWheel); }

bool SQUIDHID::mouseIsPressed(MouseKey b) { return mouse.mouseIsPressed(b); }

void SQUIDHID::sendMouseReport() { mouse.sendMouseReport(); }
#endif

//
// ----------------------------------------- Digitizer Block
//

#if DIGITIZER_ENABLE
void SQUIDHID::click(uint16_t x, uint16_t y, DigitizerKey b) { digitizer.click(x, y, b); }

void SQUIDHID::moveTo(uint16_t x, uint16_t y, uint8_t pressure, DigitizerKey buttons) { digitizer.moveTo(x, y, pressure, buttons); }

void SQUIDHID::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) { digitizer.beginStroke(x, y, initialPressure); }

void SQUIDHID::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) { digitizer.updateStroke(x, y, pressure); }

void SQUIDHID::endStroke(uint16_t x, uint16_t y) { digitizer.endStroke(x, y); }

void SQUIDHID::setDigitizerRange(uint16_t maxX, uint16_t maxY) { digitizer.setDigitizerRange(maxX, maxY); }

void SQUIDHID::sendDigitizerReport() { digitizer.sendDigitizerReport(); }
#endif

//
// ----------------------------------------- Gamepad Block
//

#if GAMEPAD_ENABLE
size_t SQUIDHID::press(GamepadButton button) { return gamepad.press(button); }

size_t SQUIDHID::release(GamepadButton button) { return gamepad.release(button); }

bool SQUIDHID::gamepadIsPressed(GamepadButton button) { return gamepad.gamepadIsPressed(button); }

void SQUIDHID::setLeftStick(int16_t x, int16_t y) { gamepad.gamepadSetLeftStick(x, y); }

void SQUIDHID::setRightStick(int16_t x, int16_t y) { gamepad.gamepadSetRightStick(x, y); }

void SQUIDHID::setTriggers(int16_t left, int16_t right) { gamepad.gamepadSetTriggers(left, right); }

void SQUIDHID::getLeftStick(int16_t &x, int16_t &y) { gamepad.gamepadGetLeftStick(x, y); }

void SQUIDHID::getRightStick(int16_t &x, int16_t &y) { gamepad.gamepadGetRightStick(x, y); }

void SQUIDHID::gamepadSetAxis(GamepadAnalogue axis, int16_t value) { gamepad.gamepadSetAxis(axis, value); }

int16_t SQUIDHID::gamepadGetAxis(GamepadAnalogue axis) { return gamepad.gamepadGetAxis(axis); }

void SQUIDHID::gamepadSetAllAxes(int16_t values[GAMEPAD_ANALOGUE_COUNT]) { gamepad.gamepadSetAllAxes(values); }

void SQUIDHID::sendGamepadReport() { gamepad.sendGamepadReport(); }
#endif
#endif

//
// ----------------------------------------- PloverHID Stenotype Block
//

#if STENO_ENABLE
size_t SQUIDHID::press(StenoKey stenoKey) { return steno.press(stenoKey); }

size_t SQUIDHID::release(StenoKey stenoKey) { return steno.release(stenoKey); }

void SQUIDHID::stenoStroke(const StenoKey* keys, size_t count) { steno.stenoStroke(keys, count); }

void SQUIDHID::sendStenoReport() { steno.sendStenoReport(); }
#endif

//
// ----------------------------------------- LED Block (NeoPixel)
//

#if LED_ENABLE
void SQUIDHID::initializeLEDs(uint16_t count, int16_t pin, neoPixelType type) {
    if (leds) {
        delete leds;
    }
    
    ledCount = count;
    ledPin = pin;
    ledType = type;
    ledsDirty = true;  // Mark as dirty on initialization
    
    leds = new NeoPixel(count, pin, type);
    
    SQUID_LOG_INFO(LOG_TAG, "LED driver initialized for %d LEDs on pin %d", count, pin);
}

void SQUIDHID::setLEDColor(uint16_t index, uint32_t color) {
    if (leds && index < ledCount) {
        leds->setPixelColor(index, color);
        ledsDirty = true;  // Mark as dirty when color changes
    }
}

void SQUIDHID::setLEDColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (leds && index < ledCount) {
        leds->setPixelColor(index, leds->Color(r, g, b));
        ledsDirty = true;  // Mark as dirty when color changes
    }
}

void SQUIDHID::fillLEDs(uint8_t r, uint8_t g, uint8_t b) {
    if (leds) {
        uint32_t color = leds->Color(r, g, b);
        leds->fill(color);
        ledsDirty = true;  // Mark as dirty when fill changes
    }
}

void SQUIDHID::fillLEDsRGB(uint8_t r, uint8_t g, uint8_t b, uint16_t first, uint16_t count) {
    if (leds) {
        uint32_t color = leds->Color(r, g, b);
        leds->fill(color, first, count);
        ledsDirty = true;  // Mark as dirty when fill changes
    }
}

void SQUIDHID::clearLEDs() {
    if (leds) {
        leds->clear();
        ledsDirty = true;  // Mark as dirty when cleared
    }
}

void SQUIDHID::showLEDs() {
    if (leds && leds->canShow()) {
        leds->show();
        ledsDirty = false;  // Clear dirty flag after showing
    }
}

void SQUIDHID::setLEDBrightness(uint8_t brightness) {
    if (leds) {
        leds->setBrightness(brightness);
        ledsDirty = true;  // Mark as dirty when brightness changes
    }
}

void SQUIDHID::rainbowLEDs(uint16_t first_hue, int8_t reps, uint8_t saturation, uint8_t brightness, bool gammify) {
    if (leds) {
        leds->rainbow(first_hue, reps, saturation, brightness, gammify);
        ledsDirty = true;  // Mark as dirty when rainbow pattern changes
    }
}

bool SQUIDHID::ledsCanShow() {
    return leds ? leds->canShow() : false;
}

void SQUIDHID::updateLEDs() {
    if (ledsDirty && leds && leds->canShow()) { // LEDs only update when dirty
        leds->show();
        ledsDirty = false;
    }
}
#endif

//
// ----------------------------------------- OLED Block
//

#if OLED_ENABLE
void SQUIDHID::initializeOLED(uint8_t sda_pin, uint8_t scl_pin, 
                             uint_fast8_t width, uint_fast8_t height,
                             OLED::tDisplayCtrl displayCtrl, uint8_t i2c_address) 
{
    if (oledDisplay) {
        delete oledDisplay;
    }
    
    uint_fast8_t displayWidth = (width == 0) ? 
                               (OLED_WIDTH > 0 ? OLED_WIDTH : 128) : width;
    uint_fast8_t displayHeight = (height == 0) ? 
                                (OLED_HEIGHT > 0 ? OLED_HEIGHT : 64) : height;
    
    oledDisplay = new OLED(sda_pin, scl_pin, displayWidth, displayHeight, 
                          displayCtrl, i2c_address);
    oledInitialized = false;
    oledDirty = true;  // Mark as dirty on initialization
    
    SQUID_LOG_INFO(LOG_TAG, "OLED driver initialized on SDA:%d SCL:%d, Size: %dx%d", 
                   sda_pin, scl_pin, displayWidth, displayHeight);
}

void SQUIDHID::oledClear(OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->clear(color);
        oledDirty = true;  // Mark as dirty when cleared
    }
}
void SQUIDHID::oledDisplayUpdate() {
    if (oledDisplay && oledInitialized) {
        oledDisplay->display();
        oledDirty = false;  // Clear dirty flag on updates
    }
}

void SQUIDHID::oledSetPower(bool enable) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->set_power(enable);
    }
}

void SQUIDHID::oledSetCursor(uint_fast8_t x, uint_fast8_t y) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->setCursor(x, y);
    }
}

void SQUIDHID::oledDrawString(uint_fast8_t x, uint_fast8_t y, const char* s, OLED::tFontScaling scaling, OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->draw_string(x, y, s, scaling, color);
        oledDirty = true;  // Mark as dirty when drawing stuff
    }
}

void SQUIDHID::oledDrawString_P(uint_fast8_t x, uint_fast8_t y, const char* s, OLED::tFontScaling scaling, OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->draw_string_P(x, y, s, scaling, color);
        oledDirty = true;  // Mark as dirty when drawing stuff
    }
}

size_t SQUIDHID::oledPrintf(uint_fast8_t x, uint_fast8_t y, const char *format, ...) {
    if (!oledDisplay || !oledInitialized) return 0;
    
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) return 0;
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    
    oledDisplay->setCursor(x, y);
    len = oledDisplay->write((const uint8_t*) buffer, len);
    
    if (buffer != temp) delete[] buffer;
    return len;
}

size_t SQUIDHID::oledPrintf(const char *format, ...) {
    if (!oledDisplay || !oledInitialized) return 0;
    
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) return 0;
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    
    len = oledDisplay->write((const uint8_t*) buffer, len);
    oledDirty = true;  // Mark as dirty when drawing stuff
    if (buffer != temp) delete[] buffer;
    return len;
}

void SQUIDHID::oledSetTTYMode(bool enabled) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->setTTYMode(enabled);
    }
}

void SQUIDHID::oledDrawBitmap(uint_fast8_t x, uint_fast8_t y, uint_fast8_t width, uint_fast8_t height, const uint8_t* data, OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->draw_bitmap(x, y, width, height, data, color);
        oledDirty = true;  // Mark as dirty when drawing stuff
    }
}

void SQUIDHID::oledDrawBitmap_P(uint_fast8_t x, uint_fast8_t y, uint_fast8_t width, uint_fast8_t height, const uint8_t* data, OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->draw_bitmap_P(x, y, width, height, data, color);
        oledDirty = true;  // Mark as dirty when drawing stuff
    }
}

void SQUIDHID::oledShowSquidLogo(OLED::tColor color) {
    if (!oledDisplay || !oledInitialized) return;
    
    oledDisplay->clear(OLED::BLACK);
    
    // "Get" display dimensions (just read them off of the config.h file)
    uint_fast8_t displayWidth = OLED_WIDTH;
    uint_fast8_t displayHeight = OLED_HEIGHT;
    
    // Center the 64x64 squid logo
    if (displayWidth >= 64 && displayHeight >= 64) {
        uint_fast8_t x_pos = (displayWidth - 64) / 2;
        uint_fast8_t y_pos = (displayHeight - 64) / 2;
        oledDisplay->draw_bitmap_P(x_pos, y_pos, 64, 64, simple_squid, color);
    }
    
    oledDisplay->display();
    oledDirty = false;  // Clear dirty flag whenever it finishes displaying something
}

void SQUIDHID::oledShowConnectionStatus(bool connected) {
    if (!oledDisplay || !oledInitialized) return;
    
    oledDisplay->clear(OLED::BLACK);
    
    if (connected) {
        oledDisplay->draw_string(20, 0, "Connected", OLED::NORMAL_SIZE, OLED::WHITE);
        oledDisplay->draw_string(10, 16, deviceName.c_str(), OLED::NORMAL_SIZE, OLED::WHITE);
    } else {
        oledDisplay->draw_string(15, 0, "Disconnected", OLED::NORMAL_SIZE, OLED::WHITE);
        oledDisplay->draw_string(10, 16, "Advertising...", OLED::NORMAL_SIZE, OLED::WHITE);
    }
    
    // Show battery level
    char battStr[20];
    snprintf(battStr, sizeof(battStr), "Battery: %d%%", batteryLevel);
    oledDisplay->draw_string(10, 32, battStr, OLED::NORMAL_SIZE, OLED::WHITE);
    
    oledDisplay->display();
    oledDirty = false;  // Clear dirty flag whenever it finishes displaying something
}

void SQUIDHID::oledShowBatteryLevel(uint8_t level) {
    if (!oledDisplay || !oledInitialized) return;
    
    char battStr[20];
    snprintf(battStr, sizeof(battStr), "Battery: %d%%", level);
    
    oledDisplay->draw_string(10, 32, battStr, OLED::NORMAL_SIZE, OLED::WHITE);
    oledDisplay->display();
    oledDirty = false;  // Clear dirty flag whenever it finishes displaying something
}

void SQUIDHID::oledShowLayerInfo(uint8_t layer) {
    if (!oledDisplay || !oledInitialized) return;
    
    char layerStr[20];
    snprintf(layerStr, sizeof(layerStr), "Layer: %d", layer);
    
    oledDisplay->draw_string(10, 48, layerStr, OLED::NORMAL_SIZE, OLED::WHITE);
    oledDisplay->display();
    oledDirty = false;  // Clear dirty flag whenever it finishes displaying something
}
#endif

//
// ----------------------------------------- Logger Block
//

void SQUIDHID::setLogLevel(LogLevel level) { SQUIDLOGS::getInstance().setLogLevel(level); }

LogLevel SQUIDHID::getLogLevel() const { return SQUIDLOGS::getInstance().getLogLevel(); }

void SQUIDHID::initialize(std::function<void(const LogEntry&)> handler) { SQUIDLOGS::getInstance().initialize(handler); }

void SQUIDHID::log(LogLevel level, const std::string& tag, const std::string& message) { SQUIDLOGS::getInstance().log(level, tag, message); }

void SQUIDHID::processQueue() { SQUIDLOGS::getInstance().processQueue(); }

void SQUIDHID::flush() { SQUIDLOGS::getInstance().flush(); }

void SQUIDHID::setMaxQueueSize(uint32_t size) { SQUIDLOGS::getInstance().setMaxQueueSize(size); }

size_t SQUIDHID::getQueueSize() const { return SQUIDLOGS::getInstance().getQueueSize(); }

bool SQUIDHID::isInitialized() const { return SQUIDLOGS::getInstance().isInitialized(); }

bool SQUIDHID::isQueueEmpty() const { return SQUIDLOGS::getInstance().isQueueEmpty(); }
