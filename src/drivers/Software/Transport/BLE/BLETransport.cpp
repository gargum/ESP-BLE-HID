/**
 * @file BLETransport.cpp
 * @brief BLE transport implementation
 */

#include "BLETransport.h"

static const char* TRANSPORT_TAG = "BLETransport";

BLETransport::BLETransport() 
    : server(nullptr), hidDevice(nullptr), advertising(nullptr),
      transportCallbacks(nullptr), vid(0x046D), pid(0xC52B), version(0x0310),
      batteryLevel(100), appearance(KEYBOARD),
      initialized(false), connected(false),
      reportMap(nullptr), reportMapLength(0),
      inputNKRO(nullptr), 
      #if MEDIA_ENABLE
      inputMediaKeys(nullptr),
      #endif
      #if SPACEMOUSE_ENABLE
      inputSpacetrans(nullptr),
      inputSpacerotat(nullptr),
      inputSpaceclick(nullptr),
      #else
      #if MOUSE_ENABLE
      inputMouse(nullptr), 
      #endif
      #if DIGITIZER_ENABLE
      inputDigitizer(nullptr), 
      #endif
      #if GAMEPAD_ENABLE
      inputGamepad(nullptr),
      #endif
      #endif
      #if STENO_ENABLE
      inputSteno(nullptr), 
      #endif
      outputKeyboard(nullptr) { }

BLETransport::~BLETransport() {
    end();
}

bool BLETransport::begin() {
    if (initialized) {
        return true;
    }
    
    NimBLEDevice::init(deviceName.empty() ? "SquidHID" : deviceName.c_str());
    NimBLEDevice::setSecurityAuth(true, true, true);
    
    server = NimBLEDevice::createServer();
    server->setCallbacks(this);
    
    // Create HID device
    hidDevice = new NimBLEHIDDevice(server);
    hidDevice->setManufacturer(deviceManufacturer.empty() ? "SquidHID" : deviceManufacturer.c_str());
    hidDevice->setHidInfo(0x11, 0x01);
    hidDevice->setBatteryLevel(batteryLevel);
    
    // Create HID Service with characteristics
    createHIDService();
    
    // Set PnP ID and create Device Information Service
    uint8_t pnpId[7] = {
        0x02, // USB-IF
        static_cast<uint8_t>(vid & 0xFF), static_cast<uint8_t>(vid >> 8),
        static_cast<uint8_t>(pid & 0xFF), static_cast<uint8_t>(pid >> 8),
        static_cast<uint8_t>(version & 0xFF), static_cast<uint8_t>(version >> 8)
    };
    
    BLEService* disService = server->createService(NimBLEUUID((uint16_t)0x180A));
    disService->createCharacteristic((uint16_t)0x2A29, NIMBLE_PROPERTY::READ)
              ->setValue(deviceManufacturer.c_str());
    disService->createCharacteristic((uint16_t)0x2A24, NIMBLE_PROPERTY::READ)
              ->setValue(deviceName.c_str());
    disService->createCharacteristic((uint16_t)0x2A50, NIMBLE_PROPERTY::READ)
              ->setValue(pnpId, 7);
    disService->start();
    
    advertising = server->getAdvertising();
    initialized = true;
    
    return true;
}

void BLETransport::end() {
    if (advertising) {
        advertising->stop();
    }
    
    if (server) {
        // NimBLE doesn't have deleteServer() - I'm just gonna deinit the device
        // The server will be cleaned up when NimBLEDevice is deinitialized maybe? 
        server = nullptr;
    }
    
    NimBLEDevice::deinit(true);
    initialized = false;
    connected = false;
}

void BLETransport::update() {
    static uint32_t lastStateCheck = 0;
    uint32_t currentTime = millis();
    
    // Check connection state periodically
    if (currentTime - lastStateCheck >= 1000) {
        lastStateCheck = currentTime;
        bool currentConnected = server && server->getConnectedCount() > 0;
        
        if (currentConnected != connected) {
            connected = currentConnected;
            SQUID_LOG_INFO(TRANSPORT_TAG, "Connection state: %s", 
                          connected ? "connected" : "disconnected");
        }
    }
}

bool BLETransport::isConnected() {
    bool bleConnected = server && server->getConnectedCount() > 0;
    
    // Force state synchronization
    if (bleConnected != connected) {
        connected = bleConnected;
        SQUID_LOG_INFO(TRANSPORT_TAG, "Connection state changed to: %s", 
                      connected ? "connected" : "disconnected");
    }
    
    return connected;
}

bool BLETransport::connect() {
    // For BLE, connection is initiated by the client it appears
    return startAdvertising();
}

void BLETransport::disconnect() {
    if (server) {
        // Haven't figured out how to force disconnection for all clients without causing significant issues yet, so this is just empty rn
    }
}

void BLETransport::createHIDService() {
    if (!hidDevice) {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "HID device not initialized");
        return;
    }
    
    SQUID_LOG_INFO(TRANSPORT_TAG, "Creating HID service - following working sequence");
    
    // Set report map first
    if (reportMap && reportMapLength > 0) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Setting HID Report Map - Length: %zu", reportMapLength);
        hidDevice->setReportMap(const_cast<uint8_t*>(reportMap), reportMapLength);
    }
    
    // Create input reports for each HID device type
    SQUID_LOG_DEBUG(TRANSPORT_TAG, "Creating HID input reports...");
    
    outputKeyboard = hidDevice->getOutputReport(0x01);     // Status LEDs
    inputNKRO = hidDevice->getInputReport(0x01);           // NKRO keyboard  
    #if MEDIA_ENABLE
    inputMediaKeys = hidDevice->getInputReport(0x03);      // Media keys
    #endif
    #if SPACEMOUSE_ENABLE
    inputSpacetrans = hidDevice->getInputReport(0x04);     // Spacemouse translations
    inputSpacerotat = hidDevice->getInputReport(0x05);     // Spacemouse rotations
    inputSpaceclick = hidDevice->getInputReport(0x06);     // Spacemouse buttons
    #else
    #if MOUSE_ENABLE
    inputMouse = hidDevice->getInputReport(0x07);          // Mouse
    #endif
    #if DIGITIZER_ENABLE
    inputDigitizer = hidDevice->getInputReport(0x08);      // Digitizer
    #endif
    #if GAMEPAD_ENABLE
    inputGamepad = hidDevice->getInputReport(0x09);        // Gamepad
    #endif
    #endif
    #if STENO_ENABLE
    inputSteno = hidDevice->getInputReport(0x50);          // Plover HID steno
    #endif
    
    // Set callbacks for ALL characteristics
    if (outputKeyboard) {
        outputKeyboard->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Keyboard Output characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Keyboard Output characteristic creation failed!");
    }
    
    if (inputNKRO) {
        inputNKRO->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "NKRO characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "NKRO Input characteristic creation failed!");
    }
    
    #if MEDIA_ENABLE
    if (inputMediaKeys) {
        inputMediaKeys->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Media keys characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Media Keys Input characteristic creation failed!");
    }
    #endif
    
    #if SPACEMOUSE_ENABLE    
    if (inputSpacetrans) {
        inputSpacetrans->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Spacemouse translations characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Spacemouse Translations Input characteristic creation failed!");
    }
    
    if (inputSpacerotat) {
        inputSpacerotat->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Spacemouse rotations characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Spacemouse Rotations Input characteristic creation failed!");
    }
    
    if (inputSpaceclick) {
        inputSpaceclick->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Spacemouse buttons characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Spacemouse Buttons Input characteristic creation failed!");
    }
    #else
    
    #if MOUSE_ENABLE
    if (inputMouse) {
        inputMouse->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Mouse characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Mouse Input characteristic creation failed!");
    }
    #endif
    
    #if DIGITIZER_ENABLE
    if (inputDigitizer) {
        inputDigitizer->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Digitizer characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Digitizer Input characteristic creation failed!");
    }
    #endif
    
    #if GAMEPAD_ENABLE
    if (inputGamepad) {
        inputGamepad->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Gamepad characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Gamepad Input characteristic creation failed!");
    }
    #endif
    #endif
    
    #if STENO_ENABLE
    if (inputSteno) {
        inputSteno->setCallbacks(this);
        SQUID_LOG_INFO(TRANSPORT_TAG, "Steno characteristic created");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Steno Input characteristic creation failed!");
    }
    #endif
    
    // Start HID services AFTER creating all characteristics
    SQUID_LOG_DEBUG(TRANSPORT_TAG, "Starting HID services...");
    hidDevice->startServices();
    
    // Wait a moment for services to be fully initialized
    delay(50);
    
    // Verify all characteristics have valid handles
    verifyCharacteristicHandles();
    
    // Debug output
    debugCharacteristics();
}

void BLETransport::verifyCharacteristicHandles() {
    std::vector<std::pair<NimBLECharacteristic*, const char*>> characteristics = {
        {inputNKRO, "NKRO Input"},
        #if MEDIA_ENABLE
        {inputMediaKeys, "Media Keys Input"},
        #endif
        #if SPACEMOUSE_ENABLE
        {inputSpacetrans, "Spacemouse Translations Input"},
        {inputSpacerotat, "Spacemouse Rotations Input"},
        {inputSpaceclick, "Spacemouse Buttons Input"},
        #else
        #if MOUSE_ENABLE
        {inputMouse, "Mouse Input"},
        #endif
        #if DIGITIZER_ENABLE
        {inputDigitizer, "Digitizer Input"},
        #endif
        #if GAMEPAD_ENABLE
        {inputGamepad, "Gamepad Input"},
        #endif
        #endif
        #if STENO_ENABLE
        {inputSteno, "Steno Input"},
        #endif
        {outputKeyboard, "Keyboard Output"}
    };
    
    bool allValid = true;
    
    for (auto& [charac, name] : characteristics) {
        if (charac) {
            uint16_t handle = charac->getHandle();
            if (handle == 0) {
                SQUID_LOG_ERROR(TRANSPORT_TAG, "%s characteristic has INVALID handle (0)!", name);
                allValid = false;
            } else {
                SQUID_LOG_INFO(TRANSPORT_TAG, "%s characteristic handle: %d, UUID: %s", 
                             name, handle, charac->getUUID().toString().c_str());
            }
        } else {
            SQUID_LOG_ERROR(TRANSPORT_TAG, "%s characteristic is NULL", name);
            allValid = false;
        }
    }
    
    if (allValid) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "All HID characteristics created successfully!");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Some HID characteristics failed to create properly");
    }
}

void BLETransport::debugCharacteristics() {
    SQUID_LOG_INFO(TRANSPORT_TAG, "=== Characteristic Debug ===");
    SQUID_LOG_INFO(TRANSPORT_TAG, "HID Device: %s", hidDevice ? "VALID" : "NULL");
    SQUID_LOG_INFO(TRANSPORT_TAG, "HID Service: %s", hidDevice && hidDevice->getHidService() ? "VALID" : "NULL");
    
    std::vector<std::pair<NimBLECharacteristic*, const char*>> characteristics = {
        {inputNKRO, "NKRO Input"},
        #if MEDIA_ENABLE
        {inputMediaKeys, "Media Keys Input"},
        #endif
        #if SPACEMOUSE_ENABLE
        {inputSpacetrans, "Spacemouse Translations Input"},
        {inputSpacerotat, "Spacemouse Rotations Input"},
        {inputSpaceclick, "Spacemouse Buttons Input"},
        #else
        #if MOUSE_ENABLE
        {inputMouse, "Mouse Input"},
        #endif
        #if DIGITIZER_ENABLE
        {inputDigitizer, "Digitizer Input"},
        #endif
        #if GAMEPAD_ENABLE
        {inputGamepad, "Gamepad Input"},
        #endif
        #endif
        #if STENO_ENABLE
        {inputSteno, "Steno Input"},
        #endif
        {outputKeyboard, "Keyboard Output"}
    };
    
    for (auto& [charac, name] : characteristics) {
        if (charac) {
            SQUID_LOG_INFO(TRANSPORT_TAG, "%s: Handle=%d, UUID=%s", 
                         name, 
                         charac->getHandle(),
                         charac->getUUID().toString().c_str());
        } else {
            SQUID_LOG_ERROR(TRANSPORT_TAG, "%s: NULL", name);
        }
    }
    SQUID_LOG_INFO(TRANSPORT_TAG, "=== End Debug ===");
}

bool BLETransport::sendData(const uint8_t* data, size_t length) {
    // Generic data transmission - I nuked SPP so this does nothing rn
    SQUID_LOG_DEBUG(TRANSPORT_TAG, "Generic data send - Length: %zu", length); // Random debug line that I don't think can ever be triggered
    return true; // This is here because it threw a compiler error before I had it here
}

bool BLETransport::sendReport(uint8_t reportId, const uint8_t* data, size_t length) {
    if (!isConnected()) {
        SQUID_LOG_DEBUG(TRANSPORT_TAG, "Cannot send report - not connected");
        return false;
    }
    
    NimBLECharacteristic* characteristic = nullptr;
    const char* charName = "Unknown";
    
    // Map report IDs to characteristics
    switch (reportId) {
        case 0x01: characteristic = inputNKRO; charName = "NKRO"; break;
        #if MEDIA_ENABLE
        case 0x03: characteristic = inputMediaKeys; charName = "Media Keys"; break;
        #endif
        #if SPACEMOUSE_ENABLE
        case 0x04: characteristic = inputSpacetrans; charName = "Spacetrans"; break;
        case 0x05: characteristic = inputSpacerotat; charName = "Spacerotat"; break;
        case 0x06: characteristic = inputSpaceclick; charName = "Spaceclick"; break;
        #else
        #if MOUSE_ENABLE
        case 0x07: characteristic = inputMouse; charName = "Mouse"; break;
        #endif
        #if DIGITIZER_ENABLE
        case 0x08: characteristic = inputDigitizer; charName = "Digitizer"; break;
        #endif
        #if GAMEPAD_ENABLE
        case 0x09: characteristic = inputGamepad; charName = "Gamepad"; break;
        #endif
        #endif
        #if STENO_ENABLE
        case 0x50: characteristic = inputSteno; charName = "Steno"; break;
        #endif
        default:
            SQUID_LOG_WARN(TRANSPORT_TAG, "Unknown report ID: %d", reportId);
            return false;
    }
    
    if (!characteristic) {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Characteristic for %s (ID %d) is NULL!", charName, reportId);
        return false;
    }
    
    if (characteristic->getHandle() == 0) {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Characteristic for %s (ID %d) has invalid handle 0!", charName, reportId);
        return false;
    }
    
    // Set the report value and notify
    characteristic->setValue(const_cast<uint8_t*>(data), length);
    bool result = characteristic->notify();
    
    if (result) {
        SQUID_LOG_DEBUG(TRANSPORT_TAG, "Report sent successfully - %s (ID %d), Length: %zu", 
                       charName, reportId, length);
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Failed to send report - %s (ID %d), Length: %zu", 
                       charName, reportId, length);
    }
    
    return result;
}

void BLETransport::setDeviceInfo(const char* name, const char* manufacturer, 
                                uint16_t vid, uint16_t pid, uint16_t version) {
    this->deviceName = name ? name : "";
    this->deviceManufacturer = manufacturer ? manufacturer : "";
    this->vid = vid;
    this->pid = pid;
    this->version = version;
}

void BLETransport::setBatteryLevel(uint8_t level) {
    // Clamp battery level to 0-100 in case other values break something
    uint8_t oldLevel = this->batteryLevel;
    this->batteryLevel = (level > 100) ? 100 : level;
    
    // Update HID device battery level
    if (hidDevice) {
        hidDevice->setBatteryLevel(this->batteryLevel);
        
        // Force battery service update if connected
        if (isConnected()) {
            // Get the battery service and characteristic directly
            BLEService* batteryService = hidDevice->getBatteryService();
            if (batteryService) {
                BLECharacteristic* batteryLevelChar = batteryService->getCharacteristic((uint16_t)0x2A19);
                if (batteryLevelChar) {
                    // Update the characteristic value
                    batteryLevelChar->setValue(&batteryLevel, 1);
                    // Force notification
                    batteryLevelChar->notify();
                    
                    SQUID_LOG_DEBUG(TRANSPORT_TAG, "Battery level forced update: %d%%", this->batteryLevel);
                }
            }
        }
    }
    
    if (this->batteryLevel != oldLevel) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Battery level changed: %d%% -> %d%%", oldLevel, this->batteryLevel);
    }
}

void BLETransport::setCallbacks(TransportCallbacks* callbacks) {
    this->transportCallbacks = callbacks;
}

void BLETransport::setReportMap(const uint8_t* descriptor, size_t length) {
    // Store the report map for use in begin()
    this->reportMap = descriptor;
    this->reportMapLength = length;
    
    SQUID_LOG_INFO(TRANSPORT_TAG, "Report map stored - Length: %zu", length);
}

void BLETransport::setAppearance(uint16_t newAppearance) {
    this->appearance = newAppearance;
    SQUID_LOG_INFO(TRANSPORT_TAG, "Appearance set to: 0x%04X", appearance);
    
    // If advertising is already running, we need to restart it with new appearance
    if (advertising && advertising->isAdvertising()) {
        SQUID_LOG_DEBUG(TRANSPORT_TAG, "Restarting advertising with new appearance");
        startAdvertising();
    }
}

bool BLETransport::startAdvertising() {
    if (!advertising) {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "No advertising object available");
        return false;
    }
    
    // Stop any existing advertising first
    advertising->stop();
    delay(50);
    
    // Configure advertising parameters
    advertising->setMinInterval(32);
    advertising->setMaxInterval(48);
    
    // Create advertisement data
    BLEAdvertisementData advData;
    BLEAdvertisementData scanResponseData;
    
    // Main advertisement data
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.setAppearance(appearance);
    
    // Scan response data - include name and services
    scanResponseData.setName(deviceName.c_str());
    
    // Add HID service (battery service is included automatically by HID device)
    if (hidDevice && hidDevice->getHidService()) {
        scanResponseData.addServiceUUID(hidDevice->getHidService()->getUUID());
        advData.addServiceUUID(hidDevice->getHidService()->getUUID());
    }
    
    // Set the advertisement data
    advertising->setAdvertisementData(advData);
    advertising->setScanResponseData(scanResponseData);
    
    // Start advertising
    bool result = advertising->start();
    if (result) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "BLE advertising started");
    } else {
        SQUID_LOG_ERROR(TRANSPORT_TAG, "Failed to start BLE advertising");
    }
    
    return result;
}

// BLE Callbacks
void BLETransport::onConnect(NimBLEServer* pServer) {
    connected = true;
    
    SQUID_LOG_INFO(TRANSPORT_TAG, "Client connected - Connection count: %d", pServer->getConnectedCount());
    
    // Notify all HID characteristics
    if (inputNKRO && inputNKRO->getHandle() != 0) {
        inputNKRO->notify();
    }
    
    #if MEDIA_ENABLE
    if (inputMediaKeys && inputMediaKeys->getHandle() != 0) {
        inputMediaKeys->notify();
    }
    #endif
    
    #if SPACEMOUSE_ENABLE
    if (inputSpacetrans && inputSpacetrans->getHandle() != 0) {
        inputSpacetrans->notify();
    }
    if (inputSpacerotat && inputSpacerotat->getHandle() != 0) {
        inputSpacerotat->notify();
    }
    if (inputSpaceclick && inputSpaceclick->getHandle() != 0) {
        inputSpaceclick->notify();
    }
    #else
    
    #if MOUSE_ENABLE
    if (inputMouse && inputMouse->getHandle() != 0) {
        inputMouse->notify();
    }
    #endif
    
    #if DIGITIZER_ENABLE
    if (inputDigitizer && inputDigitizer->getHandle() != 0) {
        inputDigitizer->notify();
    }
    #endif
    
    #if GAMEPAD_ENABLE
    if (inputGamepad && inputGamepad->getHandle() != 0) {
        inputGamepad->notify();
    }
    #endif
    #endif
    
    #if STENO_ENABLE
    if (inputSteno && inputSteno->getHandle() != 0) {
        inputSteno->notify();
    }
    #endif
    
    if (transportCallbacks) {
        transportCallbacks->onConnect();
    }
}

void BLETransport::onDisconnect(NimBLEServer* pServer) {
    connected = false;
    SQUID_LOG_INFO(TRANSPORT_TAG, "Client disconnected");
    
    if (transportCallbacks) {
        transportCallbacks->onDisconnect();
    }
    
    // Restart advertising when disconnected
    if (advertising) {
        advertising->start();
        SQUID_LOG_INFO(TRANSPORT_TAG, "Advertising restarted after disconnect");
    }
}

void BLETransport::onWrite(NimBLECharacteristic* characteristic) {
    SQUID_LOG_DEBUG(TRANSPORT_TAG, "Characteristic write - Handle: %d, UUID: %s", 
                   characteristic->getHandle(), characteristic->getUUID().toString().c_str());
    
    // Specifically handle keyboard output reports (LED status and nothing else currently)
    if (characteristic == outputKeyboard) {
        auto value = characteristic->getValue();
        SQUID_LOG_INFO(TRANSPORT_TAG, "Keyboard output report received - Length: %zu", value.length());
        
        // Logging the raw data for debugging
        std::string hexStr;
        for (size_t i = 0; i < value.length(); i++) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X ", value[i]);
            hexStr += buf;
        }
        SQUID_LOG_DEBUG(TRANSPORT_TAG, "Output report data: %s", hexStr.c_str());
    }
    
    if (transportCallbacks) {
        auto value = characteristic->getValue();
        transportCallbacks->onDataReceived(
            reinterpret_cast<const uint8_t*>(value.data()), 
            value.length()
        );
    }
}

void BLETransport::onSubscribe(NimBLEServer* pServer, ble_gap_conn_desc* desc, uint16_t attr_handle) {
    SQUID_LOG_INFO(TRANSPORT_TAG, "Subscribe event - attr_handle: %d", attr_handle);
    
    // Check which characteristic was subscribed by comparing handles
    if (outputKeyboard) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Subscribing to output reports isn't really a thing."); 
    } else if (inputNKRO && inputNKRO->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "NKRO report subscribed");
    #if MEDIA_ENABLE
    } else if (inputMediaKeys && inputMediaKeys->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Media keys report subscribed");
    #endif
    #if SPACEMOUSE_ENABLE 
    } else if (inputSpacetrans && inputSpacetrans->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Spacemouse translations report subscribed");
    } else if (inputSpacerotat && inputSpacerotat->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Spacemouse rotations report subscribed");
    } else if (inputSpaceclick && inputSpaceclick->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Spacemouse buttons report subscribed");
    #else
    #if MOUSE_ENABLE
    } else if (inputMouse && inputMouse->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Mouse report subscribed");
    #endif
    #if DIGITIZER_ENABLE
    } else if (inputDigitizer && inputDigitizer->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Digitizer report subscribed");
    #endif
    #if GAMEPAD_ENABLE
    } else if (inputGamepad && inputGamepad->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Gamepad report subscribed");
    #endif
    #endif
    #if STENO_ENABLE
    } else if (inputSteno && inputSteno->getHandle() == attr_handle) {
        SQUID_LOG_INFO(TRANSPORT_TAG, "Steno report subscribed");
    #endif
    } else {
        SQUID_LOG_DEBUG(TRANSPORT_TAG, "Unknown characteristic subscribed, handle: %d", attr_handle);
    }
}

void BLETransport::onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
    SQUID_LOG_INFO(TRANSPORT_TAG, "MTU changed to: %d", MTU);
}
