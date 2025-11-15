/**
 * @file SQUIDHID.cpp
 * @brief Implementation of the full library
 */

#include "SQUIDHID.h"

#if GEMINIPR_ENABLE
// SPP UUIDs (Standard Serial Port Service)
const char* SQUIDHID::SERIAL_SERVICE_UUID           = "00001101-0000-1000-8000-00805f9b34fb";
const char* SQUIDHID::SERIAL_CHARACTERISTIC_UUID_TX = "00001102-0000-1000-8000-00805f9b34fb";
const char* SQUIDHID::SERIAL_CHARACTERISTIC_UUID_RX = "00001103-0000-1000-8000-00805f9b34fb";
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
static const char* LOG_TAG = "SQUIDHID";
static             SQUIDHID* _activeSQUIDHIDInstance = nullptr;
void               pollConnection(void * arg);

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

// This is a "constructor". It takes that class from the SQUIDHID.h file, and turns it into "objects" that can actually be used.
SQUIDHID::SQUIDHID(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel, SquidFactory::Implementation impl)
    : ble(SquidFactory::create(impl))
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel)
    , initialized(false)
    , last_connected_count(0)
    , lastPollTime(0)
    , _delay_ms(7)
{
    SQUIDLOGS::getInstance().initialize(); 
    _activeSQUIDHIDInstance = this;
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID instance created");
}

// This is a "destructor". It takes objects the contructor made, and destroys them whenever you tell it to. 
SQUIDHID::~SQUIDHID() { 
    SQUID_LOG_DEBUG(LOG_TAG, "SQUIDHID destructor called");
    
    // Always call end() to ensure proper cleanup
    end();

    // Unregister this instance
    if (_activeSQUIDHIDInstance == this) {
        _activeSQUIDHIDInstance = nullptr;
        SQUID_LOG_DEBUG(LOG_TAG, "Active instance unregistered");
    }
    
    SQUID_LOG_DEBUG(LOG_TAG, "SQUIDHID destructor completed");
}

void SQUIDHID::begin(void) {
    if (initialized) {
        SQUID_LOG_WARN(LOG_TAG, "BLE already initialized, cleaning up first...");
        end();
        delay(100);
    }
    
    if (!ble->init(deviceName.c_str())) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to initialize BLE stack");
        return;
    }
    
    ble->setSecurityAuth(false, true, true);
    SQUID_LOG_INFO(LOG_TAG, "Just Works simple pairing enabled");
    
    // Create server - store raw pointer (owned by ble)
    pServer = ble->createServer();
    if (!pServer) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to create BLE server");
        return;
    }
    pServer->setCallbacks(this);

    // Create HID device using smart pointer
    hid.reset(ble->createHIDDevice(pServer));
    if (!hid) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to create HID device");
        return;
    }

    // Create advertising using smart pointer
    advertising.reset(ble->getAdvertising());
    if (!advertising) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to get advertising object");
        return;
    }

    // Obtain characteristics - these are raw pointers (owned by hid device)
    outputKeyboard = hid->getOutputReport(KEYBOARD_ID);
    
    #if KEYBOARD_ENABLE
    inputNKRO = hid->getInputReport(NKRO_ID);
    #endif
    
    #if MEDIA_ENABLE
    inputMediaKeys = hid->getInputReport(MEDIA_KEYS_ID);
    #endif
    
    #if MOUSE_ENABLE
    inputMouse = hid->getInputReport(MOUSE_ID);
    #endif
    
    #if DIGITIZER_ENABLE
    inputDigitizer = hid->getInputReport(DIGITIZER_ID);
    #endif
    
    #if GAMEPAD_ENABLE
    inputGamepad = hid->getInputReport(GAMEPAD_ID);
    #endif
    
    // Set callbacks using raw pointers
    if (outputKeyboard) {
        outputKeyboard->setCallbacks(this);
    }
    
    // Initialize feature modules with raw pointers
    #if KEYBOARD_ENABLE
    if (inputNKRO) {
        inputNKRO->setCallbacks(this);
        nkro.begin(inputNKRO, _delay_ms);
    }
    #endif
    
    #if MEDIA_ENABLE
    if (inputMediaKeys) {
        inputMediaKeys->setCallbacks(this);
        media.begin(inputMediaKeys, _delay_ms);
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
    
    #if GAMEPAD_ENABLE
    if (inputGamepad) {
        inputGamepad->setCallbacks(this);
        gamepad.begin(inputGamepad, _delay_ms);
    }
    #endif
    
    #if GEMINIPR_ENABLE
    // Create serial service using smart pointer
    serialService.reset(pServer->createService(SERIAL_SERVICE_UUID));
    if (serialService) {
        // These factory methods return smart pointers
        serialInput = serialService->createCharacteristic(SERIAL_CHARACTERISTIC_UUID_TX, SquidProperty::NOTIFY);
        serialOutput = serialService->createCharacteristic(SERIAL_CHARACTERISTIC_UUID_RX, SquidProperty::WRITE | SquidProperty::WRITE_NR);
        
        if (serialInput && serialOutput) {
            serialOutput->setCallbacks(this);
            serialService->start();
            steno.begin(serialService.get(), serialInput.get(), serialOutput.get(), _delay_ms);
            SQUID_LOG_INFO(LOG_TAG, "Serial service started successfully");
        }
    }
    #endif
    
    // Manufacturer / PnP / HID-info
    hid->setManufacturer(deviceManufacturer);
    hid->setHidInfo(0x11, 0x01);

    // Publish HID report map and start services
    hid->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->startServices();
    
    // Create Device Information Service - use local smart pointers (not class members)
    P_AUTO(disService, pServer->createService(0x180A));
    if (disService) {
        // These factory methods return smart pointers
        P_AUTO(manufChar, disService->createCharacteristic(0x2A29, SquidProperty::READ));
        if (manufChar) manufChar->setValue(deviceManufacturer);

        P_AUTO(modelChar, disService->createCharacteristic(0x2A24, SquidProperty::READ));
        if (modelChar) modelChar->setValue(deviceName);

        P_AUTO(fwChar, disService->createCharacteristic(0x2A26, SquidProperty::READ));
        if (fwChar) fwChar->setValue(BLE_KEYBOARD_VERSION);

        P_AUTO(pnpChar, disService->createCharacteristic(0x2A50, SquidProperty::READ));
        if (pnpChar) {
            uint8_t pnpId[7] = {0x02, vid & 0xFF, vid >> 8, pid & 0xFF, pid >> 8, version & 0xFF, version >> 8};
            pnpChar->setValue(pnpId, 7);
        }
        disService->start();
        SQUID_LOG_DEBUG(LOG_TAG, "Device Information Service started");
    }

    // Advertising setup
    advData = ble->createAdvertisementData();
    scanData = ble->createAdvertisementData();
    
    if (!advData || !scanData) {
        SQUID_LOG_ERROR(LOG_TAG, "Failed to create advertisement data");
        return;
    }
    
    // Configure advertisements
    auto configureAd = [this](SquidAdvertisementData& ad) {
        ad.setFlags(0x06);
        ad.setName(deviceName);
        ad.setAppearance(this->appearance);
    };
    
    configureAd(*advData);
    configureAd(*scanData);
    scanData->setShortName(deviceName.substr(0, 8));
    
    // Add services to advertisements - getHidService() returns smart pointer
    P_AUTO(hidService, hid->getHidService());
    if (hidService) {
        P_AUTO(hidUUID, hidService->getUUID());
        if (hidUUID) {
            scanData->addServiceUUID(*hidUUID);
            advData->setCompleteServices(*hidUUID);
            SQUID_LOG_DEBUG(LOG_TAG, "Added HID service to advertisement");
        }
    }
    
    #if GEMINIPR_ENABLE
    if (serialService) {
        P_AUTO(serialUUID, serialService->getUUID());
        if (serialUUID) {
            scanData->addServiceUUID(*serialUUID);
            advData->addServiceUUID(*serialUUID);
            SQUID_LOG_DEBUG(LOG_TAG, "Added Serial service to advertisement");
        }
    }
    #endif

    advertising->setMinInterval(40);
    advertising->setMaxInterval(160);
    advertising->setAdvertisementData(*advData);
    advertising->setScanResponseData(*scanData);

    // Start advertising
    onStarted(pServer);
    advertising->start();
    hid->setBatteryLevel(batteryLevel);
    
    lastPollTime = millis();
    initialized = true;
    
    SQUID_LOG_INFO(LOG_TAG, "SQUIDHID started successfully");
}

// Update/polling function
void SQUIDHID::update() {
    
    static uint32_t lastHeartBeat = 0;
    static uint32_t lastUpdateTime = 0;
    static uint32_t lastBatteryUpdateTime = 0;
    static uint32_t lastLogProcessTime = 0;
    uint32_t currentTime = millis();
    
    // Process logs every 50ms (safe to do even if not initialized)
    if (currentTime - lastLogProcessTime >= 50) {
        lastLogProcessTime = currentTime;
        SQUID_LOG_PROCESS();
    }
    
    // Process heartbeat every 50ms to keep our objects alive
    if (currentTime - lastHeartBeat >= 50) {          
        lastHeartBeat = currentTime;

        if (hid) {
            #if KEYBOARD_ENABLE
            nkro.sendNKROReport();       // NKRO heartbeat
            #endif
            #if MEDIA_ENABLE
            media.sendMediaReport(); // Media heartbeat
            #endif
            #if DIGITIZER_ENABLE
            digitizer.sendDigitizerReport(); // Digitizer heartbeat
            #endif
            #if GAMEPAD_ENABLE
            gamepad.sendGamepadReport(); // Gamepad heartbeat
            #endif
        }
    }
    
    // CRITICAL SAFETY CHECK: Ensure instance is fully initialized and valid
    if (!initialized || !ble) {
        // Only log this warning occasionally to avoid spam
        static uint32_t lastWarningTime = 0;
        if (currentTime - lastWarningTime > 5000) { // Every 5 seconds
            SQUID_LOG_WARN(LOG_TAG, "Update called but BLE not initialized (init: %d, ble: %d)", 
                          initialized, ble != nullptr);
            lastWarningTime = currentTime;
        }
        return;
    }
    
    // Handle SCAN_INTERVAL timing
    if (currentTime - lastUpdateTime >= SCAN_INTERVAL) {
        lastUpdateTime = currentTime;
        
        uint32_t currentPollTime = millis();
        
        // Handle millis() rollover safely
        if (currentPollTime < lastPollTime) {
            lastPollTime = currentPollTime;
            return;
        }
        
        // Handle POLL_INTERVAL timing with safety checks
        if (currentPollTime - lastPollTime >= POLL_INTERVAL) {
            lastPollTime = currentPollTime;
            
            // Direct polling implementation (replaces pollConnection function)
            // Double-check ble is still valid
            if (ble) {
                uint8_t cnt = ble->getConnectedCount();
                
                // Connection state changed from connected to disconnected
                if (last_connected_count && !cnt) {
                    SQUID_LOG_WARN(LOG_TAG, "Connection lost - clients dropped from %d to %d", 
                                  last_connected_count, cnt);
                    
                    // Small delay to ensure BLE stack is ready
                    delay(100);
                    
                    // Safely restart advertising if needed
                    if (advertising) {
                        if (!advertising->isAdvertising()) {
                            SQUID_LOG_INFO(LOG_TAG, "Advertising not running, attempting to restart");
                        } else {
                            SQUID_LOG_DEBUG(LOG_TAG, "Advertising already running, no restart needed");
                        }
                    } else {
                        SQUID_LOG_WARN(LOG_TAG, "No advertising object available to restart");
                    }
                }
                // Connection state changed from disconnected to connected
                else if (!last_connected_count && cnt) {
                    SQUID_LOG_INFO(LOG_TAG, "Connection established - clients: %d", cnt);
                }
                
                last_connected_count = cnt;
            } else {
                SQUID_LOG_ERROR(LOG_TAG, "BLE interface became null during polling");
                initialized = false; // Mark as uninitialized to prevent further operations
            }
        }
    }
    
    // Update battery level every 30 seconds if connected
    if (isConnected() && (currentTime - lastBatteryUpdateTime >= 30000)) {
        lastBatteryUpdateTime = currentTime;
        
        // Triple-check all pointers before battery update
        if (hid) {
            P_AUTO(batteryService, hid->getBatteryService());
            if (batteryService) {
                P_AUTO(batteryLevelChar, batteryService->getCharacteristic(0x2A19));
                if (batteryLevelChar) {
                    // Update the characteristic value
                    batteryLevelChar->setValue(&batteryLevel, 1);
                    SQUID_LOG_DEBUG(LOG_TAG, "Battery level updated to %d%%", batteryLevel);
                    
                    // Optional: Notify connected clients (if supported and connected)
                    // Note: Some systems may not support notifications for battery level
                    if (isConnected()) {
                        batteryLevelChar->notify();
                    }
                } else {
                    SQUID_LOG_WARN(LOG_TAG, "Battery level characteristic not found");
                }
            } else {
                SQUID_LOG_WARN(LOG_TAG, "Battery service not available");
            }
        } else {
            SQUID_LOG_WARN(LOG_TAG, "HID device not available for battery update");
        }
    }
    
    // Optional: Add periodic health check (every 60 seconds)
    static uint32_t lastHealthCheckTime = 0;
    if (currentTime - lastHealthCheckTime >= 60000) {
        lastHealthCheckTime = currentTime;
        
        SQUID_LOG_DEBUG(LOG_TAG, "Health check - Init: %d, BLE: %d, Advertising: %d, HID: %d, Connected: %d",
                       initialized, 
                       ble != nullptr,
                       advertising != nullptr,
                       hid != nullptr,
                       isConnected());
        
        // If we think we're initialized but critical components are missing, auto-recover
        if (initialized && (!ble || !advertising || !hid)) {
            SQUID_LOG_ERROR(LOG_TAG, "Health check failed - critical components missing, attempting recovery");
            end(); // Clean up
            delay(100);
            begin(); // Try to reinitialize
        }
    }
}

void SQUIDHID::end(void) {
    SQUID_LOG_DEBUG(LOG_TAG, "SQUIDHID end() called");
    
    if (!initialized) {
        SQUID_LOG_DEBUG(LOG_TAG, "SQUIDHID not initialized, nothing to clean up");
        return;
    }
    
    // Stop advertising first
    if (advertising) {
        advertising->stop();
        SQUID_LOG_DEBUG(LOG_TAG, "Advertising stopped");
    }
    
    // Reset raw pointers (we don't own these)
    pServer = nullptr;
    outputKeyboard = nullptr;
    
    #if KEYBOARD_ENABLE
    inputNKRO = nullptr;
    #endif
    #if MEDIA_ENABLE
    inputMediaKeys = nullptr;
    #endif
    #if MOUSE_ENABLE
    inputMouse = nullptr;
    #endif
    #if DIGITIZER_ENABLE
    inputDigitizer = nullptr;
    #endif
    #if GAMEPAD_ENABLE
    inputGamepad = nullptr;
    #endif
    
    // Reset smart pointers (this will delete the objects)
    hid.reset();
    advertising.reset();
    advData.reset();
    scanData.reset();
    
    #if GEMINIPR_ENABLE
    serialService.reset();
    serialInput.reset();
    serialOutput.reset();
    #endif
    
    // Deinitialize BLE stack
    if (ble) {
        ble->deinit(true);
        SQUID_LOG_DEBUG(LOG_TAG, "BLE stack deinitialized");
    }
    
    // Reset initialization flag
    initialized = false;
    
    SQUID_LOG_INFO(LOG_TAG, "BLE Keyboard stopped completely");
}

//
// ----------------------------------------- Global Function Block
//

void SQUIDHID::onConnect(SquidServer *pServer) {
    SQUID_LOG_DEBUG(LOG_TAG, "ESP-HID onConnect callback triggered");
    
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
      SQUID_LOG_INFO(LOG_TAG, "SPP Serial connected");
    #endif
    
    #if GAMEPAD_ENABLE
    if (inputGamepad) inputGamepad->notify();
    #endif
    
    SQUID_LOG_INFO(LOG_TAG, "Client connected - Connection count: %d, Services: HID%s",
    ble->getConnectedCount(),
    #if GEMINIPR_ENABLE
      "+SPP"
    #else
      ""
    #endif
    );
}

bool SQUIDHID::isConnected(void) {
    // Comprehensive safety check
    if (!initialized || !ble || !ble.get()) {
        return false;
    }
    
    try {
        int connectedClients = ble->getConnectedCount();

        // Debug logging (every 10 seconds max to avoid spam)
        static uint64_t lastLogTime = 0;
        uint64_t currentTime = micros();
        
        if (currentTime - lastLogTime > 10000000) {
            SQUID_LOG_DEBUG(LOG_TAG, "Connection check - Clients: %d, Advertising: %s, Init: %d",
                          connectedClients,
                          advertising ? (advertising->isAdvertising() ? "Yes" : "No") : "Null",
                          initialized);
            lastLogTime = currentTime;
        }
        
        return connectedClients > 0;
    } catch (...) {
        // Catch any exceptions that might occur (defensive programming)
        SQUID_LOG_ERROR(LOG_TAG, "Exception in isConnected()");
        return false;
    }
}

void pollConnection(void * arg) {
    SQUIDHID * kb = static_cast<SQUIDHID*>(arg);
    if (!kb || !kb->ble) {
        return;
    }
    
    uint8_t cnt = kb->ble->getConnectedCount();

    if (kb->last_connected_count && !cnt) {
        SQUID_LOG_WARN(LOG_TAG, "Poller: link lost - restarting advertising");
        
        delay(100);
        
        if (kb->advertising) {
            if (!kb->advertising->isAdvertising()) {
                kb->advertising->start();
                SQUID_LOG_INFO(LOG_TAG, "Advertising restarted by poller");
            }
        }
    }
    kb->last_connected_count = cnt;
}

void SQUIDHID::onDisconnect(SquidServer *pServer) {
  #if GEMINIPR_ENABLE
    steno.setSerialConnected(false);
    SQUID_LOG_INFO(LOG_TAG, "SPP Serial disconnected");
  #endif
  // Restart advertising immediately when disconnected
  if (advertising) {
    advertising->start();
    SQUID_LOG_INFO(LOG_TAG, "Advertising restarted after disconnect");
  }
}

void SQUIDHID::onWrite(SquidCharacteristic* me) {
    #if GEMINIPR_ENABLE
    P_AUTO(uuid, me->getUUID());
    if (uuid && uuid->isEqualToString(SERIAL_CHARACTERISTIC_UUID_RX)) {
        std::string value = me->getValue();
        SQUID_LOG_DEBUG(LOG_TAG, "Received serial data: %s", value.c_str());
    } else {
        SQUID_LOG_DEBUG(LOG_TAG, "ESP-HID onWrite callback triggered");
        uint8_t* value = (uint8_t*)(me->getValue().c_str());
        size_t length = me->getValue().length();
        SQUID_LOG_DEBUG(LOG_TAG, "special keys: %d", *value);
    }
    #else
    SQUID_LOG_DEBUG(LOG_TAG, "ESP-HID onWrite callback triggered");
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    size_t length = me->getValue().length();
    SQUID_LOG_DEBUG(LOG_TAG, "special keys: %d", *value);
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
  
  #if GEMINIPR_ENABLE
  steno.releaseAll();
  #endif
  
  #if GAMEPAD_ENABLE
  gamepad.releaseAll();
  #endif
}

void SQUIDHID::setAppearance(uint16_t newAppearance) {
  this->appearance = newAppearance;
  
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
    this->batteryLevel = level;
    if (hid) {
        hid->setBatteryLevel(this->batteryLevel);
        
        if (isConnected()) {
            P_AUTO(batteryService, hid->getBatteryService());
            if (batteryService) {
                P_AUTO(batteryLevelChar, batteryService->getCharacteristic(0x2A19));
                if (batteryLevelChar) {
                    batteryLevelChar->setValue(&batteryLevel, 1);
                    batteryLevelChar->notify();
                }
            }
        }
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

#if GEMINIPR_ENABLE
size_t SQUIDHID::press(StenoKey stenoKey) { return steno.press(stenoKey); }

size_t SQUIDHID::release(StenoKey stenoKey) { return steno.release(stenoKey); }

void SQUIDHID::geminiStroke(const StenoKey* keys, size_t count) { steno.geminiStroke(keys, count); }

uint8_t SQUIDHID::stenoCharToKey(char c) { return steno.stenoCharToKey(c); }

void SQUIDHID::sendGeminiPRReport() { steno.sendGeminiPRReport(); }

void SQUIDHID::sendSerialData(const uint8_t* data, size_t length) { steno.sendSerialData(data, length); }

bool SQUIDHID::isSerialConnected() { return steno.isSerialConnected(); }
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
