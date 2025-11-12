#include "BLEHID.h"

#if GEMINIPR_ENABLE
// SPP UUIDs (Standard Serial Port Service)
const char* BLEHID::SERIAL_SERVICE_UUID           = "00001101-0000-1000-8000-00805f9b34fb";
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

#if MOUSE_ENABLE
  +  sizeof(_mouseReportDescriptor)
#endif

#if DIGITIZER_ENABLE
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
        
        #if MOUSE_ENABLE
        memcpy(_hidReportDescriptor + currentPosition, _mouseReportDescriptor, sizeof(_mouseReportDescriptor));
        currentPosition += sizeof(_mouseReportDescriptor);
        #endif
    
        #if DIGITIZER_ENABLE
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
    , lastPollTime(0) 
{
  AsyncLogger::getInstance().initialize(); 
  _activeBLEHIDInstance = this;
  BLE_LOG_INFO(LOG_TAG, "BLEHID instance created");
}
// This is a "destructor". It takes objects the contructor made, and destroys them whenever you tell it to. 
BLEHID::~BLEHID() { 
    // Unregister this instance
    if (_activeBLEHIDInstance == this) {
        _activeBLEHIDInstance = nullptr;
    }
}

void BLEHID::begin(void) {
    // Initialise BLE stack only once
    if (getInitialized) {
        BLE_LOG_WARN(LOG_TAG, "BLE already initialized, cleaning up first...");
        end();
        delay(100);
    } else { NimBLEDevice::init(deviceName.c_str()); }
    
    NimBLEDevice::setSecurityAuth(false, true, true); // Bonding, MITM, Secure Connections disabled
    BLE_LOG_INFO(LOG_TAG, "Just Works simple pairing enabled");
    
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
    
    #if MOUSE_ENABLE
    inputMouse     = hid->getInputReport(MOUSE_ID);
    #endif
    
    #if DIGITIZER_ENABLE
    inputDigitizer = hid->getInputReport(DIGITIZER_ID);
    #endif
    
    #if GEMINIPR_ENABLE
      BLEService* serialService = pServer->createService(SERIAL_SERVICE_UUID);

      // Create TX Characteristic (device -> client)
      BLECharacteristic* serialInput = serialService->createCharacteristic(SERIAL_CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);

      // Create RX Characteristic (client -> device)  
      BLECharacteristic* serialOutput = serialService->createCharacteristic(SERIAL_CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    #endif
    
    #if GAMEPAD_ENABLE
    inputGamepad   = hid->getInputReport(GAMEPAD_ID);
    #endif
    
    // Set callbacks
    outputKeyboard->setCallbacks(this);
    
    #if KEYBOARD_ENABLE
    if (inputNKRO) {
      inputNKRO->setCallbacks(this);
      nkro.begin(inputNKRO, _delay_ms);  // Initialize the BLENKRO instance
    }
    #endif
    
    #if MEDIA_ENABLE
    if (inputMediaKeys) {
      inputMediaKeys->setCallbacks(this);
      media.begin(inputMediaKeys, _delay_ms);  // Initialize the BLEMEDIA instance
    }
    #endif
    
    #if MOUSE_ENABLE
    if (inputMouse) {
      inputMouse->setCallbacks(this);
      mouse.begin(inputMouse, _delay_ms);
    }
    #endif
    
    #if DIGITIZER_ENABLE
    if (inputDigitizer) {
      inputDigitizer->setCallbacks(this);
      digitizer.begin(inputDigitizer, _delay_ms);
    }
    #endif
    
    #if GEMINIPR_ENABLE
    serialOutput->setCallbacks(this);
    steno.begin(serialService, serialInput, serialOutput, _delay_ms);
    #endif
    
    if (inputGamepad) {
        inputGamepad->setCallbacks(this);
        gamepad.begin(inputGamepad, _delay_ms);  // Initialize the BLEGAMEPAD instance
    }
    
    // Manufacturer / PnP / HID-info
    hid->setManufacturer(std::string(deviceManufacturer.c_str()));
    hid->setHidInfo(0x11, 0x01);

    // Publish HID report map and start services
    hid->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->startServices();
    
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
        ad.addServiceUUID(BLEUUID((uint16_t)0x180F));
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

    BLE_LOG_INFO(LOG_TAG, "Advertising started!");
    BLE_LOG_INFO(LOG_TAG, "Device name: %s", deviceName.c_str());
    BLE_LOG_INFO(LOG_TAG, "Service UUID: %s", hid->getHidService()->getUUID().toString().c_str());
    BLE_LOG_INFO(LOG_TAG, "Using %s mode by default", nkro.isNKROEnabled() ? "NKRO" : "6KRO");
    
    getInitialized = true;
}

// Update/polling function
void BLEHID::update() {
  static uint32_t lastUpdateTime        = 0;
  static uint32_t lastBatteryUpdateTime = 0;
  static uint32_t lastLogProcessTime    = 0;
  uint32_t currentTime                  = millis();
  
  if (currentTime - lastLogProcessTime >= 10) {
        lastLogProcessTime = currentTime;
        BLE_LOG_PROCESS();
  }
  
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
  
  // Update battery level every 30 seconds if connected
  if (isConnected() && (currentTime - lastBatteryUpdateTime >= 30000)) {
    lastBatteryUpdateTime = currentTime;
    // This ensures the host device always has the current battery level
    if (hid != 0) {
      BLEService* batteryService = hid->getBatteryService();
      if (batteryService) {
        BLECharacteristic* batteryLevelChar = batteryService->getCharacteristic((uint16_t)0x2A19);
        if (batteryLevelChar) {
          batteryLevelChar->setValue(&batteryLevel, 1);
        }
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
  BLE_LOG_INFO(LOG_TAG, "BLE Keyboard stopped");
}

//
// ----------------------------------------- Global Function Block
//

void BLEHID::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) {
    BLE_LOG_DEBUG(LOG_TAG, "ESP-HID onConnect callback triggered");
    
    #if KEYBOARD_ENABLE
      if (inputNKRO) inputNKRO->notify();
    #endif
    
    #if MEDIA_ENABLE
    if (inputMediaKeys) inputMediaKeys->notify(); 
    #endif
    
    #if MOUSE_ENABLE
    if (inputMouse) inputMouse->notify();
    #endif
    
    #if DIGITIZER_ENABLE
    if (inputDigitizer) inputDigitizer->notify();
    #endif
    
    #if GEMINIPR_ENABLE
      steno.setSerialConnected(true);
      BLE_LOG_INFO(LOG_TAG, "SPP Serial connected");
    #endif
    
    #if GAMEPAD_ENABLE
    if (inputGamepad) inputGamepad->notify();
    #endif
    
    BLE_LOG_INFO(LOG_TAG, "Client connected - Connection count: %d, Services: HID%s",
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
            BLE_LOG_DEBUG(LOG_TAG, "BLE Status - Connected clients: %d, Advertising: %s",
                    connectedClients,
                    advertising ? (advertising->isAdvertising() ? "Yes" : "No") : "Null");
            lastLogTime = currentTime;
        }
        
        return (connectedClients > 0);
    }
    
    static uint64_t lastLogTime = 0;
    uint64_t currentTime = micros();
    
    if (currentTime - lastLogTime > 10000000) {
        BLE_LOG_DEBUG(LOG_TAG, "BLE Status: No server instance available");
        lastLogTime = currentTime;
    }
    
    return false;
}

void pollConnection(void * arg) {
    BLEHID * kb = static_cast<BLEHID*>(arg);
    uint8_t cnt = NimBLEDevice::getServer()->getConnectedCount();

    if (kb->last_connected_count && !cnt) {   // Connection just dropped
        BLE_LOG_WARN(LOG_TAG, "Poller: link lost - restarting advertising");
        
        // Small delay to ensure BLE stack is ready
        delay(100);
        
        if (kb->advertising) {
            kb->advertising->stop();
            delay(50);
            if (!kb->advertising->start()) {
                BLE_LOG_ERROR(LOG_TAG, "Poller: Failed to restart advertising, will retry");
            }
        }
    }
    kb->last_connected_count = cnt;
}

void BLEHID::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  NimBLEServerCallbacks::onDisconnect(pServer, connInfo, reason);
  #if GEMINIPR_ENABLE
    steno.setSerialConnected(false);
    BLE_LOG_INFO(LOG_TAG, "SPP Serial disconnected");
  #endif
  // Restart advertising immediately when disconnected
  if (advertising) {
    advertising->start();
    BLE_LOG_INFO(LOG_TAG, "Advertising restarted after disconnect (reason: %d)", reason);
  }
}

void BLEHID::onWrite(NimBLECharacteristic* me) {
    #if GEMINIPR_ENABLE
    if (me->getUUID().toString() == SERIAL_CHARACTERISTIC_UUID_RX) {
        // Handle incoming serial data if needed
        std::string value = me->getValue();
        BLE_LOG_DEBUG(LOG_TAG, "Received serial data: %s", value.c_str());
    } else {
        // Existing HID write handling
        BLE_LOG_DEBUG(LOG_TAG, "ESP-HID onWrite callback triggered");
        uint8_t* value = (uint8_t*)(me->getValue().c_str());
        size_t length = me->getValue().length();
        BLE_LOG_DEBUG(LOG_TAG, "special keys: %d", *value);
    }
    #else
    // HID-only write handling when GeminiPR is disabled
    BLE_LOG_DEBUG(LOG_TAG, "ESP-HID onWrite callback triggered");
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    size_t length = me->getValue().length();
    BLE_LOG_DEBUG(LOG_TAG, "special keys: %d", *value);
    #endif
}

size_t BLEHID::write(const uint8_t *buf, size_t len) {
    size_t n = 0;
    while (len--) n += write(*buf++);
    return n;
}

void BLEHID::releaseAll() {
  #if KEYBOARD_ENABLE
  nkro.releaseAll();
  #endif
  
  #if MEDIA_ENABLE
  media.releaseAll();
  #endif
  
  #if MOUSE_ENABLE
  mouse.releaseAll();
  #endif
  
  #if GEMINIPR_ENABLE
  steno.releaseAll();
  #endif
  
  #if GAMEPAD_ENABLE
  gamepad.releaseAll();
  #endif
}

void BLEHID::setAppearance(uint16_t newAppearance) {
  this->appearance = newAppearance;
  
  #if DIGITIZER_ENABLE
  digitizer.setAppearance(newAppearance);
  #endif
  
  #if DIGITIZER_ENABLE && MOUSE_ENABLE
  BLE_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X, Mode: %s", newAppearance, digitizer.isAbsoluteMode() ? "absolute" : "relative");
  #elif !DIGITIZER_ENABLE && MOUSE_ENABLE
  BLE_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X (Mouse only)", newAppearance);
  #elif DIGITIZER_ENABLE && !MOUSE_ENABLE
  BLE_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X (Digitizer only)", newAppearance);
  #else
  BLE_LOG_INFO(LOG_TAG, "Appearance set to: 0x%04X", newAppearance);
  #endif
}

void BLEHID::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0) {
    this->hid->setBatteryLevel(this->batteryLevel);
    
    // Force an update to the BLE characteristic
    if (isConnected()) {
      // This ensures the host device receives the updated battery level
      hid->getBatteryService()->getCharacteristic((uint16_t)0x2A19)->setValue(&batteryLevel, 1);
      hid->getBatteryService()->getCharacteristic((uint16_t)0x2A19)->notify();
    }
  }
}

//must be called before begin in order to set the name
void BLEHID::setName(std::string deviceName) { this->deviceName = deviceName; }

//must be called before begin in order to set the manufacturer
void BLEHID::setManufacturer(std::string deviceManufacturer) { this->deviceManufacturer = deviceManufacturer; }

// Sets the waiting time (in milliseconds) between multiple keystrokes
void BLEHID::setDelay(uint32_t ms) { _delay_ms = ms; }

void BLEHID::setVendorId(uint16_t vid) { this->vid = vid; }

void BLEHID::setProductId(uint16_t pid) { this->pid = pid; }

void BLEHID::setVersion(uint16_t version) { this->version = version; }

//
// ----------------------------------------- NKRO Keyboard Block
//

#if KEYBOARD_ENABLE
size_t BLEHID::press(uint8_t k) { return nkro.press(k); }

size_t BLEHID::press(int16_t modifier) { return nkro.press(modifier); }

size_t BLEHID::release(uint8_t k) { return nkro.release(k); }

size_t BLEHID::release(int16_t modifier) { return nkro.release(modifier); }

size_t BLEHID::write(uint8_t c) { return nkro.write(c); }

size_t BLEHID::write(int16_t modifier) { return nkro.write(modifier); }

void BLEHID::useNKRO(bool state) { nkro.useNKRO(state); }

void BLEHID::use6KRO(bool state) { nkro.use6KRO(state); }

bool BLEHID::isNKROEnabled() { return nkro.isNKROEnabled(); }

void BLEHID::setModifiers(uint8_t modifiers) { nkro.setModifiers(modifiers); }

uint8_t BLEHID::getModifiers() { return nkro.getModifiers(); }

void BLEHID::sendNKROReport() { nkro.sendNKROReport(); }
#endif

//
// ----------------------------------------- Media key Block
//

#if MEDIA_ENABLE
size_t BLEHID::press(uint16_t mediaKey) { return media.press(mediaKey); }

size_t BLEHID::release(uint16_t mediaKey) { return media.release(mediaKey); }

size_t BLEHID::write(uint16_t mediaKey) { return media.write(mediaKey); }

void BLEHID::setMediaKeyBitmask(uint32_t bitmask) { media.setMediaKeyBitmask(bitmask); }

uint32_t BLEHID::getMediaKeyBitmask() { return media.getMediaKeyBitmask(); }

void BLEHID::addMediaKey(uint16_t mediaKey) { media.addMediaKey(mediaKey); }

void BLEHID::removeMediaKey(uint16_t mediaKey) { media.removeMediaKey(mediaKey); }

void BLEHID::sendMediaReport() { media.sendMediaReport(); }
#endif

//
// ----------------------------------------- Mouse Block
//

#if MOUSE_ENABLE
size_t BLEHID::press(char b) { return mouse.press(b); }

size_t BLEHID::release(char b) { return mouse.release(b); }

void BLEHID::click(char b) { mouse.click(b); }

void BLEHID::move(signed char x, signed char y, signed char wheel, signed char hWheel) { mouse.move(x, y, wheel, hWheel); }

bool BLEHID::mouseIsPressed(char b) { return mouse.mouseIsPressed(b); }
#endif

//
// ----------------------------------------- Digitizer Block
//

#if DIGITIZER_ENABLE
void BLEHID::click(uint16_t x, uint16_t y, char b) { digitizer.click(x, y, b); }

void BLEHID::moveTo(uint16_t x, uint16_t y, uint8_t pressure, uint8_t buttons) { digitizer.moveTo(x, y, pressure, buttons); }

void BLEHID::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) { digitizer.beginStroke(x, y, initialPressure); }

void BLEHID::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) { digitizer.updateStroke(x, y, pressure); }

void BLEHID::endStroke(uint16_t x, uint16_t y) { digitizer.endStroke(x, y); }

void BLEHID::useAbsoluteMode(bool state) { digitizer.useAbsoluteMode(state); }

bool BLEHID::isAbsoluteMode() { return digitizer.isAbsoluteMode(); }

void BLEHID::useAutoMode(bool state) { digitizer.useAutoMode(state); }

void BLEHID::setDigitizerRange(uint16_t maxX, uint16_t maxY) { digitizer.setDigitizerRange(maxX, maxY); }

bool BLEHID::isAutoModeEnabled() { return digitizer.isAutoModeEnabled(); }

void BLEHID::sendDigitizerReport() { digitizer.sendDigitizerReport(); }
#endif

//
// ----------------------------------------- GeminiPR Stenotype Block
//

#if GEMINIPR_ENABLE
size_t BLEHID::press(int32_t stenoKey) { return steno.press(stenoKey); }

size_t BLEHID::release(int32_t stenoKey) { return steno.release(stenoKey); }

void BLEHID::geminiStroke(const int32_t* keys, size_t count) { steno.geminiStroke(keys, count); }

uint8_t BLEHID::stenoCharToKey(char c) { return steno.stenoCharToKey(c); }

void BLEHID::sendGeminiPRReport() { steno.sendGeminiPRReport(); }

void BLEHID::sendSerialData(const uint8_t* data, size_t length) { steno.sendSerialData(data, length); }

bool BLEHID::isSerialConnected() { return steno.isSerialConnected(); }
#endif

//
// ----------------------------------------- Gamepad Block
//

#if GAMEPAD_ENABLE
size_t BLEHID::press(int8_t button) { return gamepad.press(button); }

size_t BLEHID::release(int8_t button) { return gamepad.release(button); }

bool BLEHID::gamepadIsPressed(int8_t button) { return gamepad.gamepadIsPressed(button); }

void BLEHID::gamepadSetLeftStick(int16_t x, int16_t y) { gamepad.gamepadSetLeftStick(x, y); }

void BLEHID::gamepadSetRightStick(int16_t x, int16_t y) { gamepad.gamepadSetRightStick(x, y); }

void BLEHID::gamepadSetTriggers(int16_t left, int16_t right) { gamepad.gamepadSetTriggers(left, right); }

void BLEHID::gamepadGetLeftStick(int16_t &x, int16_t &y) { gamepad.gamepadGetLeftStick(x, y); }

void BLEHID::gamepadGetRightStick(int16_t &x, int16_t &y) { gamepad.gamepadGetRightStick(x, y); }

void BLEHID::gamepadSetAxis(int8_t axis, int16_t value) { gamepad.gamepadSetAxis(axis, value); }

int16_t BLEHID::gamepadGetAxis(int8_t axis) { return gamepad.gamepadGetAxis(axis); }

void BLEHID::gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]) { gamepad.gamepadSetAllAxes(values); }

void BLEHID::sendGamepadReport() { gamepad.sendGamepadReport(); }
#endif
