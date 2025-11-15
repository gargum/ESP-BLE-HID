/**
 * @file Interface.cpp
 * @brief Implementation of the wrapper for the abstraction layer between the library and things like the NimBLE stack
 */

#include "Interface.h"
#include "NimBLE/NimBLE.h"
// Future: #include "ArduinoBLEImplementation.h"
// Future: #include "BlueZImplementation.h"

std::unique_ptr<SquidInterface> SquidFactory::create(Implementation impl) {
    switch (impl) {
        case Implementation::NIMBLE:
            return std::make_unique<NimBLE>();
        default:
            return std::make_unique<NimBLE>();
    }
}

// These need to be implemented in the platform-specific files
std::unique_ptr<SquidUUID> SquidUUID::fromString(const std::string& uuid) {
    return std::make_unique<NimBLEUUIDWrapper>(uuid);
}

std::unique_ptr<SquidUUID> SquidUUID::fromuint16_t(uint16_t uuid) {
    return std::make_unique<NimBLEUUIDWrapper>(uuid);
}
