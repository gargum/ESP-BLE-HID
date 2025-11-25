/**
 * @file Types.h
 * @brief Consolidated type definitions, matrix, and keymap implementations
 */

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <vector>
#include <functional>
#include <initializer_list>
#include "../Log/Log.h"

// ============================================================================
// Strong Type Definitions
// ============================================================================

// Base key type for template parameterization
struct KeyTag {};

// Barton-Nackman strong type aliasing because I think it looks really pretty
struct ModKeyTag          : KeyTag {};
struct MediaKeyTag        : KeyTag {};
struct NKROKeyTag         : KeyTag {};
struct StenoKeyTag        : KeyTag {};
struct GamepadKeyTag      : KeyTag {};
struct GamepadHatTag      : KeyTag {};
struct GamepadAnalogueTag : KeyTag {};
struct MouseKeyTag        : KeyTag {};
struct DigitizerKeyTag    : KeyTag {};

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
    
    // Arithmetic operators
    constexpr KeyType operator+(const KeyType& other) const { return KeyType(value + other.value); }
    constexpr KeyType operator-(const KeyType& other) const { return KeyType(value - other.value); }
    constexpr KeyType operator*(const KeyType& other) const { return KeyType(value * other.value); }
    constexpr KeyType operator/(const KeyType& other) const { return KeyType(value / other.value); }
};

// Specific key types - these are the actual definitions
using ModKey          = KeyType<ModKeyTag>;
using MediaKey        = KeyType<MediaKeyTag>;
using NKROKey         = KeyType<NKROKeyTag>;
using StenoKey        = KeyType<StenoKeyTag>;
using GamepadButton   = KeyType<GamepadKeyTag>;
using GamepadHat      = KeyType<GamepadHatTag>;
using GamepadAnalogue = KeyType<GamepadAnalogueTag>;
using MouseKey        = KeyType<MouseKeyTag>;
using DigitizerKey    = KeyType<DigitizerKeyTag>;

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

constexpr StenoKey operator"" _steno(unsigned long long value) {
    return StenoKey(static_cast<int32_t>(value));
}

constexpr GamepadButton operator"" _button(unsigned long long value) {
    return GamepadButton(static_cast<int32_t>(value));
}

constexpr GamepadHat operator"" _hat(unsigned long long value) {
    return GamepadHat(static_cast<int32_t>(value));
}

constexpr GamepadAnalogue operator"" _analogue(unsigned long long value) {
    return GamepadAnalogue(static_cast<int32_t>(value));
}

constexpr MouseKey operator"" _mouse(unsigned long long value) {
    return MouseKey(static_cast<int32_t>(value));
}

constexpr DigitizerKey operator"" _digitizer(unsigned long long value) {
    return DigitizerKey(static_cast<int32_t>(value));
}

// Helper macro for enum type name generation
#define GET_ENUM_TYPE_FROM_KEY_TYPE(key_type) key_type##s

// Main MK macro with automatic enum type detection
#define MK(type, short_name, enum_value) \
    constexpr type short_name = type{static_cast<int32_t>(GET_ENUM_TYPE_FROM_KEY_TYPE(type)::enum_value)}

#define PACKED __attribute__((packed))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// ============================================================================
// Matrix Definitions
// ============================================================================

// Matrix pin pair definition
struct MatrixPinPair {
    int from_pin;
    int to_pin;
    bool is_ground; // true if to_pin is GND
    
    MatrixPinPair(int from, int to = -1) 
        : from_pin(from), to_pin(to), is_ground(to == -1) {}
    
    // Constructor for initializer list {from, to}
    MatrixPinPair(std::initializer_list<int> pins) {
        auto it = pins.begin();
        from_pin = *it++;
        if (it != pins.end()) {
            to_pin = *it;
            is_ground = false;
        } else {
            to_pin = -1;
            is_ground = true;
        }
    }
};

// Type alias for matrix
using squid_matrix = std::vector<MatrixPinPair>;

// Matrix scan result
struct MatrixScanResult {
    size_t switch_index;
    bool pressed;
    
    MatrixScanResult(size_t idx, bool p) : switch_index(idx), pressed(p) {}
};

// ============================================================================
// Keymap Definitions
// ============================================================================

// Union of all possible key types for the keymap
union KeymapValue {
    NKROKey         nkro_key;
    ModKey          mod_key;
    MediaKey        media_key;
    StenoKey        steno_key;
    GamepadButton   gamepad_button;
    GamepadHat      gamepad_hat;
    GamepadAnalogue gamepad_analogue;
    MouseKey        mouse_key;
    DigitizerKey    digitizer_key;
    
    KeymapValue() : nkro_key(NKROKey{0}) {}
    KeymapValue(NKROKey k) : nkro_key(k) {}
    KeymapValue(ModKey k) : mod_key(k) {}
    KeymapValue(MediaKey k) : media_key(k) {}
    KeymapValue(StenoKey k) : steno_key(k) {}
    KeymapValue(GamepadButton k) : gamepad_button(k) {}
    KeymapValue(GamepadHat k) : gamepad_hat(k) {}
    KeymapValue(GamepadAnalogue k) : gamepad_analogue(k) {}
    KeymapValue(MouseKey k) : mouse_key(k) {}
    KeymapValue(DigitizerKey k) : digitizer_key(k) {}
};

// Key type identifier
enum class KeypressType {
    NKRO_KEY,
    MOD_KEY,
    MEDIA_KEY,
    STENO_KEY,
    GAMEPAD_BUTTON,
    GAMEPAD_HAT,
    GAMEPAD_ANALOGUE,
    MOUSE_KEY,
    DIGITIZER_KEY
};

// Keymap entry
struct KeymapEntry {
    KeypressType type;
    KeymapValue key;
    
    // Constructors for each key type
    KeymapEntry(NKROKey k) : type(KeypressType::NKRO_KEY), key(k) {}
    KeymapEntry(ModKey k) : type(KeypressType::MOD_KEY), key(k) {}
    KeymapEntry(MediaKey k) : type(KeypressType::MEDIA_KEY), key(k) {}
    KeymapEntry(StenoKey k) : type(KeypressType::STENO_KEY), key(k) {}
    KeymapEntry(GamepadButton k) : type(KeypressType::GAMEPAD_BUTTON), key(k) {}
    KeymapEntry(GamepadHat k) : type(KeypressType::GAMEPAD_HAT), key(k) {}
    KeymapEntry(GamepadAnalogue k) : type(KeypressType::GAMEPAD_ANALOGUE), key(k) {}
    KeymapEntry(MouseKey k) : type(KeypressType::MOUSE_KEY), key(k) {}
    KeymapEntry(DigitizerKey k) : type(KeypressType::DIGITIZER_KEY), key(k) {}
    
    // Default constructor
    KeymapEntry() : type(KeypressType::NKRO_KEY), key(NKROKey{0}) {}
};

// Type alias for keymap
class squid_map : public std::vector<KeymapEntry> {
public:
    // Default constructor
    squid_map() = default;
    
    // Constructor from initializer list
    squid_map(std::initializer_list<KeymapEntry> keys) 
        : std::vector<KeymapEntry>(keys) {}
    
};

// ============================================================================
// Layer Definitions
// ============================================================================

// Layer action types
enum class LayerActionType {
    NORMAL_KEY,           // Regular keypress
    LAYER_MOMENTARY,      // Momentary layer switch (while held)
    LAYER_TOGGLE,         // Toggle layer on/off
    LAYER_ON,             // Turn layer on
    LAYER_OFF,            // Turn layer off
    LAYER_MOD,            // Layer modifier (shift to layer while held)
    TRANSPARENT,          // Pass through to lower layer
    LAYER_DEFAULT,        // Switch to default layer
};

// Simple layer action value - using a struct instead of union
struct LayerActionValue {
    KeymapEntry key;                   // For NORMAL_KEY
    uint8_t layer_index;               // For layer actions
    
    // Simple constructor
    LayerActionValue() : key(), layer_index(0) {}
};

// Enhanced keymap entry with layer support
struct LayerKeymapEntry {
    LayerActionType action_type;
    LayerActionValue action;
    
    // Constructors for different action types
    LayerKeymapEntry(KeymapEntry k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = k; 
    }
    
    LayerKeymapEntry(LayerActionType type, uint8_t layer) 
        : action_type(type) { 
        action.layer_index = layer; 
    }
    
    // Constructor for regular key codes (implicit conversion from KeymapEntry)
    LayerKeymapEntry(NKROKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(ModKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(MediaKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(MouseKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(DigitizerKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(GamepadButton k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    } 
    
    LayerKeymapEntry(GamepadHat k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(GamepadAnalogue k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(StenoKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    // Default constructor
    LayerKeymapEntry() 
        : action_type(LayerActionType::NORMAL_KEY) {}
};

// Layer state management
struct LayerState {
    uint8_t default_layer;
    std::vector<uint8_t> active_layers;  // Layer stack (highest priority first)
    std::vector<bool> layer_states;      // Persistent layer states
    
    LayerState() : default_layer(0) {}
};

inline LayerKeymapEntry MO(uint8_t layer) { 
    return LayerKeymapEntry(LayerActionType::LAYER_MOMENTARY, layer); 
}

inline LayerKeymapEntry TG(uint8_t layer) { 
    return LayerKeymapEntry(LayerActionType::LAYER_TOGGLE, layer); 
}

inline LayerKeymapEntry TO(uint8_t layer) { 
    return LayerKeymapEntry(LayerActionType::LAYER_ON, layer); 
}

inline LayerKeymapEntry DF(uint8_t layer) { 
    return LayerKeymapEntry(LayerActionType::LAYER_DEFAULT, layer); 
}

inline LayerKeymapEntry TRANS() { 
    return LayerKeymapEntry(LayerActionType::TRANSPARENT, 0); 
}

// Helper for creating layer definitions
#define LAYER(key_entries) std::vector<LayerKeymapEntry> key_entries
#define KEYMAP(layer_entries) std::vector<std::vector<LayerKeymapEntry>> layer_entries

// Simplified layer action macros 
#define MO(layer) LayerKeymapEntry(LayerActionType::LAYER_MOMENTARY, layer)
#define TG(layer) LayerKeymapEntry(LayerActionType::LAYER_TOGGLE, layer)
#define TO(layer) LayerKeymapEntry(LayerActionType::LAYER_ON, layer)
#define DF(layer) LayerKeymapEntry(LayerActionType::LAYER_DEFAULT, layer)
#define TRANS LayerKeymapEntry(LayerActionType::TRANSPARENT, 0)

// ============================================================================
// Matrix Class Implementation
// ============================================================================

class SQUIDMATRIX {
private:
    squid_matrix _matrix;
    std::vector<bool> _current_state; 
    std::vector<bool> _previous_state; 
    std::function<void(size_t, bool)> _key_event_callback; 
    
    // Smart scanning members
    std::vector<int> _unique_from_pins;
    std::vector<int> _unique_to_pins;
    std::unordered_map<int, bool> _pin_needs_pullup;
    
    // Scanning state
    size_t _current_active_to_pin;
    bool _scan_initialized;
    
    void initializePins();
    void scanMatrix();
    void extractUniquePins();
    
    // New smart detection methods
    bool detectPinNeedsPullup(int pin);
    void detectAllPinPullupRequirements();
    bool getOptimalPinMode(int pin);
    
    // Scanning methods
    void scanWithTimeDivision();
    void scanDirectGND();
    
public:
    SQUIDMATRIX();
    
    void begin(const squid_matrix& matrix, std::function<void(size_t, bool)> key_event_callback = nullptr);
    
    void update();
    bool isPressed(size_t switch_index) const;
    size_t getSwitchCount() const;
    
    void printMatrixState();
    void printPinPullupInfo();
};

// ============================================================================
// Keymap Class Implementation
// ============================================================================

class SQUIDKEYMAP {
private:
    squid_map _keymap;
    std::function<void(const KeymapEntry&)> _press_callback;
    std::function<void(const KeymapEntry&)> _release_callback;
    
public:
    SQUIDKEYMAP();
    
    // Initialize with keymap definition
    void begin(const squid_map& keymap,
               std::function<void(const KeymapEntry&)> press_callback = nullptr,
               std::function<void(const KeymapEntry&)> release_callback = nullptr);
    
    // Handle key events by matrix position
    void handleKeyEvent(size_t switch_index, bool pressed);
    
    // Get key at position
    KeymapEntry getKeyAt(size_t switch_index) const;
    
    // Get keymap size
    size_t getKeyCount() const;
};

// ============================================================================
// Layer Class Implementation
// ============================================================================

// Enhanced keymap class with layer support
class SquidLayerKeymap {
private:
    std::vector<std::vector<LayerKeymapEntry>> _layers;
    LayerState _layer_state;
    
    std::function<void(const KeymapEntry&)> _press_callback;
    std::function<void(const KeymapEntry&)> _release_callback;
    std::function<void(uint8_t)> _layer_change_callback;
    
public:
    SquidLayerKeymap();
    
    void begin(const std::vector<std::vector<LayerKeymapEntry>>& layers,
               std::function<void(const KeymapEntry&)> press_callback = nullptr,
               std::function<void(const KeymapEntry&)> release_callback = nullptr,
               std::function<void(uint8_t)> layer_change_callback = nullptr);
    
    void handleKeyEvent(size_t switch_index, bool pressed);
    void update();
    
    // Layer management
    void setDefaultLayer(uint8_t layer);
    void momentaryLayer(uint8_t layer, bool pressed);
    void toggleLayer(uint8_t layer);
    void layerOn(uint8_t layer);
    void layerOff(uint8_t layer);
    
    // State queries
    uint8_t getActiveLayer() const;
    bool isLayerActive(uint8_t layer) const;
    LayerKeymapEntry getKeyAt(size_t switch_index) const;
    KeymapEntry getEffectiveKeyAt(size_t switch_index) const;
    
    size_t getLayerCount() const;
    size_t getKeyCount() const;
};

// ============================================================================
// Helper Functions for Syntax
// ============================================================================

// Helper function to create matrix from initializer list
inline squid_matrix make_matrix(std::initializer_list<int> pins) {
    squid_matrix matrix;
    // Handle pairs of pins {from, to, from, to, ...}
    auto it = pins.begin();
    while (it != pins.end()) {
        int from = *it++;
        if (it != pins.end()) {
            int to = *it++;
            matrix.emplace_back(from, to);
        } else {
            matrix.emplace_back(from); // Last pin goes to GND
        }
    }
    return matrix;
}

// Helper function to create keymap from initializer list
inline squid_map make_keymap(std::initializer_list<KeymapEntry> keys) {
    return squid_map(keys);
}

#endif // TYPES_H
