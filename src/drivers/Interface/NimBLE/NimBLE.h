/**
 * @file NimBLE.h
 * @brief Allows the library to use the NimBLE stack
 */

#ifndef NIMBLE_H
#define NIMBLE_H

#include "../Interface.h"
#include "NimBLEDevice.h"
#include "NimBLEServer.h"
#include "NimBLEService.h"
#include "NimBLEAdvertising.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"
#include "NimBLEUUID.h"
#include "NimBLEUtils.h"

uint32_t squidPropertyToNimBLE(SquidProperty properties);

// Forward declare the actual implementation
class NimBLE;

class NimBLEUUIDWrapper : public SquidUUID {
private:
    NimBLEUUID uuid;
public:
    NimBLEUUIDWrapper(const NimBLEUUID& nimUUID) : uuid(nimUUID) {}
    NimBLEUUIDWrapper(const std::string& uuidStr) : uuid(uuidStr) {}
    NimBLEUUIDWrapper(uint16_t uuid16) : uuid(uuid16) {}
    
    std::string toString() const override { return uuid.toString(); }
    bool equals(const SquidUUID& other) const override {
        // Safe cast since we control all SquidUUID implementations
        const auto& otherNimble = static_cast<const NimBLEUUIDWrapper&>(other);
        return uuid.equals(otherNimble.uuid);
    }
    std::unique_ptr<SquidUUID> clone() const override { 
        return std::make_unique<NimBLEUUIDWrapper>(uuid); 
    }
    
    const NimBLEUUID& getNimBLEUUID() const { return uuid; }
};

class NimBLECharacteristicWrapper : public SquidCharacteristic {
private:
    NimBLECharacteristic* characteristic;
public:
    NimBLECharacteristicWrapper(NimBLECharacteristic* charac) : characteristic(charac) {}
    
    void setValue(const uint8_t* data, size_t length) override { characteristic->setValue(data, length); }
    void setValue(const std::string& value) override { characteristic->setValue(value); }
    void setValue(uint8_t value) override { characteristic->setValue(&value, 1); }
    std::string getValue() const override { return characteristic->getValue(); }
    bool notify() override { return characteristic->notify(); }
    void setCallbacks(SquidCharacteristicCallbacks* callbacks) override;
    std::unique_ptr<SquidUUID> getUUID() const override;
    SquidDescriptor* createDescriptor(const std::string& uuid, SquidProperty properties) override;
    SquidDescriptor* createDescriptor(uint16_t uuid, SquidProperty properties) override;
};

class NimBLEServiceWrapper : public SquidService {
private:
    NimBLEService* service;
public:
    NimBLEServiceWrapper(NimBLEService* serv) : service(serv) {}
    
    // DECLARE the methods here, but implement in .cpp file
    std::unique_ptr<SquidCharacteristic> createCharacteristic(const std::string& uuid, SquidProperty properties) override;
    std::unique_ptr<SquidCharacteristic> createCharacteristic(uint16_t uuid, SquidProperty properties) override;
    
    // Getter methods return raw pointers
    SquidCharacteristic* getCharacteristic(const std::string& uuid) override;
    SquidCharacteristic* getCharacteristic(uint16_t uuid) override;
    
    void start() override { service->start(); }
    std::unique_ptr<SquidUUID> getUUID() const override;
};


class NimBLEServerWrapper : public SquidServer {
private:
    NimBLEServer* server;
public:
    NimBLEServerWrapper(NimBLEServer* serv) : server(serv) {}
    NimBLEServer* getNimBLEServer() { return server; }
    void setCallbacks(SquidServerCallbacks* callbacks) override;
    SquidService* createService(const std::string& uuid) override;
    SquidService* createService(uint16_t uuid) override;
    int getConnectedCount() const override { return server->getConnectedCount(); }
    std::unique_ptr<SquidUUID> getUUID() const override;
};

class NimBLEAdvertisingWrapper : public SquidAdvertising {
private:
    NimBLEAdvertising* advertising;
public:
    NimBLEAdvertisingWrapper(NimBLEAdvertising* adv) : advertising(adv) {}
    
    void start() override { advertising->start(); }
    void stop() override { advertising->stop(); }
    bool isAdvertising() const override { return advertising->isAdvertising(); }
    void setAdvertisementData(SquidAdvertisementData& data) override;
    void setScanResponseData(SquidAdvertisementData& data) override;
    void setMinInterval(uint16_t interval) override { advertising->setMinInterval(interval); }
    void setMaxInterval(uint16_t interval) override { advertising->setMaxInterval(interval); }
};

class NimBLEHIDDeviceWrapper : public SquidHIDDevice {
private:
    NimBLEHIDDevice* hidDevice;
public:
    NimBLEHIDDeviceWrapper(NimBLEHIDDevice* hid) : hidDevice(hid) {}
    NimBLEHIDDevice* getNimHIDDevice() { return hidDevice; }
    
    void setManufacturer(const std::string& manufacturer) override { hidDevice->setManufacturer(manufacturer); }
    void setHidInfo(uint8_t countryCode, uint8_t flags) override { hidDevice->setHidInfo(countryCode, flags); }
    void setReportMap(const uint8_t* map, size_t length) override { hidDevice->setReportMap(const_cast<uint8_t*>(map), length); }
    void startServices() override { hidDevice->startServices(); }
    void setBatteryLevel(uint8_t level) override { hidDevice->setBatteryLevel(level); }
    
    // Factory methods return smart pointers
    std::unique_ptr<SquidService> getHidService() override {
        auto nimService = hidDevice->getHidService();
        return std::make_unique<NimBLEServiceWrapper>(nimService);
    }
    
    std::unique_ptr<SquidService> getBatteryService() override {
        auto nimService = hidDevice->getBatteryService();
        return std::make_unique<NimBLEServiceWrapper>(nimService);
    }
    
    // Getter methods return raw pointers
    SquidCharacteristic* getInputReport(uint8_t reportId) override {
        auto nimChar = hidDevice->getInputReport(reportId);
        return nimChar ? new NimBLECharacteristicWrapper(nimChar) : nullptr;
    }
    
    SquidCharacteristic* getOutputReport(uint8_t reportId) override {
        auto nimChar = hidDevice->getOutputReport(reportId);
        return nimChar ? new NimBLECharacteristicWrapper(nimChar) : nullptr;
    }
};

class NimBLE : public SquidInterface {
private:
    bool initialized = false;
    
public:
    bool init(const std::string& deviceName) override;
    void deinit(bool deleteStack = true) override;
    
    SquidServer* createServer() override;
    SquidServer* getServer() override;
    
    SquidService* createService(const std::string& uuid) override;
    SquidService* createService(uint16_t uuid) override;
    
    SquidAdvertising* getAdvertising() override;
    SquidHIDDevice* createHIDDevice(SquidServer* server) override;
    std::unique_ptr<SquidAdvertisementData> createAdvertisementData() override;
    
    void setSecurityAuth(bool bonding, bool mitm, bool secureConnections) override;
    void setSecurityPasskey(uint32_t passkey) override;
    void setSecurityIOCap(uint8_t iocap) override;
    void setPower(int powerLevel) override;
    void setMTU(uint16_t mtu) override;
    int getConnectedCount() override;
    std::string getAddress() override;
    void setDeviceName(const std::string& name) override;
    void setScanParams(uint16_t interval, uint16_t window, bool activeScan = true) override;
    SquidService* createSerialService() override;
    
    bool startAdvertising(uint16_t appearance = 0x03C0, const std::string& deviceName = "", const std::string& manufacturer = "") override;
    void stopAdvertising() override;
    bool isAdvertising() override;
    void updateBatteryLevel(uint8_t level) override;
    void disconnect(uint16_t conn_id = 0) override;
    void startScan(uint32_t duration = 0) override;
    void stopScan() override;
};

class NimBLEServerCallbacksAdapter : public NimBLEServerCallbacks {
private:
    SquidServerCallbacks* squidCallbacks;
    SquidServer* squidServer;
public:
    NimBLEServerCallbacksAdapter(SquidServerCallbacks* callbacks, SquidServer* server) : squidCallbacks(callbacks), squidServer(server) {}
    
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        if (squidCallbacks) squidCallbacks->onConnect(squidServer);
    }
    
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
        if (squidCallbacks) squidCallbacks->onDisconnect(squidServer);
    }

};

class NimBLECharacteristicCallbacksAdapter : public NimBLECharacteristicCallbacks {
private:
    SquidCharacteristicCallbacks* squidCallbacks;
    SquidCharacteristic* squidCharacteristic;
public:
    NimBLECharacteristicCallbacksAdapter(SquidCharacteristicCallbacks* callbacks, SquidCharacteristic* characteristic) 
        : squidCallbacks(callbacks), squidCharacteristic(characteristic) {}
    
    void onWrite(NimBLECharacteristic* characteristic, ble_gap_conn_desc* desc) {
        if (squidCallbacks) squidCallbacks->onWrite(squidCharacteristic);
    }
};

class NimBLEAdvertisementDataWrapper : public SquidAdvertisementData {
private:
    BLEAdvertisementData data;
public:
    void setFlags(uint8_t flags) override { data.setFlags(flags); }
    void setName(const std::string& name) override { data.setName(name); }
    void setShortName(const std::string& name) override { data.setShortName(name); }
    void setAppearance(uint16_t appearance) override { data.setAppearance(appearance); }
    void setManufacturerData(const std::string& manufacturer) override {
        std::vector<uint8_t> manufData(manufacturer.begin(), manufacturer.end());
        data.setManufacturerData(manufData);
    }
    void addServiceUUID(const SquidUUID& uuid) override {
        std::string uuidStr = uuid.toString();
        NimBLEUUID nimbleUUID(uuidStr);
        data.addServiceUUID(nimbleUUID);
    }
    void setCompleteServices(const SquidUUID& uuid) override { addServiceUUID(uuid); }
    void addUUIDWithPriority(const SquidUUID& uuid) override { addServiceUUID(uuid); } // Just delegate to addServiceUUID - no prioritization needed
    const BLEAdvertisementData& getNimData() const { return data; }
    BLEAdvertisementData& getNimData() { return data; }
    
    // Stub out interface methods
    size_t getCurrentSize() const override { return 0; }
    size_t getMaxSize() const override { return 31; }
    bool canAddUUID(const SquidUUID& uuid) const override { return true; }
    void clearServiceUUIDs() override { data = BLEAdvertisementData(); }
};

class NimBLEDescriptorWrapper : public SquidDescriptor {
private:
    NimBLE2904* descriptor;
public:
    NimBLEDescriptorWrapper(NimBLE2904* desc) : descriptor(desc) {}
    
    void setValue(const uint8_t* data, size_t length) override { descriptor->setValue(data, length); }
    void setValue(const std::string& value) override { descriptor->setValue(value); }
    void setValue(uint32_t value) override { descriptor->setValue(value); }
    std::string getValue() const override { return descriptor->getValue(); }
    std::unique_ptr<SquidUUID> getUUID() const override { return std::make_unique<NimBLEUUIDWrapper>(descriptor->getUUID()); }
    
    NimBLE2904* getNimDescriptor() { return descriptor; }
};



#endif
