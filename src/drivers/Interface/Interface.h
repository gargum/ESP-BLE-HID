/**
 * @file Interface.h
 * @brief Abstraction interface between the library and things like the NimBLE stack
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>
#include "../Event/Types.h"

// Forward declarations in correct order
class SquidUUID;
class SquidCharacteristicCallbacks;
class SquidServerCallbacks;
class SquidCharacteristic;
class SquidDescriptor;
class SquidService;
class SquidAdvertising;
class SquidAdvertisementData;
class SquidServer;
class SquidHIDDevice;
class SquidInterface;

// Property flags abstraction
enum class SquidProperty {
    READ = 0x01,
    WRITE = 0x02,
    NOTIFY = 0x04,
    INDICATE = 0x08,
    WRITE_NR = 0x10
};

// Bitwise operators for SquidProperty
inline SquidProperty operator|(SquidProperty a, SquidProperty b) {
    return static_cast<SquidProperty>(static_cast<int>(a) | static_cast<int>(b));
}

inline SquidProperty operator&(SquidProperty a, SquidProperty b) {
    return static_cast<SquidProperty>(static_cast<int>(a) & static_cast<int>(b));
}

inline SquidProperty operator^(SquidProperty a, SquidProperty b) {
    return static_cast<SquidProperty>(static_cast<int>(a) ^ static_cast<int>(b));
}

inline SquidProperty operator~(SquidProperty a) {
    return static_cast<SquidProperty>(~static_cast<int>(a));
}

inline SquidProperty& operator|=(SquidProperty& a, SquidProperty b) {
    a = a | b;
    return a;
}

inline SquidProperty& operator&=(SquidProperty& a, SquidProperty b) {
    a = a & b;
    return a;
}

inline SquidProperty& operator^=(SquidProperty& a, SquidProperty b) {
    a = a ^ b;
    return a;
}

// UUID abstraction
class SquidUUID {
public:
    virtual ~SquidUUID() = default;
    virtual std::string toString() const = 0;
    virtual bool equals(const SquidUUID& other) const = 0;
    virtual std::unique_ptr<SquidUUID> clone() const = 0;
    
    // Helper method to check if this is a specific UUID string
    virtual bool isEqualToString(const std::string& uuidStr) const {
        return toString() == uuidStr;
    }
    
    // Static factory methods
    static std::unique_ptr<SquidUUID> fromString(const std::string& uuid);
    static std::unique_ptr<SquidUUID> fromuint16_t(uint16_t uuid);
};

// Callback interfaces
class SquidCharacteristicCallbacks {
public:
    virtual ~SquidCharacteristicCallbacks() = default;
    virtual void onWrite(SquidCharacteristic* characteristic) = 0;
};

class SquidServerCallbacks {
public:
    virtual ~SquidServerCallbacks() = default;
    virtual void onConnect(SquidServer *pServer) = 0;
    virtual void onDisconnect(SquidServer *pServer) = 0;
    virtual void onStarted(SquidServer* pServer) { };
};

// Descriptor abstraction
class SquidDescriptor {
public:
    virtual ~SquidDescriptor() = default;
    virtual void setValue(const uint8_t* data, size_t length) = 0;
    virtual void setValue(const std::string& value) = 0;
    virtual void setValue(uint32_t value) = 0;
    virtual std::string getValue() const = 0;
    virtual std::unique_ptr<SquidUUID> getUUID() const = 0;
};

// Characteristic abstraction
class SquidCharacteristic {
public:
    virtual ~SquidCharacteristic() = default;
    virtual void setValue(const uint8_t* data, size_t length) = 0;
    virtual void setValue(const std::string& value) = 0;
    virtual void setValue(uint8_t value) = 0;
    virtual std::string getValue() const = 0;
    virtual bool notify() = 0;
    virtual void setCallbacks(SquidCharacteristicCallbacks* callbacks) = 0;
    virtual std::unique_ptr<SquidUUID> getUUID() const = 0;
    virtual SquidDescriptor* createDescriptor(const std::string& uuid, SquidProperty properties) = 0;
    virtual SquidDescriptor* createDescriptor(uint16_t uuid, SquidProperty properties) = 0;
};

// Service abstraction
class SquidService {
public:
    virtual ~SquidService() = default;
    
    // Factory methods return smart pointers
    virtual std::unique_ptr<SquidCharacteristic> createCharacteristic(const std::string& uuid, SquidProperty properties) = 0;
    virtual std::unique_ptr<SquidCharacteristic> createCharacteristic(uint16_t uuid, SquidProperty properties) = 0;
    
    // Getter methods return raw pointers (they don't transfer ownership)
    virtual SquidCharacteristic* getCharacteristic(const std::string& uuid) = 0;
    virtual SquidCharacteristic* getCharacteristic(uint16_t uuid) = 0;
    
    virtual void start() = 0;
    virtual std::unique_ptr<SquidUUID> getUUID() const = 0;
};

// Advertising abstraction
class SquidAdvertising {
public:
    virtual ~SquidAdvertising() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isAdvertising() const = 0;
    virtual void setAdvertisementData(SquidAdvertisementData& data) = 0;
    virtual void setScanResponseData(SquidAdvertisementData& data) = 0;
    virtual void setMinInterval(uint16_t interval) = 0;
    virtual void setMaxInterval(uint16_t interval) = 0;
};

// Advertisement Data abstraction
class SquidAdvertisementData {
public:
    virtual ~SquidAdvertisementData() = default;
    virtual void setFlags(uint8_t flags) = 0;
    virtual void setName(const std::string& name) = 0;
    virtual void setShortName(const std::string& name) = 0;
    virtual void setAppearance(uint16_t appearance) = 0;
    virtual void setManufacturerData(const std::string& data) = 0;
    virtual void addServiceUUID(const SquidUUID& uuid) = 0;
    virtual void setCompleteServices(const SquidUUID& uuid) = 0;
    // Size tracking methods
    virtual size_t getCurrentSize() const = 0;
    virtual size_t getMaxSize() const = 0;
    virtual bool canAddUUID(const SquidUUID& uuid) const = 0;
    virtual void clearServiceUUIDs() = 0;
    virtual void addUUIDWithPriority(const SquidUUID& uuid) = 0;
};

// Server abstraction
class SquidServer {
public:
    virtual ~SquidServer() = default;
    virtual void setCallbacks(SquidServerCallbacks* callbacks) = 0;
    virtual SquidService* createService(const std::string& uuid) = 0;
    virtual SquidService* createService(uint16_t uuid) = 0;
    virtual int getConnectedCount() const = 0;
    virtual std::unique_ptr<SquidUUID> getUUID() const = 0;  // Make sure this is declared
};

// HID Device abstraction
class SquidHIDDevice {
public:
    virtual ~SquidHIDDevice() = default;
    virtual void setManufacturer(const std::string& manufacturer) = 0;
    virtual void setHidInfo(uint8_t countryCode, uint8_t flags) = 0;
    virtual void setReportMap(const uint8_t* map, size_t length) = 0;
    virtual void startServices() = 0;
    virtual void setBatteryLevel(uint8_t level) = 0;
    
    // Factory methods return smart pointers
    virtual std::unique_ptr<SquidService> getHidService() = 0;
    virtual std::unique_ptr<SquidService> getBatteryService() = 0;
    
    // Getter methods return raw pointers (no ownership transfer)
    virtual SquidCharacteristic* getInputReport(uint8_t reportId) = 0;
    virtual SquidCharacteristic* getOutputReport(uint8_t reportId) = 0;
};

// Main BLE interface
class SquidInterface {
public:
    virtual ~SquidInterface() = default;
    
    // Device management
    virtual bool init(const std::string& deviceName) = 0;
    virtual void deinit(bool deleteStack = true) = 0;
    
    // Server management
    virtual SquidServer* createServer() = 0;
    virtual SquidServer* getServer() = 0;
    
    // Service management
    virtual SquidService* createService(const std::string& uuid) = 0;
    virtual SquidService* createService(uint16_t uuid) = 0;
    
    // Advertising
    virtual SquidAdvertising* getAdvertising() = 0;
    
    // HID Device creation
    virtual SquidHIDDevice* createHIDDevice(SquidServer* server) = 0;
    
    // Advertisement Data creation
    P_INTERFACE(SquidAdvertisementData, createAdvertisementData);
    
    // Security
    virtual void setSecurityAuth(bool bonding, bool mitm, bool secureConnections) = 0;
    virtual void setSecurityPasskey(uint32_t passkey) = 0;
    virtual void setSecurityIOCap(uint8_t iocap) = 0;
    
    // Power management
    virtual void setPower(int powerLevel) = 0;
    virtual void setMTU(uint16_t mtu) = 0;
    
    // Utility functions
    virtual int getConnectedCount() = 0;
    virtual std::string getAddress() = 0;
    virtual void setDeviceName(const std::string& name) = 0;
    virtual void setScanParams(uint16_t interval, uint16_t window, bool activeScan = true) = 0;
    
    // SPP/Serial specific
    virtual SquidService* createSerialService() = 0;
    
    // BLE helpers
    virtual bool startAdvertising(uint16_t appearance = 0x03C0, const std::string& deviceName = "", const std::string& manufacturer = "") = 0;
    
    virtual void stopAdvertising() = 0;
    
    virtual bool isAdvertising() = 0;
    
    // Battery service helper
    virtual void updateBatteryLevel(uint8_t level) = 0;
    
    // Connection management
    virtual void disconnect(uint16_t conn_id = 0) = 0;
    
    // GAP operations
    virtual void startScan(uint32_t duration = 0) = 0;
    virtual void stopScan() = 0;
};

// Factory for creating BLE implementations
class SquidFactory {
public:
    enum class Implementation {
        NIMBLE,
        ARDUINO_BLE,
        BLUEZ,
        CUSTOM
    };
    
    static std::unique_ptr<SquidInterface> create(Implementation impl);
};

#endif
