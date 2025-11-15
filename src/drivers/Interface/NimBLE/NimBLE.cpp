/**
 * @file NimBLE.cpp
 * @brief Allows the library to use the NimBLE stack
 */

#include "NimBLE.h"
#include "../../Log/Log.h"

// Convert SquidProperty to NimBLE properties
uint32_t squidPropertyToNimBLE(SquidProperty properties) {
    uint32_t result = 0;
    if (static_cast<int>(properties) & static_cast<int>(SquidProperty::READ)) result |= NIMBLE_PROPERTY::READ;
    if (static_cast<int>(properties) & static_cast<int>(SquidProperty::WRITE)) result |= NIMBLE_PROPERTY::WRITE;
    if (static_cast<int>(properties) & static_cast<int>(SquidProperty::NOTIFY)) result |= NIMBLE_PROPERTY::NOTIFY;
    if (static_cast<int>(properties) & static_cast<int>(SquidProperty::INDICATE)) result |= NIMBLE_PROPERTY::INDICATE;
    if (static_cast<int>(properties) & static_cast<int>(SquidProperty::WRITE_NR)) result |= NIMBLE_PROPERTY::WRITE_NR;
    return result;
}

// NimBLEServiceWrapper implementations
std::unique_ptr<SquidCharacteristic> NimBLEServiceWrapper::createCharacteristic(const std::string& uuid, SquidProperty properties) {
    auto nimChar = service->createCharacteristic(uuid, squidPropertyToNimBLE(properties));
    return std::make_unique<NimBLECharacteristicWrapper>(nimChar);
}

std::unique_ptr<SquidCharacteristic> NimBLEServiceWrapper::createCharacteristic(uint16_t uuid, SquidProperty properties) {
    auto nimChar = service->createCharacteristic(uuid, squidPropertyToNimBLE(properties));
    return std::make_unique<NimBLECharacteristicWrapper>(nimChar);
}

SquidCharacteristic* NimBLEServiceWrapper::getCharacteristic(const std::string& uuid) {
    auto nimChar = service->getCharacteristic(uuid);
    return nimChar ? new NimBLECharacteristicWrapper(nimChar) : nullptr;
}

SquidCharacteristic* NimBLEServiceWrapper::getCharacteristic(uint16_t uuid) {
    auto nimChar = service->getCharacteristic(uuid);
    return nimChar ? new NimBLECharacteristicWrapper(nimChar) : nullptr;
}

std::unique_ptr<SquidUUID> NimBLEServiceWrapper::getUUID() const {
    return std::make_unique<NimBLEUUIDWrapper>(service->getUUID());
}

// NimBLECharacteristicWrapper implementations
std::unique_ptr<SquidUUID> NimBLECharacteristicWrapper::getUUID() const {
    return std::make_unique<NimBLEUUIDWrapper>(characteristic->getUUID());
}

SquidDescriptor* NimBLECharacteristicWrapper::createDescriptor(const std::string& uuid, SquidProperty properties) {
    auto nimDesc = characteristic->createDescriptor(uuid, squidPropertyToNimBLE(properties));
    if (uuid == "2904") { // CCCD descriptor
        auto nim2904 = new NimBLE2904(characteristic);
        return new NimBLEDescriptorWrapper(nim2904);
    }
    return nullptr;
}

SquidDescriptor* NimBLECharacteristicWrapper::createDescriptor(uint16_t uuid, SquidProperty properties) {
    auto nimDesc = characteristic->createDescriptor(uuid, squidPropertyToNimBLE(properties));
    if (uuid == 0x2904) { // CCCD descriptor
        auto nim2904 = new NimBLE2904(characteristic);
        return new NimBLEDescriptorWrapper(nim2904);
    }
    return nullptr;
}

void NimBLECharacteristicWrapper::setCallbacks(SquidCharacteristicCallbacks* callbacks) {
    if (callbacks) {
        characteristic->setCallbacks(new NimBLECharacteristicCallbacksAdapter(callbacks, this));
    }
}

// NimBLEServerWrapper implementations  
void NimBLEServerWrapper::setCallbacks(SquidServerCallbacks* callbacks) {
    if (callbacks) {
        server->setCallbacks(new NimBLEServerCallbacksAdapter(callbacks, this));
    }
}

SquidService* NimBLEServerWrapper::createService(const std::string& uuid) {
    auto nimService = server->createService(uuid);
    return new NimBLEServiceWrapper(nimService);
}

SquidService* NimBLEServerWrapper::createService(uint16_t uuid) {
    auto nimService = server->createService(uuid);
    return new NimBLEServiceWrapper(nimService);
}

std::unique_ptr<SquidUUID> NimBLEServerWrapper::getUUID() const {
    return std::make_unique<NimBLEUUIDWrapper>(NimBLEUUID((uint16_t)0x1812));
}

// NimBLEAdvertisingWrapper implementations
void NimBLEAdvertisingWrapper::setAdvertisementData(SquidAdvertisementData& squidData) {
    auto* nimbleWrapper = static_cast<NimBLEAdvertisementDataWrapper*>(&squidData);
    if (nimbleWrapper) {
        advertising->setAdvertisementData(nimbleWrapper->getNimData());
    }
}

void NimBLEAdvertisingWrapper::setScanResponseData(SquidAdvertisementData& squidData) {
    auto* nimbleWrapper = static_cast<NimBLEAdvertisementDataWrapper*>(&squidData);
    if (nimbleWrapper) {
        advertising->setScanResponseData(nimbleWrapper->getNimData());
    }
}

// Main NimBLE interface implementations
bool NimBLE::init(const std::string& deviceName) {
    if (!initialized) {
        NimBLEDevice::init(deviceName);
        initialized = true;
    }
    return true;
}

void NimBLE::deinit(bool deleteStack) {
    NimBLEDevice::deinit(deleteStack);
    initialized = false;
}

SquidServer* NimBLE::createServer() {
    auto nimServer = NimBLEDevice::createServer();
    return new NimBLEServerWrapper(nimServer);
}

SquidServer* NimBLE::getServer() {
    auto nimServer = NimBLEDevice::getServer();
    return nimServer ? new NimBLEServerWrapper(nimServer) : nullptr;
}

SquidService* NimBLE::createService(const std::string& uuid) {
    auto server = getServer();
    return server ? server->createService(uuid) : nullptr;
}

SquidService* NimBLE::createService(uint16_t uuid) {
    auto server = getServer();
    return server ? server->createService(uuid) : nullptr;
}

SquidAdvertising* NimBLE::getAdvertising() {
    auto server = getServer();
    auto nimAdvertising = server ? NimBLEDevice::getAdvertising() : nullptr;
    return nimAdvertising ? new NimBLEAdvertisingWrapper(nimAdvertising) : nullptr;
}

SquidHIDDevice* NimBLE::createHIDDevice(SquidServer* server) {
    auto* nimbleServerWrapper = static_cast<NimBLEServerWrapper*>(server);
    if (nimbleServerWrapper && nimbleServerWrapper->getNimBLEServer()) {
        auto* hidDevice = new NimBLEHIDDevice(nimbleServerWrapper->getNimBLEServer());
        return new NimBLEHIDDeviceWrapper(hidDevice);
    }
    return nullptr;
}

std::unique_ptr<SquidAdvertisementData> NimBLE::createAdvertisementData() {
    return std::make_unique<NimBLEAdvertisementDataWrapper>();
}

void NimBLE::setSecurityAuth(bool bonding, bool mitm, bool secureConnections) {
    NimBLEDevice::setSecurityAuth(bonding, mitm, secureConnections);
}

int NimBLE::getConnectedCount() {
    auto server = getServer();
    return server ? server->getConnectedCount() : 0;
}

void NimBLE::setSecurityPasskey(uint32_t passkey) {
    NimBLEDevice::setSecurityPasskey(passkey);
}

void NimBLE::setSecurityIOCap(uint8_t iocap) {
    NimBLEDevice::setSecurityIOCap(iocap);
}

void NimBLE::setPower(int powerLevel) {
    // TODO: Fix this to be hardware-agnostic later when we add the hardware abstraction layers
    // Convert generic power level to ESP32 power level for now
    NimBLEDevice::setPower(static_cast<esp_power_level_t>(powerLevel));
}

void NimBLE::setMTU(uint16_t mtu) {
    NimBLEDevice::setMTU(mtu);
}

std::string NimBLE::getAddress() {
    return NimBLEDevice::getAddress().toString();
}

void NimBLE::setDeviceName(const std::string& name) {
    NimBLEDevice::setDeviceName(name);
}

void NimBLE::setScanParams(uint16_t interval, uint16_t window, bool activeScan) {
    // TODO: Implement this properly using NimBLEScan instead of leaving it unimplemented
    SQUID_LOG_DEBUG("NimBLE", "setScanParams not implemented yet");
}

SquidService* NimBLE::createSerialService() {
    auto server = getServer();
    if (!server) return nullptr;
    
    // Use the server wrapper to create the service
    return server->createService("00001101-0000-1000-8000-00805f9b34fb");
}

bool NimBLE::startAdvertising(uint16_t appearance, const std::string& deviceName, const std::string& manufacturer) {
    auto advertising = getAdvertising();
    if (!advertising) return false;
    
    auto adv = createAdvertisementData();
    if (!adv) return false;
    
    adv->setFlags(0x06);
    if (!deviceName.empty()) {
        adv->setName(deviceName);
        adv->setShortName(deviceName.substr(0, 8));
    }
    adv->setAppearance(appearance);
    if (!manufacturer.empty()) {
        adv->setManufacturerData(manufacturer);
    }
    
    advertising->setAdvertisementData(*adv);
    advertising->start();
    return true;
}

void NimBLE::stopAdvertising() {
    auto advertising = getAdvertising();
    if (advertising) {
        advertising->stop();
    }
}

bool NimBLE::isAdvertising() {
    auto advertising = getAdvertising();
    return advertising ? advertising->isAdvertising() : false;
}

void NimBLE::updateBatteryLevel(uint8_t level) {
    // NimBLE handles battery level through the HID device, not directly on NimBLEDevice
    // TODO: Implement this properly through the HID device wrapper
    SQUID_LOG_DEBUG("NimBLE", "updateBatteryLevel should be called on HID device");
}


void NimBLE::disconnect(uint16_t conn_id) {
    NimBLEDevice::getServer()->disconnect(conn_id);
}

void NimBLE::startScan(uint32_t duration) {
    auto scan = NimBLEDevice::getScan();
    if (scan) {
        scan->start(duration, false);
    }
}

void NimBLE::stopScan() {
    NimBLEDevice::getScan()->stop();
}
