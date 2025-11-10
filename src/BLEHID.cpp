#include "BLEHID.h"

#if GEMINIPR_ENABLE
// SPP UUIDs (Standard Serial Port Service)
const char* BLEHID::SERIAL_SERVICE_UUID = "00001101-0000-1000-8000-00805f9b34fb";
const char* BLEHID::SERIAL_CHARACTERISTIC_UUID_TX = "00001102-0000-1000-8000-00805f9b34fb";
const char* BLEHID::SERIAL_CHARACTERISTIC_UUID_RX = "00001103-0000-1000-8000-00805f9b34fb";
#endif

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
  // LED output report
  REPORT_COUNT(1),    0x05,                      REPORT_SIZE(1),     0x01,
  USAGE_PAGE(1),      0x08,                      USAGE_MINIMUM(1),   0x01,                   
  USAGE_MAXIMUM(1),   0x05,                      HIDOUTPUT(1),       0x02,                   
  REPORT_COUNT(1),    0x01,                      REPORT_SIZE(1),     0x03,
  HIDOUTPUT(1),       0x01,                   
  // Key array (6 bytes for boot compatibility)
  REPORT_COUNT(1),    0x06,                      REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,                      LOGICAL_MAXIMUM(1), 0x65,                   
  USAGE_PAGE(1),      0x07,                      USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,                      HIDINPUT(1),        0x00,                   
  END_COLLECTION(0),
};

const size_t       descriptorSize = 0

  +  sizeof(_basicReportDescriptor)  
  
#if KEYBOARD_ENABLE
  +  sizeof(_nkroReportDescriptor)  
#endif

#if MEDIA_ENABLE
  +  sizeof(_mediakeyReportDescriptor)
#endif

#if POINTER_ENABLE
  +  sizeof(_mouseReportDescriptor)
  +  sizeof(_digitizerReportDescriptor)
#endif

#if GAMEPAD_ENABLE
  +  sizeof(_gamepadReportDescriptor)
#endif
  ;                      
                        
static uint8_t     _hidReportDescriptor[descriptorSize];
static const char* LOG_TAG = "BLEHID";
static             BLEHID* _activeBLEHIDInstance = nullptr;
void               pollConnection(void * arg);
bool               getInitialized = false;

class HIDDescriptorInitializer {
public:
    HIDDescriptorInitializer() {
        size_t currentPosition = 0;
        memcpy(_hidReportDescriptor + currentPosition, _basicReportDescriptor, sizeof(_basicReportDescriptor));
        currentPosition += sizeof(_basicReportDescriptor);
        #if KEYBOARD_ENABLE
        memcpy(_hidReportDescriptor + currentPosition, _nkroReportDescriptor, sizeof(_nkroReportDescriptor));
        currentPosition += sizeof(_nkroReportDescriptor);
        #endif
        
        #if MEDIA_ENABLE
        memcpy(_hidReportDescriptor + currentPosition, _mediakeyReportDescriptor, sizeof(_mediakeyReportDescriptor));
        currentPosition += sizeof(_mediakeyReportDescriptor);
        #endif
        
        #if POINTER_ENABLE
        memcpy(_hidReportDescriptor + currentPosition, _mouseReportDescriptor, sizeof(_mouseReportDescriptor));
        currentPosition += sizeof(_mouseReportDescriptor);
        
        memcpy(_hidReportDescriptor + currentPosition, _digitizerReportDescriptor, sizeof(_digitizerReportDescriptor));
        currentPosition += sizeof(_digitizerReportDescriptor);
        #endif
        
        #if GAMEPAD_ENABLE
        memcpy(_hidReportDescriptor + currentPosition, _gamepadReportDescriptor, sizeof(_gamepadReportDescriptor));
        currentPosition += sizeof(_gamepadReportDescriptor);
        #endif
    }
};

static HIDDescriptorInitializer _hidDescriptorInitializer;

// This is a "constructor". It takes that class from the BLEHID.h file, and turns it into "objects" that can actually be used.
BLEHID::BLEHID(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) 
    #if GEMINIPR_ENABLE
    , serialService(nullptr)
    , serialInput(nullptr)
    , serialOutput(nullptr)
    , serialOutputDescriptor(nullptr)
    , serialConnected(false)
    #endif
    #if KEYBOARD_ENABLE
    , _useNKRO(true)
    #endif
    #if MEDIA_ENABLE
    , _mediaKeyBitmask(0) 
    #endif
    #if POINTER_ENABLE
    , _mouseButtons(0)
    , _useAbsolute(false) 
    , _autoMode(true)
    , _screenWidth(1920)
    , _screenHeight(1080)
    , _digitizerConfigured(false)
    #endif
    , lastPollTime(0) 
{
  // Initialize reports
  #if KEYBOARD_ENABLE
  memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
  #endif
  
  #if POINTER_ENABLE
  memset(&_pointerReport, 0, sizeof(_pointerReport));
  memset(&_digitizerReport, 0, sizeof(_digitizerReport));
  #endif
  
  #if GEMINIPR_ENABLE
  memset(&_geminiReport, 0, sizeof(_geminiReport));
  #endif
  
  #if GAMEPAD_ENABLE
  memset(&_gamepadReport, 0, sizeof(_gamepadReport));
  _gamepadReport.hat = HAT_CE;
  #endif
  
  _activeBLEHIDInstance = this;
}
// This is a "destructor". It takes objects the contructor made, and destroys them whenever you tell it to. 
BLEHID::~BLEHID() { 
    // Unregister this instance
    if (_activeBLEHIDInstance == this) {
        _activeBLEHIDInstance = nullptr;
    }
    
    #if GEMINIPR_ENABLE
    // Clean up SPP resources
    if (serialOutputDescriptor) {
        delete serialOutputDescriptor;
        serialOutputDescriptor = nullptr;
    } // Remember, serialService, serialInput, serialOutput are managed by BLE stack so they don't go here
    #endif
}

void BLEHID::begin(void) {
    // Initialise BLE stack only once
    if (getInitialized) {
        Serial.printf("[%s] BLE already initialized, cleaning up first...\n", LOG_TAG);
        end();
        delay(100);
    } else { NimBLEDevice::init(deviceName.c_str()); }
    
    // Configure security if enabled
    if (isSecurityEnabled()) {
        // Enhanced security configuration for bonding
        NimBLEDevice::setSecurityAuth(true, true, true); // Bonding, MITM, Secure Connections enabled
        NimBLEDevice::setSecurityPasskey(passkey);       // Actually set a passkey
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
        
        // Configure bonding parameters
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        
        Serial.printf("[%s] Security configured with PIN: %06lu \n", LOG_TAG, passkey);
    } else {
        NimBLEDevice::setSecurityAuth(false, false, false); // Bonding, MITM, Secure Connections disabled
        Serial.printf("[%s] Just Works simple pairing enabled\n", LOG_TAG);
    }
    
    // Create server & install callbacks
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(this);

    // Create HID device object
    hid = new NimBLEHIDDevice(pServer);

    // Obtain report-characteristic pointers
    outputKeyboard = hid->getOutputReport(KEYBOARD_ID);
    #if KEYBOARD_ENABLE
    inputNKRO      = hid->getInputReport(NKRO_ID);
    #endif
    
    #if MEDIA_ENABLE
    inputMediaKeys = hid->getInputReport(MEDIA_KEYS_ID);
    #endif
    
    #if POINTER_ENABLE
    inputMouse     = hid->getInputReport(MOUSE_ID);
    inputDigitizer = hid->getInputReport(DIGITIZER_ID);
    #endif
    
    #if GEMINIPR_ENABLE
      serialService = pServer->createService(SERIAL_SERVICE_UUID);

      // Create TX Characteristic (device -> client)
      serialInput = serialService->createCharacteristic(SERIAL_CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);

      // Create RX Characteristic (client -> device)  
      serialOutput = serialService->createCharacteristic(SERIAL_CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    #endif
    
    #if GAMEPAD_ENABLE
    inputGamepad   = hid->getInputReport(GAMEPAD_ID);
    #endif
    
    // Set callbacks
    outputKeyboard->setCallbacks(this);
    #if KEYBOARD_ENABLE
    if (inputNKRO) {inputNKRO->setCallbacks(this);}
    #endif
    
    #if MEDIA_ENABLE
    if (inputMediaKeys) {inputMediaKeys->setCallbacks(this);}
    #endif
    
    #if POINTER_ENABLE
    if (inputMouse) {inputMouse->setCallbacks(this);}
    if (inputDigitizer) {inputDigitizer->setCallbacks(this);}
    #endif
    
    #if GEMINIPR_ENABLE
    serialOutput->setCallbacks(this);
    #endif
    
    #if GAMEPAD_ENABLE
    if (inputGamepad) {inputGamepad->setCallbacks(this);}
    #endif
    
    // Manufacturer / PnP / HID-info
    hid->setManufacturer(std::string(deviceManufacturer.c_str()));
    hid->setHidInfo(0x11, 0x01);

    // Publish HID report map and start services
    hid->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->startServices();
    
    #if GEMINIPR_ENABLE
    // Add CCCD descriptor for TX characteristic
    serialOutputDescriptor = new BLE2904();
    serialInput->addDescriptor(serialOutputDescriptor);
    
    // Start the serial service
    serialService->start();
    #endif
    
    // Create complete Device Information Service
    BLEService* disService = pServer->createService(NimBLEUUID((uint16_t)0x180A));

    // Manufacturer Name
    BLECharacteristic* manufChar = disService->createCharacteristic(
      (uint16_t)0x2A29, 
      NIMBLE_PROPERTY::READ
    );
    manufChar->setValue(deviceManufacturer.c_str());

    // Model Number
    BLECharacteristic* modelChar = disService->createCharacteristic(
      (uint16_t)0x2A24,
      NIMBLE_PROPERTY::READ
    );
    modelChar->setValue(deviceName.c_str());

    // Firmware Version
    BLECharacteristic* fwChar = disService->createCharacteristic(
      (uint16_t)0x2A26,
      NIMBLE_PROPERTY::READ
    );
    fwChar->setValue(BLE_KEYBOARD_VERSION);

    // PnP ID (required)
    BLECharacteristic* pnpChar = disService->createCharacteristic(
      (uint16_t)0x2A50,
      NIMBLE_PROPERTY::READ
    );

    uint8_t pnpId[7] = {
      0x02,           // Vendor ID source (USB-IF)
      vid & 0xFF, vid >> 8,
      pid & 0xFF, pid >> 8,
      version & 0xFF, version >> 8
    };
    pnpChar->setValue(pnpId, 7);

    disService->start();

    // Advertising setup
    advertising = pServer->getAdvertising();
    BLEAdvertisementData adv, scan;
    
    auto configureAdvertisement = [&](BLEAdvertisementData &ad) {
        ad.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
        ad.setName(deviceName.c_str());
        ad.setShortName(deviceName.substr(0, 8).c_str());
        ad.setAppearance(this->appearance);
        ad.setManufacturerData(deviceManufacturer.c_str());
    };
    
    // Configure both advertisements
    configureAdvertisement(scan);
    configureAdvertisement(adv);
    
    // Add unique elements to each
    scan.addServiceUUID(hid->getHidService()->getUUID());
    adv.setCompleteServices(hid->getHidService()->getUUID());
    adv.addServiceUUID(hid->getHidService()->getUUID());
    
    #if GEMINIPR_ENABLE
      // Add SPP service
      NimBLEUUID serialUUID(SERIAL_SERVICE_UUID);
      scan.addServiceUUID(serialUUID);
      adv.addServiceUUID(serialUUID);
      adv.setCompleteServices(serialUUID);
    #endif
    
    advertising->setMinInterval(40);   // 25ms  (40  * 0.625ms)
    advertising->setMaxInterval(160);  // 100ms (160 * 0.625ms)  
    advertising->setAdvertisementData(adv);
    advertising->setScanResponseData(scan);

    // Start advertising & finish
    onStarted(pServer);
    advertising->start();
    hid->setBatteryLevel(batteryLevel);
    
    lastPollTime = millis();

    Serial.printf("[%s] Advertising started!\n", LOG_TAG);
    Serial.printf("[%s] Device name: %s\n", LOG_TAG, deviceName.c_str());
    Serial.printf("[%s] Service UUID: %s\n", LOG_TAG, hid->getHidService()->getUUID().toString().c_str());
    Serial.printf("[%s] Using %s mode by default\n", LOG_TAG, _useNKRO ? "NKRO" : "6KRO");
    
    getInitialized = true;
}

// Update/polling function
void BLEHID::update() {
  static uint32_t lastUpdateTime = 0;
  uint32_t currentTime = millis();
  
  if (currentTime - lastUpdateTime >= SCAN_INTERVAL) {
    lastUpdateTime = currentTime;
    
    if (_activeBLEHIDInstance) {
      uint32_t currentPollTime = millis();
      
      // Handle millis() rollover
      if (currentPollTime < lastPollTime) {
        lastPollTime = currentPollTime;
        return;
      }
      
      if (currentPollTime - lastPollTime >= POLL_INTERVAL) {
        lastPollTime = currentPollTime;
        pollConnection(this);
      }
    }
  }
}

void BLEHID::end(void) {
  if (hid != 0) {
    delete hid;
    hid = 0;
  }
  BLEDevice::deinit(true);
  getInitialized = false;
  Serial.printf("[%s] BLE Keyboard stopped\n", LOG_TAG);
}

void BLEHID::setAppearance(uint16_t newAppearance) {
  this->appearance = newAppearance;
  #if POINTER_ENABLE
  // If auto-mode is enabled, detect the default pointer mode from appearance
  if (_autoMode) {_detectModeFromAppearance();}
  // Actually inform the user which mode is the default for debugging purposes
  Serial.printf("[%s] Appearance set to: 0x%04X, Mode: %s\n", LOG_TAG, newAppearance, _useAbsolute ? "absolute" : "relative");
  #endif
}

void BLEHID::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}

//must be called before begin in order to set the name
void BLEHID::setName(std::string deviceName) {
  this->deviceName = deviceName;
}

//must be called before begin in order to set the manufacturer
void BLEHID::setManufacturer(std::string deviceManufacturer) {
  this->deviceManufacturer = deviceManufacturer;
}

// Sets the waiting time (in milliseconds) between multiple keystrokes
void BLEHID::setDelay(uint32_t ms) {
  _delay_ms = ms;
}

void BLEHID::setVendorId(uint16_t vid) { 
	this->vid = vid; 
}

void BLEHID::setProductId(uint16_t pid) { 
	this->pid = pid; 
}

void BLEHID::setVersion(uint16_t version) { 
	this->version = version; 
}

//
// ----------------------------------------- NKRO Keyboard Block
//

#if KEYBOARD_ENABLE
size_t BLEHID::press(uint8_t k) {
  if (k >= 136) { k = k - 136; }
  
  if (k != 0) {
    // Check if we're already at 6 non-modifier keys
    if (!_useNKRO && countPressedKeys() >= 6) {
      setWriteError();                
      return 0;
    }
    
    // Update the bitmask - ONLY for regular keys
    updateNKROBitmask(k, true);
  }
  
  sendNKROReport();
  return 1;
}

size_t BLEHID::press(int16_t modifier) {
  uint8_t hidModifier = 0;
  if (modifier >= 0x0100 && modifier <= 0x8000 && ((modifier & (modifier - 1)) == 0)) {
    hidModifier = modifier >> 8;
  } else {
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers |= hidModifier;
  sendNKROReport();
  return 1;
}

size_t BLEHID::release(uint8_t k) {
  if (k >= 136) {
    k = k - 136;
  }
  
  if (k != 0) {
    updateNKROBitmask(k, false);
  }
  
  sendNKROReport();
  return 1;
}

size_t BLEHID::release(int16_t modifier) {
  uint8_t hidModifier = 0;
  if (modifier >= 0x0100 && modifier <= 0x8000 && ((modifier & (modifier - 1)) == 0)) {
    hidModifier = modifier >> 8;
  } else {
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers &= ~hidModifier;
  sendNKROReport();
  return 1;
}

size_t BLEHID::write(uint8_t c) {
    bool shift;
    uint8_t key = BLEHID::charToKeyCode((char)c, &shift);
    if (key == 0) return 0;                     // character not supported

    if (shift) press(KEY_LEFT_SHIFT);           // hold shift
    press(key);                                 // key-down
    release(key);                               // key-up
    if (shift) release(KEY_LEFT_SHIFT);         // release shift
    return 1;
}

size_t BLEHID::write(int16_t modifier) {
  uint16_t p = press(modifier);  // Modifier down
  release(modifier);             // Modifier up
  return p;
}

void BLEHID::useNKRO(bool state) {
  _useNKRO = state; // state = enabled, therefore _useNKRO = true/enabled
  Serial.printf("[%s] Switched to %s mode\n", LOG_TAG, _useNKRO ? "NKRO" : "6KRO");
}

void BLEHID::use6KRO(bool state) {
  _useNKRO = !state; // state = enabled, therefore _useNKRO = not true/enabled = false
  Serial.printf("[%s] Switched to %s mode\n", LOG_TAG, _useNKRO ? "NKRO" : "6KRO");
}

bool BLEHID::isNKROEnabled() {
  return _useNKRO;
}

uint8_t BLEHID::countPressedKeys() {
  uint8_t count = 0;
  // Only count non-modifier keys in the main key area
  // This ensures modifiers don't count toward the 6-key limit
  for (int i = 0; i < (NKRO_KEY_COUNT / 8); i++) {
    uint8_t byte = _keyReportNKRO.keys_bitmask[i];
    count += __builtin_popcount(byte);
  }
  return count;
}

void BLEHID::updateNKROBitmask(uint8_t k, bool pressed) {
  if (k < NKRO_KEY_COUNT) {
    uint8_t bitmaskIndex = k / 8;
    uint8_t bitOffset = k % 8;
    
    if (pressed) {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] |= (1 << bitOffset);
    } else {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] &= ~(1 << bitOffset);
    }
  }
}

void BLEHID::setModifiers(uint8_t modifiers) {
    _keyReportNKRO.modifiers = modifiers;
    sendNKROReport();
}

uint8_t BLEHID::getModifiers() {
    return _keyReportNKRO.modifiers;
}

uint8_t BLEHID::charToKeyCode(char c, bool *needShift) {
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
    case '\n': case '\r':                return KEY_ENTER;          // 0xB0
    case '\t':                           return KEY_TAB;            // 0xB3
    case ' ':                            return KEY_SPACE;          // 0x2C
    case '-': case '_':
        if (c == '_') *needShift = true;
        return KEY_MINUS;                                      // 0x2D
    case '=': case '+':
        if (c == '+') *needShift = true;
        return KEY_EQUAL;                                      // 0x2E
    case '[': case '{':
        if (c == '{') *needShift = true;
        return KEY_LEFT_BRACKET;                               // 0x2F
    case ']': case '}':
        if (c == '}') *needShift = true;
        return KEY_RIGHT_BRACKET;                              // 0x30
    case '\\': case '|':
        if (c == '|') *needShift = true;
        return KEY_BACKSLASH;                                  // 0x31
    case ';': case ':':
        if (c == ':') *needShift = true;
        return KEY_SEMICOLON;                                  // 0x33
    case '\'': case '"':
        if (c == '"') *needShift = true;
        return KEY_APOSTROPHE;                                 // 0x34
    case '`': case '~':
        if (c == '~') *needShift = true;
        return KEY_GRAVE;                                      // 0x35
    case ',': case '<':
        if (c == '<') *needShift = true;
        return KEY_COMMA;                                      // 0x36
    case '.': case '>':
        if (c == '>') *needShift = true;
        return KEY_PERIOD;                                     // 0x37
    case '/': case '?':
        if (c == '?') *needShift = true;
        return KEY_FORWARD_SLASH;                              // 0x38
    default:                             return 0;                // non-printable
    }
}

void BLEHID::sendNKROReport() {
  if (this->isConnected() && inputNKRO) {
    // If in NKRO mode, send the extended report (ID 2)
    if (_useNKRO) {
      inputNKRO->setValue((uint8_t*)&_keyReportNKRO, sizeof(KeyReportNKRO));
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
    }
    inputNKRO->notify();
    delay(_delay_ms);
  }
}
#endif

//
// ----------------------------------------- Media key Block
//

#if MEDIA_ENABLE
size_t BLEHID::press(uint16_t mediaKey) {
    addMediaKey(mediaKey);
    return 1;
}

size_t BLEHID::release(uint16_t mediaKey) {
    removeMediaKey(mediaKey);
    return 1;
}

size_t BLEHID::write(uint16_t mediaKey) {
	uint16_t p = press(mediaKey);  // Keydown
	release(mediaKey);            // Keyup
	return p;
}

void BLEHID::setMediaKeyBitmask(uint32_t bitmask) {
    _mediaKeyBitmask = bitmask;
    sendMediaReport(); // Send the updated bitmask
}

uint32_t BLEHID::getMediaKeyBitmask() {
    return _mediaKeyBitmask;
}

uint32_t BLEHID::mediaKeyToBitmask(uint16_t usageCode) {
    switch (usageCode) {
      case 0x0130: return (1UL << 0);   // System Power
      case 0x0134: return (1UL << 1);   // System Sleep  
      case 0x0135: return (1UL << 2);   // System Wake
      case 0x00B5: return (1UL << 3);   // Next Track
      case 0x00B6: return (1UL << 4);   // Previous Track
      case 0x00B7: return (1UL << 5);   // Stop
      case 0x00CD: return (1UL << 6);   // Play/Pause
      case 0x00B3: return (1UL << 7);   // Fast Forward
      case 0x00B4: return (1UL << 8);   // Rewind
      case 0x00B8: return (1UL << 9);   // Eject
      case 0x00E2: return (1UL << 10);  // Mute
      case 0x00E9: return (1UL << 11);  // Volume Up
      case 0x00EA: return (1UL << 12);  // Volume Down
      case 0x006F: return (1UL << 13);  // Brightness Up
      case 0x0070: return (1UL << 14);  // Brightness Down
      case 0x0194: return (1UL << 15);  // My Computer
      case 0x0192: return (1UL << 16);  // Calculator
      case 0x018A: return (1UL << 17);  // Mail
      case 0x0183: return (1UL << 18);  // Media Selection
      case 0x0186: return (1UL << 19);  // Control Panel
      case 0x0187: return (1UL << 20);  // Launchpad
      case 0x0223: return (1UL << 21);  // WWW Home
      case 0x022A: return (1UL << 22);  // WWW Favorites
      case 0x0221: return (1UL << 23);  // WWW Search
      case 0x0226: return (1UL << 24);  // WWW Stop
      case 0x0224: return (1UL << 25);  // WWW Back
      case 0x0225: return (1UL << 26);  // WWW Forward
      case 0x0227: return (1UL << 27);  // WWW Refresh
      default: return 0;
    }
}

void BLEHID::addMediaKey(uint16_t mediaKey) {
    uint32_t keyBitmask = mediaKeyToBitmask(mediaKey);
    _mediaKeyBitmask |= keyBitmask;
    sendMediaReport();
}

void BLEHID::removeMediaKey(uint16_t mediaKey) {
    uint32_t keyBitmask = mediaKeyToBitmask(mediaKey);
    _mediaKeyBitmask &= ~keyBitmask;
    sendMediaReport();
}

void BLEHID::sendMediaReport() {
  if (this->isConnected()) {
    // Send the current media key bitmask
    inputMediaKeys->setValue((uint8_t*)&_mediaKeyBitmask, sizeof(uint32_t));
    inputMediaKeys->notify();
    delay(_delay_ms);
  }	
}
#endif

//
// ----------------------------------------- Relative/Absolute Pointer Block
//

#if POINTER_ENABLE
size_t BLEHID::press(char b) {
  _mouseButtons |= b;
  _pointerReport.buttons = _mouseButtons;
  move(0, 0, 0, 0);
}

size_t BLEHID::release(char b) {
  _mouseButtons &= ~b;
  _pointerReport.buttons = _mouseButtons;
  move(0, 0, 0, 0);
}

void BLEHID::mouseReleaseAll() {
  _mouseButtons = 0;
  _pointerReport.buttons = 0;
  move(0, 0, 0, 0);
}

void BLEHID::click(char b) {
  press(b);
  delay(_delay_ms);
  release(b);
}

void BLEHID::_detectModeFromAppearance() {
    // If appearance is set to digitizer or tablet, default pointer is the digitizer
    if (this->appearance == DIGITIZER || this->appearance == DIGITAL_PEN || this->appearance == TABLET) {
        _useAbsolute = true;
        Serial.printf("[%s] Auto-detected absolute mode from appearance: 0x%04X\n", LOG_TAG, this->appearance);
    // For all other appearances, default pointer is the mouse
    } else {
        _useAbsolute = false;
        Serial.printf("[%s] Auto-detected relative mode from appearance: 0x%04X\n", LOG_TAG, this->appearance);
    }
}

bool BLEHID::_shouldUseAbsoluteMode() {
    // If the user explicitly says they don't want it to switch by itself, then respect their decision
    if (!_autoMode) {return _useAbsolute;}
    
    // If the digitizer was explicitly configured, use absolute mode
    if (_digitizerConfigured) { return true;}
    
    // If the user would reasonably assume they configured it, make it still work
    if (this->appearance == DIGITIZER || this->appearance == DIGITAL_PEN || this->appearance == TABLET) {return true;}
    
    // Default to relative pointers because having digitizers be the default is definitely gonna confuse people
    return false;
}

void BLEHID::click(uint16_t x, uint16_t y, char b) {
    // Auto-detect mode based on parameters
    bool shouldUseAbsolute = _shouldUseAbsoluteMode();
    
    // If coordinates are provided and we're not in absolute mode, auto-switch
    if (_autoMode && !_useAbsolute && (x != 0 || y != 0)) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        Serial.printf("[%s] Auto-switched to absolute mode due to coordinate click\n", LOG_TAG);
        shouldUseAbsolute = true;
    }
    
    if (shouldUseAbsolute) {
        // Map mouse button constants to digitizer button constants
        uint8_t digitizerButtons = 0;
        if (b & MOUSE_LEFT)   digitizerButtons |= DIGITIZER_BTN1;
        if (b & MOUSE_RIGHT)  digitizerButtons |= DIGITIZER_BTN2;
        if (b & MOUSE_MIDDLE) digitizerButtons |= DIGITIZER_BTN3;
        moveTo(x, y, 127, digitizerButtons); // Press with pressure
        delay(_delay_ms);
        moveTo(x, y, 0, 0);   // Release
    } else {
        // Fall back to relative click
        click(b);
    }
}

void BLEHID::move(signed char x, signed char y, signed char wheel, signed char hWheel) {
  if (this->isConnected() && inputMouse) {
    _pointerReport.buttons = _mouseButtons;
    
    // Set relative fields
    _pointerReport.relX = x;
    _pointerReport.relY = y;
    _pointerReport.wheel = wheel;
    _pointerReport.hWheel = hWheel;
    inputMouse->setValue((uint8_t*)&_pointerReport, sizeof(_pointerReport));
    inputMouse->notify();
    delay(_delay_ms);
  }
}

bool BLEHID::mouseIsPressed(char b) {
  return (_pointerReport.buttons & b) != 0;
}

void BLEHID::useAbsoluteMode(bool state) {
    _useAbsolute = state;
    _autoMode = false; // If they start managing the states manually, just assume they don't want it to switch automatically
    _digitizerConfigured = state; // Digitizers actually get configured when you make digitizers the default pointer mode
    Serial.printf("[%s] %s mode %s (auto-mode disabled)\n", LOG_TAG, state ? "Absolute" : "Relative", state ? "enabled" : "disabled");
}

void BLEHID::useAutoMode(bool state) {
    _autoMode = state;
    if (state) {
        _detectModeFromAppearance(); // Re-detect the default pointer mode when enabling auto-mode
        Serial.printf("[%s] Auto-mode enabled, current mode: %s\n", LOG_TAG, _useAbsolute ? "absolute" : "relative");
    } else {Serial.printf("[%s] Auto-mode disabled\n", LOG_TAG);}
}

void BLEHID::setDigitizerRange(uint16_t maxX, uint16_t maxY) {
    _screenWidth = maxX;
    _screenHeight = maxY;
    _digitizerConfigured = true; // Mark that digitizer was explicitly configured
    
    // If auto-mode is enabled and digitizer is configured, switch to absolute mode
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        Serial.printf("[%s] Auto-switched to absolute mode due to digitizer configuration\n", LOG_TAG);
    }
    
    Serial.printf("[%s] Digitizer range set to X:%u, Y:%u\n", LOG_TAG, _screenWidth, _screenHeight);
}

void BLEHID::moveTo(uint16_t x, uint16_t y, uint8_t pressure, uint8_t buttons) { // "buttons" like the 3 digitizer pen buttons (tip, side/barrel, eraser)
    if (_autoMode && !_useAbsolute) {
        _useAbsolute = true;
        _digitizerConfigured = true;
        Serial.printf("[%s] Auto-switched to absolute mode\n", LOG_TAG);
    }
    
    if (this->isConnected() && inputDigitizer) {
        // Turn on absolute mode if it isn't on already
        if(!_useAbsolute) {_useAbsolute = true;}
        // Scale to HID descriptor's 0-32767 range
        uint16_t scaledX = (x * 32767ULL) / _screenWidth;
        uint16_t scaledY = (y * 32767ULL) / _screenHeight;
        
        _digitizerReport.buttons = buttons & 0x07;  // Mask to 3 bits
        _digitizerReport.x = scaledX;
        _digitizerReport.y = scaledY;
        _digitizerReport.pressure = (pressure > 127) ? 127 : pressure; // Adding pressure automatically counts as the tip switch button beting pressed down
        
        // Set flags: ALWAYS report In Range when active
        _digitizerReport.flags = DIGITIZER_FLAG_IN_RANGE;
        // Tip Switch = ON when pressure > 0 (touching), OFF when hovering
        if (pressure > 0) {
            _digitizerReport.flags |= DIGITIZER_FLAG_TIP_SWITCH;
        }
        
        sendDigitizerReport();
    }
}

void BLEHID::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) {
  moveTo(x, y, initialPressure);
}

void BLEHID::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) {
  moveTo(x, y, pressure);
}

void BLEHID::endStroke(uint16_t x, uint16_t y) {
  moveTo(x, y, 0);
}

void BLEHID::sendDigitizerReport() {
    if (this->isConnected() && inputDigitizer && _useAbsolute) {
        inputDigitizer->setValue((uint8_t*)&_digitizerReport, sizeof(_digitizerReport));
        inputDigitizer->notify();
        delay(_delay_ms);
    }
}

bool BLEHID::isAbsoluteMode() {
    return _shouldUseAbsoluteMode(); // Use the smart detection
}

bool BLEHID::isAutoModeEnabled() {
    return _autoMode;
}
#endif

//
// ----------------------------------------- GeminiPR Stenotype Block
//

#if GEMINIPR_ENABLE
size_t BLEHID::press(int32_t stenoKey) {
  // Set the appropriate bit in the correct byte position
  switch (stenoKey) {
    // Byte 0
    case GEMINI_FN:   _geminiReport.byte0 |= 0x01; break;
    case GEMINI_NUM1: _geminiReport.byte0 |= 0x02; break;
    
    // Byte 1  
    case GEMINI_S1:   _geminiReport.byte1 |= 0x01; break;
    case GEMINI_S2:   _geminiReport.byte1 |= 0x02; break;
    case GEMINI_T:    _geminiReport.byte1 |= 0x04; break;
    case GEMINI_K:    _geminiReport.byte1 |= 0x08; break;
    case GEMINI_P:    _geminiReport.byte1 |= 0x10; break;
    case GEMINI_W:    _geminiReport.byte1 |= 0x20; break;
    case GEMINI_H:    _geminiReport.byte1 |= 0x40; break;
    
    // Byte 2
    case GEMINI_R:    _geminiReport.byte2 |= 0x01; break;
    case GEMINI_A:    _geminiReport.byte2 |= 0x02; break;
    case GEMINI_O:    _geminiReport.byte2 |= 0x04; break;
    case GEMINI_STAR1:_geminiReport.byte2 |= 0x08; break;
    case GEMINI_STAR2:_geminiReport.byte2 |= 0x10; break;
    case GEMINI_RES1: _geminiReport.byte2 |= 0x20; break;
    case GEMINI_RES2: _geminiReport.byte2 |= 0x40; break;
    
    // Byte 3
    case GEMINI_PWR:  _geminiReport.byte3 |= 0x01; break;
    case GEMINI_STAR3:_geminiReport.byte3 |= 0x02; break;
    case GEMINI_STAR4:_geminiReport.byte3 |= 0x04; break;
    case GEMINI_E:    _geminiReport.byte3 |= 0x08; break;
    case GEMINI_U:    _geminiReport.byte3 |= 0x10; break;
    case GEMINI_F:    _geminiReport.byte3 |= 0x20; break;
    case GEMINI_R2:   _geminiReport.byte3 |= 0x40; break;
    
    // Byte 4
    case GEMINI_P2:   _geminiReport.byte4 |= 0x01; break;
    case GEMINI_B:    _geminiReport.byte4 |= 0x02; break;
    case GEMINI_L:    _geminiReport.byte4 |= 0x04; break;
    case GEMINI_G:    _geminiReport.byte4 |= 0x08; break;
    case GEMINI_T2:   _geminiReport.byte4 |= 0x10; break;
    case GEMINI_S:    _geminiReport.byte4 |= 0x20; break;
    case GEMINI_D:    _geminiReport.byte4 |= 0x40; break;
    
    // Byte 5
    case GEMINI_NUM7: _geminiReport.byte5 |= 0x01; break;
    case GEMINI_NUM8: _geminiReport.byte5 |= 0x02; break;
    case GEMINI_NUM9: _geminiReport.byte5 |= 0x04; break;
    case GEMINI_NUM10:_geminiReport.byte5 |= 0x08; break;
    case GEMINI_NUM11:_geminiReport.byte5 |= 0x10; break;
    case GEMINI_NUM12:_geminiReport.byte5 |= 0x20; break;
    case GEMINI_Z:    _geminiReport.byte5 |= 0x40; break;
  }
  
  sendGeminiPRReport();
  return 1;
}

size_t BLEHID::release(int32_t stenoKey) {
  // Clear the appropriate bit
  switch (stenoKey) {
    // Byte 0
    case GEMINI_FN:   _geminiReport.byte0 &= ~0x01; break;
    case GEMINI_NUM1: _geminiReport.byte0 &= ~0x02; break;
    
    // Byte 1  
    case GEMINI_S1:   _geminiReport.byte1 &= ~0x01; break;
    case GEMINI_S2:   _geminiReport.byte1 &= ~0x02; break;
    case GEMINI_T:    _geminiReport.byte1 &= ~0x04; break;
    case GEMINI_K:    _geminiReport.byte1 &= ~0x08; break;
    case GEMINI_P:    _geminiReport.byte1 &= ~0x10; break;
    case GEMINI_W:    _geminiReport.byte1 &= ~0x20; break;
    case GEMINI_H:    _geminiReport.byte1 &= ~0x40; break;
    
    // Byte 2
    case GEMINI_R:    _geminiReport.byte2 &= ~0x01; break;
    case GEMINI_A:    _geminiReport.byte2 &= ~0x02; break;
    case GEMINI_O:    _geminiReport.byte2 &= ~0x04; break;
    case GEMINI_STAR1:_geminiReport.byte2 &= ~0x08; break;
    case GEMINI_STAR2:_geminiReport.byte2 &= ~0x10; break;
    case GEMINI_RES1: _geminiReport.byte2 &= ~0x20; break;
    case GEMINI_RES2: _geminiReport.byte2 &= ~0x40; break;
    
    // Byte 3
    case GEMINI_PWR:  _geminiReport.byte3 &= ~0x01; break;
    case GEMINI_STAR3:_geminiReport.byte3 &= ~0x02; break;
    case GEMINI_STAR4:_geminiReport.byte3 &= ~0x04; break;
    case GEMINI_E:    _geminiReport.byte3 &= ~0x08; break;
    case GEMINI_U:    _geminiReport.byte3 &= ~0x10; break;
    case GEMINI_F:    _geminiReport.byte3 &= ~0x20; break;
    case GEMINI_R2:   _geminiReport.byte3 &= ~0x40; break;
    
    // Byte 4
    case GEMINI_P2:   _geminiReport.byte4 &= ~0x01; break;
    case GEMINI_B:    _geminiReport.byte4 &= ~0x02; break;
    case GEMINI_L:    _geminiReport.byte4 &= ~0x04; break;
    case GEMINI_G:    _geminiReport.byte4 &= ~0x08; break;
    case GEMINI_T2:   _geminiReport.byte4 &= ~0x10; break;
    case GEMINI_S:    _geminiReport.byte4 &= ~0x20; break;
    case GEMINI_D:    _geminiReport.byte4 &= ~0x40; break;
    
    // Byte 5
    case GEMINI_NUM7: _geminiReport.byte5 &= ~0x01; break;
    case GEMINI_NUM8: _geminiReport.byte5 &= ~0x02; break;
    case GEMINI_NUM9: _geminiReport.byte5 &= ~0x04; break;
    case GEMINI_NUM10:_geminiReport.byte5 &= ~0x08; break;
    case GEMINI_NUM11:_geminiReport.byte5 &= ~0x10; break;
    case GEMINI_NUM12:_geminiReport.byte5 &= ~0x20; break;
    case GEMINI_Z:    _geminiReport.byte5 &= ~0x40; break;
  }
  
  sendGeminiPRReport();
  return 1;
}

void BLEHID::geminiStroke(const int32_t* keys, size_t count) {
  releaseAll();
  for (size_t i = 0; i < count; i++) {
    press(keys[i]);
  }
  sendGeminiPRReport();
}

uint8_t BLEHID::stenoCharToKey(char c) {
  switch (toupper(c)) {
    case 'Q': return GEMINI_S1;
    case 'A': return GEMINI_S2;
    case 'W': return GEMINI_T;
    case 'S': return GEMINI_K;
    case 'E': return GEMINI_P;
    case 'D': return GEMINI_W;
    case 'R': return GEMINI_H;
    case 'F': return GEMINI_R;
    case 'C': return GEMINI_A;
    case 'V': return GEMINI_O;
    case 'T': return GEMINI_STAR1;
    case 'G': return GEMINI_STAR2;
    case 'Y': return GEMINI_STAR3;
    case 'H': return GEMINI_STAR4;
    case ',': return GEMINI_E;
    case 'M': return GEMINI_U;
    case 'U': return GEMINI_F;
    case 'J': return GEMINI_R2;
    case 'I': return GEMINI_P2;
    case 'K': return GEMINI_B;
    case 'O': return GEMINI_L;
    case 'L': return GEMINI_G;
    case 'P': return GEMINI_T2;
    case 'B': return GEMINI_S;
    case 'X': return GEMINI_D;
    case 'Z': return GEMINI_Z;
    case '1': return GEMINI_NUM1;
    case '2': return GEMINI_NUM7;
    case '3': return GEMINI_NUM8;
    case '4': return GEMINI_NUM9;
    case '5': return GEMINI_NUM10;
    case '6': return GEMINI_NUM11;
    case '7': return GEMINI_NUM12;
    case '8': return GEMINI_PWR;
    case '9': return GEMINI_RES1;
    case '0': return GEMINI_RES2;
    case 'N': return GEMINI_FN;
    default: return 0;
  }
}

void BLEHID::sendGeminiPRReport() {
    // ONLY send over SPP as 6-byte raw data (for Plover compatibility)
    if (isSerialConnected()) {
        uint8_t serialData[6] = {
            _geminiReport.byte0,
            _geminiReport.byte1, 
            _geminiReport.byte2,
            _geminiReport.byte3,
            _geminiReport.byte4,
            _geminiReport.byte5
        };
        
        sendSerialData(serialData, 6);
        Serial.printf("[%s] Sending GeminiPR SPP: %02X %02X %02X %02X %02X %02X\n", LOG_TAG,
                     serialData[0], serialData[1], serialData[2],
                     serialData[3], serialData[4], serialData[5]);
    } else {
        Serial.printf("[%s] GeminiPR stroke ready but SPP not connected: %02X %02X %02X %02X %02X %02X\n", LOG_TAG,
                     _geminiReport.byte0, _geminiReport.byte1, _geminiReport.byte2,
                     _geminiReport.byte3, _geminiReport.byte4, _geminiReport.byte5);
    }
    
    delay(_delay_ms);
}

void BLEHID::sendSerialData(const uint8_t* data, size_t length) {
    if (serialConnected && serialInput) {
        serialInput->setValue(data, length);
        serialInput->notify();
    }
}

bool BLEHID::isSerialConnected() {
    // If serial is neither on nor connected, then just return false
    if (!serialService || !serialInput) return false;
    
    // Fall back to the connection flag if the above all fails
    return serialConnected;
}
#endif

//
// ----------------------------------------- Gamepad Block
//

#if GAMEPAD_ENABLE
size_t BLEHID::press(int8_t button) {
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        _gamepadReport.buttons[field] |= (1UL << bit);
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatPress[directionIndex][currentHat];
    }
    sendGamepadReport();
    return 1;
}

size_t BLEHID::release(int8_t button) {
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        _gamepadReport.buttons[field] &= ~(1UL << bit);
    } else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatRelease[directionIndex][currentHat];
    }
    sendGamepadReport();
    return 1;
}

bool BLEHID::gamepadIsPressed(int8_t button) {
  if (button >= 1 && button <= 64) {
    uint8_t field = (button - 1) / 32;
    uint8_t bit = (button - 1) % 32;
    return (_gamepadReport.buttons[field] & (1UL << bit)) != 0;
    return false;
  } else if (button >= 65 && button <= 68) {
    uint8_t currentHat = _gamepadReport.hat;
    
    switch (button) {
      case 65: // DPAD_UP
        return (currentHat == HAT_UP || currentHat == HAT_UR || currentHat == HAT_UL);
      case 66: // DPAD_RIGHT
        return (currentHat == HAT_RI || currentHat == HAT_UR || currentHat == HAT_DR);
      case 67: // DPAD_DOWN
        return (currentHat == HAT_DO || currentHat == HAT_DR || currentHat == HAT_DL);
      case 68: // DPAD_LEFT
        return (currentHat == HAT_LE || currentHat == HAT_UL || currentHat == HAT_DL);
    }
  }
  return false;
} 

void BLEHID::gamepadSetAxis(int8_t axis, int16_t value) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    _gamepadReport.axes[axis] = value;
  }
  sendGamepadReport();
}

int16_t BLEHID::gamepadGetAxis(int8_t axis) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    return _gamepadReport.axes[axis];
  }
  return 0;
}

void BLEHID::gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]) {
  memcpy(_gamepadReport.axes, values, sizeof(_gamepadReport.axes));
  sendGamepadReport();
}

void BLEHID::gamepadSetLeftStick(int16_t x, int16_t y) {
    _gamepadReport.axes[AXIS_LX] = x;
    _gamepadReport.axes[AXIS_LY] = y;
    sendGamepadReport();
}

void BLEHID::gamepadSetRightStick(int16_t x, int16_t y) {
    _gamepadReport.axes[AXIS_RX] = x;
    _gamepadReport.axes[AXIS_RY] = y;
    sendGamepadReport();
}

void BLEHID::gamepadSetTriggers(int16_t left, int16_t right) {
    _gamepadReport.axes[AXIS_LT] = left;
    _gamepadReport.axes[AXIS_RT] = right;
    sendGamepadReport();
}

void BLEHID::gamepadGetLeftStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_LX);
    y = gamepadGetAxis(AXIS_LY);
}

void BLEHID::gamepadGetRightStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_RX);
    y = gamepadGetAxis(AXIS_RY);
}

void BLEHID::sendGamepadReport() {
    if (!isConnected() || !inputGamepad) return;
    inputGamepad->setValue((uint8_t*)&_gamepadReport, sizeof(_gamepadReport));
    inputGamepad->notify();
    delay(_delay_ms);
}
#endif

//
// ----------------------------------------- Global Function Block
//

void BLEHID::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) {
    Serial.printf("[%s] ESP-HID onConnect callback triggered - Security: %s, Encrypted: %s\n", LOG_TAG,
             isSecurityEnabled() ? "Enabled" : "Disabled", 
             desc->sec_state.encrypted ? "Yes" : "No");
    
    if (isSecurityEnabled()) {
        if (!desc->sec_state.encrypted) {
            Serial.printf("[%s] Initiating pairing for secure connection\n", LOG_TAG);
            NimBLEDevice::startSecurity(desc->conn_handle);
        }
    }
    
    #if KEYBOARD_ENABLE
      if (inputNKRO) inputNKRO->notify();
    #endif
    
    #if MEDIA_ENABLE
    if (inputMediaKeys) inputMediaKeys->notify(); 
    #endif
    
    #if POINTER_ENABLE
    if (inputMouse) inputMouse->notify();
    if (inputDigitizer) inputDigitizer->notify();
    #endif
    
    #if GEMINIPR_ENABLE
    serialConnected = true;
    Serial.printf("[%s] SPP Serial connected\n", LOG_TAG);
    #endif
    
    #if GAMEPAD_ENABLE
    if (inputGamepad) inputGamepad->notify();
    #endif
    
    Serial.printf("[%s] Client connected - Connection count: %d, Services: HID%s\n", LOG_TAG,
    NimBLEDevice::getServer()->getConnectedCount(),
    #if GEMINIPR_ENABLE
      "+SPP"
    #else
      ""
    #endif
    );
}

bool BLEHID::isConnected(void) {
    // Always check the actual BLE state - relying on cached flags kept breaking for some reason
    if (NimBLEDevice::getServer()) {
        int connectedClients = NimBLEDevice::getServer()->getConnectedCount();
        
        // Debug logging (every 10 seconds)
        static uint64_t lastLogTime = 0;
        uint64_t currentTime = micros();
        
        if (currentTime - lastLogTime > 10000000) { // This is just 10 seconds in microseconds
            Serial.printf("[%s] BLE Status - Connected clients: %d, Advertising: %s\n", LOG_TAG,
                    connectedClients,
                    advertising ? (advertising->isAdvertising() ? "Yes" : "No") : "Null");
            lastLogTime = currentTime;
        }
        
        return (connectedClients > 0);
    }
    
    static uint64_t lastLogTime = 0;
    uint64_t currentTime = micros();
    
    if (currentTime - lastLogTime > 10000000) {
        Serial.printf("[%s] BLE Status: No server instance available\n", LOG_TAG);
        lastLogTime = currentTime;
    }
    
    return false;
}

void pollConnection(void * arg) {
    BLEHID * kb = static_cast<BLEHID*>(arg);
    uint8_t cnt = NimBLEDevice::getServer()->getConnectedCount();

    if (kb->last_connected_count && !cnt) {   // Connection just dropped
        Serial.printf("[%s] Poller: link lost - restarting advertising\n", LOG_TAG);
        
        // Small delay to ensure BLE stack is ready
        delay(100);
        
        if (kb->advertising) {
            kb->advertising->stop();
            delay(50);
            if (!kb->advertising->start()) {
                Serial.printf("[%s] Poller: Failed to restart advertising, will retry\n", LOG_TAG);
            }
        }
    }
    kb->last_connected_count = cnt;
}

void BLEHID::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  NimBLEServerCallbacks::onDisconnect(pServer, connInfo, reason);
  #if GEMINIPR_ENABLE
    serialConnected = false;
    Serial.printf("[%s] SPP Serial disconnected\n", LOG_TAG);
  #endif
  // Restart advertising immediately when disconnected
  if (advertising) {
    advertising->start();
    Serial.printf("[%s] Advertising restarted after disconnect (reason: %d)\n", LOG_TAG, reason);
  }
}

void BLEHID::onWrite(NimBLECharacteristic* me) {
    #if GEMINIPR_ENABLE
    if (me->getUUID().toString() == SERIAL_CHARACTERISTIC_UUID_RX) {
        // Handle incoming serial data if needed
        std::string value = me->getValue();
        Serial.printf("[%s] Received serial data: %s\n", LOG_TAG, value.c_str());
    } else {
        // Existing HID write handling
        Serial.printf("[%s] ESP-HID onWrite callback triggered!\n", LOG_TAG);
        uint8_t* value = (uint8_t*)(me->getValue().c_str());
        size_t length = me->getValue().length();
        Serial.printf("[%s] special keys: %d\n", LOG_TAG, *value);
    }
    #else
    // HID-only write handling when GeminiPR is disabled
    Serial.printf("[%s] ESP-HID onWrite callback triggered!\n", LOG_TAG);
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    size_t length = me->getValue().length();
    Serial.printf("[%s] special keys: %d\n", LOG_TAG, *value);
    #endif
}

size_t BLEHID::write(const uint8_t *buf, size_t len) {
    size_t n = 0;
    while (len--) n += write(*buf++);
    return n;
}

void BLEHID::releaseAll() {
  #if KEYBOARD_ENABLE
  memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
  sendNKROReport();
  #endif
  
  #if MEDIA_ENABLE
  _mediaKeyBitmask = 0;
  sendMediaReport();
  #endif
  
  #if GEMINIPR_ENABLE
  memset(&_geminiReport, 0, sizeof(_geminiReport));
  sendGeminiPRReport();
  #endif
  
  #if GAMEPAD_ENABLE
  _gamepadReport.buttons[0] = 0;
  _gamepadReport.buttons[1] = 0;
  _gamepadReport.hat = HAT_CE;
  sendGamepadReport();
  #endif
}

void BLEHID::securityCallback(uint32_t passkey) {
  Serial.printf("[%s] Pairing PIN: %06lu\n", LOG_TAG, passkey);
  // You could add display output here if you have an LCD/OLED
}

void BLEHID::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    Serial.printf("[%s] Authentication complete - encrypted: %s, authenticated: %s\n", LOG_TAG,
             desc->sec_state.encrypted ? "yes" : "no",
             desc->sec_state.authenticated ? "yes" : "no");
}

void BLEHID::setPIN(const char* pin) {
  if (pin == nullptr) {
    disableSecurity();
    return;
  }
  
  if (strlen(pin) == 6) {
    this->passkey = atoi(pin);
    Serial.printf("[%s] Security enabled with PIN: %s\n", LOG_TAG, pin);
  } else {
    Serial.printf("[%s] PIN must be 6 digits, security disabled\n", LOG_TAG);
    this->passkey = 0;
  }
}

void BLEHID::setPIN(uint32_t pin) {
  if (pin >= 1 && pin <= 999999) {  // 0 means no security
    this->passkey = pin;
    Serial.printf("[%s] Security enabled with PIN: %06lu\n", LOG_TAG, pin);
  } else {
    Serial.printf("[%s] PIN must be between 000001 and 999999, security disabled\n", LOG_TAG);
    this->passkey = 0;
  }
}

void BLEHID::disableSecurity(bool enable) {
  if (!enable) {
    this->passkey = 0;
    Serial.printf("[%s] Security disabled\n", LOG_TAG);
  } else {
    Serial.printf("[%s] Security remains enabled (call setPIN to enable)\n", LOG_TAG);
  }
}

bool BLEHID::isSecurityEnabled() const {
  return passkey != 0;
}

uint32_t BLEHID::onPassKeyRequest() {
  Serial.printf("[%s] PassKeyRequest received\n", LOG_TAG);
  if (isSecurityEnabled()) {
    securityCallback(passkey);
    return passkey;
  }
  return 0; // No PIN = Just Works
}

bool BLEHID::onSecurityRequest() {
  Serial.printf("[%s] Security request received\n", LOG_TAG);
  return isSecurityEnabled(); // Only require auth if we have a PIN
}
