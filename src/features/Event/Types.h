/**
 * @file Types.h
 * @brief Strong type definitions for different event categories
 */

#pragma once
#include <cstdint>

// Base key type for template parameterization
struct KeyTag {};

// Barton-Nackman strong type aliasing, because I think it looks really pretty
struct ModKeyTag     : KeyTag {};
struct MediaKeyTag   : KeyTag {};
struct NKROKeyTag    : KeyTag {};
struct StenoKeyTag   : KeyTag {};
struct GamepadKeyTag : KeyTag {};

// Template-based strong types
template<typename Tag>
class KeyType {
private:
    int32_t value;
    
public:
    explicit constexpr KeyType(int32_t v) : value(v) {}
    
    // Implicit conversion to underlying type for compatibility
    constexpr operator int32_t() const { return value; }
    
    // Explicit getter
    constexpr int32_t get() const { return value; }
    
    // Comparison operators
    constexpr bool operator==(const KeyType& other) const { return value == other.value; }
    constexpr bool operator!=(const KeyType& other) const { return value != other.value; }
};

// Specific key types
using ModKey = KeyType<ModKeyTag>;
using MediaKey = KeyType<MediaKeyTag>;
using NKROKey = KeyType<NKROKeyTag>;
using StenoKey = KeyType<StenoKeyTag>;
using GamepadButton = KeyType<GamepadKeyTag>;

// Literal operators for easy creation
constexpr ModKey operator"" _mod(unsigned long long value) {
    return ModKey(static_cast<int32_t>(value));
}

constexpr MediaKey operator"" _media(unsigned long long value) {
    return MediaKey(static_cast<int32_t>(value));
}

constexpr NKROKey operator"" _nkro(unsigned long long value) {
    return NKROKey(static_cast<int32_t>(value));
}

// Helper macro for enum type name generation
#define GET_ENUM_TYPE_FROM_KEY_TYPE(key_type) key_type##s

// Main MK macro with automatic enum type detection
#define MK(type, short_name, enum_value) \
    constexpr type short_name = type{static_cast<int32_t>(GET_ENUM_TYPE_FROM_KEY_TYPE(type)::enum_value)}
