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
  // Key array (6 bytes for boot compatibility)
  REPORT_COUNT(1),    0x06,                      REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x65,                   
  USAGE_PAGE(1),      0x07,                      USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,                      HIDINPUT(1),        0x00,                   
  END_COLLECTION(0),
};

const size_t       descriptorSize = sizeof(_basicReportDescriptor) 
  
#if KEYBOARD_ENABLE
  +  sizeof(_nkroReportDescriptor)  
#endif

#if MEDIA_ENABLE
  +  sizeof(_mediakeyReportDescriptor)
#endif

#if MOUSE_ENABLE
  +  sizeof(_mouseReportDescriptor)
#endif

#if DIGITIZER_ENABLE
  +  sizeof(_digitizerReportDescriptor)
#endif

#if GAMEPAD_ENABLE
  +  sizeof(_gamepadReportDescriptor)
#endif

#if STENO_ENABLE
  +  sizeof(_stenoReportDescriptor)
#endif
  ;                      
                        
static uint8_t     _hidReportDescriptor[descriptorSize];
static const char* LOG_TAG = "SQUIDHID";
static             SQUIDHID* _activeSQUIDHIDInstance = nullptr;
bool               getInitialized = false;

class HIDDescriptorInitializer {
public:
    HIDDescriptorInitializer() {
        uint8_t* current = _hidReportDescriptor;
        
        // Basic keyboard descriptor
        memcpy(current, _basicReportDescriptor, sizeof(_basicReportDescriptor));
        current += sizeof(_basicReportDescriptor);
        
        #if KEYBOARD_ENABLE
        // NKRO descriptor
        memcpy(current, _nkroReportDescriptor, sizeof(_nkroReportDescriptor));
        current += sizeof(_nkroReportDescriptor);
        #endif
        
        #if MEDIA_ENABLE
        // Media keys descriptor
        memcpy(current, _mediakeyReportDescriptor, sizeof(_mediakeyReportDescriptor));
        current += sizeof(_mediakeyReportDescriptor);
        #endif
        
        #if MOUSE_ENABLE
        // Mouse descriptor
        memcpy(current, _mouseReportDescriptor, sizeof(_mouseReportDescriptor));
        current += sizeof(_mouseReportDescriptor);
        #endif
        
        #if DIGITIZER_ENABLE
        // Digitizer descriptor
        memcpy(current, _digitizerReportDescriptor, sizeof(_digitizerReportDescriptor));
        current += sizeof(_digitizerReportDescriptor);
        #endif
        
        #if GAMEPAD_ENABLE
        // Gamepad descriptor
        memcpy(current, _gamepadReportDescriptor, sizeof(_gamepadReportDescriptor));
        current += sizeof(_gamepadReportDescriptor);
        #endif
        
        #if STENO_ENABLE
        // Plover HID descriptor
        memcpy(current, _stenoReportDescriptor, sizeof(_stenoReportDescriptor));
        current += sizeof(_stenoReportDescriptor);
        #endif
        
        // Debug: Print the complete descriptor
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
    // Create appropriate transport based on type
    switch (type) {
      #if TRANSPORT == USB
        case TransportType::USB:
            transport = std::make_unique<USBTransport>();
      #endif
    
      #if TRANSPORT == PS2
        case TransportType::PS2:
            transport = std::make_unique<PS2Transport>();
      #endif
      
      #if TRANSPORT == BLE
        case TransportType::BLE:
            transport = std::make_unique<BLETransport>();
      #endif
        default:
            transport = std::make_unique<BLETransport>();
            break;
    }
    
    // Set device info and callbacks
    transport->setDeviceInfo(deviceName.c_str(), deviceManufacturer.c_str(), vid, pid, version);
    transport->setBatteryLevel(batteryLevel);
    transport->setAppearance(appearance);
    
    transport->setCallbacks(this);
    
    SQUIDLOGS::getInstance().initialize(); 
    _activeSQUIDHIDInstance = this;
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID instance created with transport layer");
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
    
    if (_activeSQUIDHIDInstance == this) {
        _activeSQUIDHIDInstance = nullptr;
    }
}

void SQUIDHID::begin(const squid_matrix& matrix, const squid_map& keymap) {
    // Call the original begin to initialize transport
    begin();
    
    // Setup matrix and keymap
    setupMatrix(matrix);
    setupKeymap(keymap);
    
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID started with matrix and keymap");
}

void SQUIDHID::begin(void) {
    if (!transport) {
        SQUID_LOG_ERROR(LOG_TAG, "No transport configured");
        return;
    }
    
    SQUID_LOG_INFO(LOG_TAG, "Starting SQUIDHID with transport...");
    
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
        // Auto-initialize OLED with pins from config.h
        initializeOLED(SDA_PIN, SCL_PIN);
        SQUID_LOG_INFO(LOG_TAG, "Auto-initialized OLED with SDA:%d, SCL:%d", SDA_PIN, SCL_PIN);
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
    
    // Set HID report map before initializing transport
    SQUID_LOG_DEBUG(LOG_TAG, "Setting HID report map...");
    transport->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    
    // Initialize transport (this will create services with the report map hopefully ffs please work)
    SQUID_LOG_DEBUG(LOG_TAG, "Initializing transport...");
    if (!transport->begin()) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to initialize transport");
        return;
    }
    
    // Start advertising
    SQUID_LOG_DEBUG(LOG_TAG, "Starting advertising...");
    if (!transport->connect()) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to start advertising");
        return;
    }
    
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
    static uint32_t lastLEDTime           = 0;
    static uint32_t lastOLEDTime          = 0;
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
    
    if (currentTime - lastPollTime >= POLL_INTERVAL) {
        lastPollTime = currentTime;
        pollConnection();
    }
    
    #if LED_ENABLE
    if (currentTime - lastLEDTime >= LED_INTERVAL) {
        lastLEDTime = currentTime;
        if (leds && leds->canShow()) {
        leds->show();
        }
    }
    #endif
    
    #if OLED_ENABLE
    // Update OLED less frequently to avoid flickering
    if (currentTime - lastOLEDTime >= 100) { // Update every 100ms
        lastOLEDTime = currentTime;
        // Could add periodic status updates here if needed
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

void SQUIDHID::onConnect() {
    SQUID_LOG_INFO(LOG_TAG, "Transport connected");
    
    // Notify feature modules about connection
    #if KEYBOARD_ENABLE
    nkro.onConnect();
    #endif
    #if MEDIA_ENABLE
    media.onConnect();
    #endif
    #if MOUSE_ENABLE
    mouse.onConnect();
    #endif
    #if DIGITIZER_ENABLE
    digitizer.onConnect();
    #endif
    #if GAMEPAD_ENABLE
    gamepad.onConnect();
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
    #if MOUSE_ENABLE
    mouse.onDisconnect();
    #endif
    #if DIGITIZER_ENABLE
    digitizer.onDisconnect();
    #endif
    #if GAMEPAD_ENABLE
    gamepad.onDisconnect();
    #endif
    #if STENO_ENABLE
    steno.onDisconnect();
    #endif

}

size_t SQUIDHID::write(const uint8_t *buf, size_t len) {
    size_t n = 0;
    while (len--) n += write(*buf++);
    return n;
}

void SQUIDHID::releaseAll() {
  #if KEYBOARD_ENABLE
  nkro.releaseAll();
  #endif
  
  #if MEDIA_ENABLE
  media.releaseAll();
  #endif
  
  #if MOUSE_ENABLE
  mouse.releaseAll();
  #endif
  
  #if STENO_ENABLE
  steno.releaseAll();
  #endif
  
  #if GAMEPAD_ENABLE
  gamepad.releaseAll();
  #endif
}

void SQUIDHID::onDataReceived(const uint8_t* data, size_t length) {
    SQUID_LOG_DEBUG(LOG_TAG, "Received %zu bytes from transport", length);
    // Handle incoming data, like HID output reports, serial data, and other stuff I never implemented
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
    
    #if DIGITIZER_ENABLE
    digitizer.setAppearance(newAppearance);
    #endif
  
    #if DIGITIZER_ENABLE && MOUSE_ENABLE
    SQUID_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X, Mode: %s", newAppearance, digitizer.isAbsoluteMode() ? "absolute" : "relative");
    #elif !DIGITIZER_ENABLE && MOUSE_ENABLE
    SQUID_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X (Mouse only)", newAppearance);
    #elif DIGITIZER_ENABLE && !MOUSE_ENABLE
    SQUID_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X (Digitizer only)", newAppearance);
    #else
    SQUID_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X", newAppearance);
    #endif
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
    
    this->matrix.begin(matrix, key_event_callback);
    SQUID_LOG_INFO(LOG_TAG, "Keyboard matrix configured with %zu switches", matrix.size());
}

void SQUIDHID::setupKeymap(const squid_map& keymap) {
    auto press_callback = [this](const KeymapEntry& key_entry) {
        switch (key_entry.type) {
            case KeypressType::NKRO_KEY:
                this->nkro.press(key_entry.key.nkro_key);
                break;
            case KeypressType::MOD_KEY:
                this->nkro.press(key_entry.key.mod_key);
                break;
            case KeypressType::MEDIA_KEY:
                #if MEDIA_ENABLE
                this->media.press(key_entry.key.media_key);
                #endif
                break;
            case KeypressType::STENO_KEY:
                #if STENO_ENABLE
                this->steno.press(key_entry.key.steno_key);
                #endif
                break;
            case KeypressType::GAMEPAD_BUTTON:
                #if GAMEPAD_ENABLE
                this->gamepad.press(key_entry.key.gamepad_button);
                #endif
                break;
            case KeypressType::MOUSE_KEY:
                #if MOUSE_ENABLE
                this->mouse.press(key_entry.key.mouse_key);
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
            case KeypressType::MEDIA_KEY:
                #if MEDIA_ENABLE
                this->media.release(key_entry.key.media_key);
                #endif
                break;
            case KeypressType::STENO_KEY:
                #if STENO_ENABLE
                this->steno.release(key_entry.key.steno_key);
                #endif
                break;
            case KeypressType::GAMEPAD_BUTTON:
                #if GAMEPAD_ENABLE
                this->gamepad.release(key_entry.key.gamepad_button);
                #endif
                break;
            case KeypressType::MOUSE_KEY:
                #if MOUSE_ENABLE
                this->mouse.release(key_entry.key.mouse_key);
                #endif
                break;
            default:
                break;
        }
    };
    
    this->keymap.begin(keymap, press_callback, release_callback);
    SQUID_LOG_INFO(LOG_TAG, "Keymap configured");
}

void SQUIDHID::updateMatrix() {
    this->matrix.update();
}

bool SQUIDHID::isKeyPressed(size_t switch_index) {
    return this->matrix.isPressed(switch_index);
}

//
// ----------------------------------------- NKRO Keyboard Block
//

#if KEYBOARD_ENABLE
size_t SQUIDHID::press(NKROKey k) { return nkro.press(k); }

size_t SQUIDHID::press(ModKey modifier) { return nkro.press(modifier); }

size_t SQUIDHID::release(NKROKey k) { return nkro.release(k); }

size_t SQUIDHID::release(ModKey modifier) { return nkro.release(modifier); }

size_t SQUIDHID::write(uint8_t c) { return nkro.write(c); }

size_t SQUIDHID::write(ModKey modifier) { return nkro.write(modifier); }

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
// ----------------------------------------- Mouse Block
//

#if MOUSE_ENABLE
size_t SQUIDHID::press(MouseKey b) { return mouse.press(b); }

size_t SQUIDHID::release(MouseKey b) { return mouse.release(b); }

void SQUIDHID::click(MouseKey b) { mouse.click(b); }

void SQUIDHID::move(signed char x, signed char y, signed char wheel, signed char hWheel) { mouse.move(x, y, wheel, hWheel); }

bool SQUIDHID::mouseIsPressed(MouseKey b) { return mouse.mouseIsPressed(b); }
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

void SQUIDHID::useAbsoluteMode(bool state) { digitizer.useAbsoluteMode(state); }

bool SQUIDHID::isAbsoluteMode() { return digitizer.isAbsoluteMode(); }

void SQUIDHID::useAutoMode(bool state) { digitizer.useAutoMode(state); }

void SQUIDHID::setDigitizerRange(uint16_t maxX, uint16_t maxY) { digitizer.setDigitizerRange(maxX, maxY); }

bool SQUIDHID::isAutoModeEnabled() { return digitizer.isAutoModeEnabled(); }

void SQUIDHID::sendDigitizerReport() { digitizer.sendDigitizerReport(); }
#endif

//
// ----------------------------------------- GeminiPR Stenotype Block
//

#if STENO_ENABLE
size_t SQUIDHID::press(StenoKey stenoKey) { return steno.press(stenoKey); }

size_t SQUIDHID::release(StenoKey stenoKey) { return steno.release(stenoKey); }

void SQUIDHID::stenoStroke(const StenoKey* keys, size_t count) { steno.stenoStroke(keys, count); }

void SQUIDHID::sendStenoReport() { steno.sendStenoReport(); }
#endif

//
// ----------------------------------------- Gamepad Block
//

#if GAMEPAD_ENABLE
size_t SQUIDHID::press(GamepadButton button) { return gamepad.press(button); }

size_t SQUIDHID::release(GamepadButton button) { return gamepad.release(button); }

bool SQUIDHID::gamepadIsPressed(GamepadButton button) { return gamepad.gamepadIsPressed(button); }

void SQUIDHID::gamepadSetLeftStick(int16_t x, int16_t y) { gamepad.gamepadSetLeftStick(x, y); }

void SQUIDHID::gamepadSetRightStick(int16_t x, int16_t y) { gamepad.gamepadSetRightStick(x, y); }

void SQUIDHID::gamepadSetTriggers(int16_t left, int16_t right) { gamepad.gamepadSetTriggers(left, right); }

void SQUIDHID::gamepadGetLeftStick(int16_t &x, int16_t &y) { gamepad.gamepadGetLeftStick(x, y); }

void SQUIDHID::gamepadGetRightStick(int16_t &x, int16_t &y) { gamepad.gamepadGetRightStick(x, y); }

void SQUIDHID::gamepadSetAxis(GamepadAnalogue axis, int16_t value) { gamepad.gamepadSetAxis(axis, value); }

int16_t SQUIDHID::gamepadGetAxis(GamepadAnalogue axis) { return gamepad.gamepadGetAxis(axis); }

void SQUIDHID::gamepadSetAllAxes(int16_t values[GAMEPAD_ANALOGUE_COUNT]) { gamepad.gamepadSetAllAxes(values); }

void SQUIDHID::sendGamepadReport() { gamepad.sendGamepadReport(); }
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
    
    leds = new NeoPixel(count, pin, type);
    
    SQUID_LOG_INFO(LOG_TAG, "LED driver initialized for %d LEDs on pin %d", count, pin);
}

void SQUIDHID::setLEDColor(uint16_t index, uint32_t color) {
    if (leds && index < ledCount) {
        leds->setPixelColor(index, color);
    }
}

void SQUIDHID::setLEDColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (leds && index < ledCount) {
        leds->setPixelColor(index, leds->Color(r, g, b));
    }
}

void SQUIDHID::fillLEDs(uint8_t r, uint8_t g, uint8_t b) {
    if (leds) {
        uint32_t color = leds->Color(r, g, b);
        leds->fill(color);
    }
}

void SQUIDHID::fillLEDsRGB(uint8_t r, uint8_t g, uint8_t b, uint16_t first, uint16_t count) {
    if (leds) {
        uint32_t color = leds->Color(r, g, b);
        leds->fill(color, first, count);
    }
}

void SQUIDHID::clearLEDs() {
    if (leds) {
        leds->clear();
    }
}

void SQUIDHID::showLEDs() {
    if (leds && leds->canShow()) {
        leds->show();
    }
}

void SQUIDHID::setLEDBrightness(uint8_t brightness) {
    if (leds) {
        leds->setBrightness(brightness);
    }
}

void SQUIDHID::rainbowLEDs(uint16_t first_hue, int8_t reps, uint8_t saturation, uint8_t brightness, bool gammify) {
    if (leds) {
        leds->rainbow(first_hue, reps, saturation, brightness, gammify);
    }
}

bool SQUIDHID::ledsCanShow() {
    return leds ? leds->canShow() : false;
}
#endif

//
// ----------------------------------------- OLED Block
//

#if OLED_ENABLE
void SQUIDHID::initializeOLED(uint8_t sda_pin, uint8_t scl_pin, OLED::tDisplayCtrl displayCtrl, uint8_t i2c_address) {
    if (oledDisplay) {
        delete oledDisplay;
    }
    
    oledDisplay = new OLED(sda_pin, scl_pin, displayCtrl, i2c_address);
    oledInitialized = false;
    
    SQUID_LOG_INFO(LOG_TAG, "OLED driver initialized on SDA:%d SCL:%d", sda_pin, scl_pin);
}

void SQUIDHID::oledClear(OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->clear(color);
    }
}

void SQUIDHID::oledDisplayUpdate() {
    if (oledDisplay && oledInitialized) {
        oledDisplay->display();
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
    }
}

void SQUIDHID::oledDrawString_P(uint_fast8_t x, uint_fast8_t y, const char* s, OLED::tFontScaling scaling, OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->draw_string_P(x, y, s, scaling, color);
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
    }
}

void SQUIDHID::oledDrawBitmap_P(uint_fast8_t x, uint_fast8_t y, uint_fast8_t width, uint_fast8_t height, const uint8_t* data, OLED::tColor color) {
    if (oledDisplay && oledInitialized) {
        oledDisplay->draw_bitmap_P(x, y, width, height, data, color);
    }
}

void SQUIDHID::oledShowSquidLogo(OLED::tColor color) {
    if (!oledDisplay || !oledInitialized) return;
    
    // Clear display
    oledDisplay->clear(OLED::BLACK);
    
    // Center the 64x64 squid logo on a 128x64 display
    // Calculate x position to center: (128 - 64) / 2 = 32
    // y position: 0 (top of screen)
    oledDisplay->draw_bitmap_P(32, 0, 64, 64, simple_squid, color);
    
    // Add text below the logo
    oledDisplay->draw_string(40, 56, "SquidHID", OLED::NORMAL_SIZE, color);
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
}

void SQUIDHID::oledShowBatteryLevel(uint8_t level) {
    if (!oledDisplay || !oledInitialized) return;
    
    char battStr[20];
    snprintf(battStr, sizeof(battStr), "Battery: %d%%", level);
    
    oledDisplay->draw_string(10, 32, battStr, OLED::NORMAL_SIZE, OLED::WHITE);
    oledDisplay->display();
}

void SQUIDHID::oledShowLayerInfo(uint8_t layer) {
    if (!oledDisplay || !oledInitialized) return;
    
    char layerStr[20];
    snprintf(layerStr, sizeof(layerStr), "Layer: %d", layer);
    
    oledDisplay->draw_string(10, 48, layerStr, OLED::NORMAL_SIZE, OLED::WHITE);
    oledDisplay->display();
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

// Platform-specific control methods
#if defined(SQUIDHID_PLATFORM_ESP32)
void SQUIDHID::setESP32LogLevel(esp_log_level_t level) { SQUIDLOGS::getInstance().setESP32LogLevel(level); }
#elif defined(SQUIDHID_PLATFORM_NRF52)
void SQUIDHID::setNRF52LogLevel(nrf_log_severity_t severity) { SQUIDLOGS::getInstance().setNRF52LogLevel(severity); }
#endif
