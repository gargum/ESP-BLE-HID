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
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <string>
#include "../Log/Log.h"

// ============================================================================
// Strong Type Definitions
// ============================================================================

// Base key type for template parameterization
struct KeyTag {};

// Barton-Nackman strong type aliasing because I think it looks really pretty
struct ModKeyTag             : KeyTag {};
struct MediaKeyTag           : KeyTag {};
struct NKROKeyTag            : KeyTag {};
struct StenoKeyTag           : KeyTag {};
struct GamepadKeyTag         : KeyTag {};
struct GamepadHatTag         : KeyTag {};
struct GamepadAnalogueTag    : KeyTag {};
struct MouseKeyTag           : KeyTag {};
struct MouseAnalogueTag      : KeyTag {};
struct DigitizerKeyTag       : KeyTag {};
struct DigitizerAnalogueTag  : KeyTag {};
struct SpacemouseKeyTag      : KeyTag {};
struct SpacemouseAnalogueTag : KeyTag {};
struct HapticKeyTag          : KeyTag {};

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
using ModKey             = KeyType<ModKeyTag>;
using MediaKey           = KeyType<MediaKeyTag>;
using NKROKey            = KeyType<NKROKeyTag>;
using StenoKey           = KeyType<StenoKeyTag>;
using GamepadButton      = KeyType<GamepadKeyTag>;
using GamepadHat         = KeyType<GamepadHatTag>;
using GamepadAnalogue    = KeyType<GamepadAnalogueTag>;
using MouseKey           = KeyType<MouseKeyTag>;
using MouseAnalogue      = KeyType<MouseAnalogueTag>;
using DigitizerKey       = KeyType<DigitizerKeyTag>;
using DigitizerAnalogue  = KeyType<DigitizerAnalogueTag>;
using SpacemouseKey      = KeyType<SpacemouseKeyTag>;
using SpacemouseAnalogue = KeyType<SpacemouseAnalogueTag>;
using HapticKey          = KeyType<HapticKeyTag>;

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

constexpr MouseAnalogue operator"" _mouseanalogue(unsigned long long value) {
    return MouseAnalogue(static_cast<int32_t>(value));
}

constexpr DigitizerKey operator"" _digitizer(unsigned long long value) {
    return DigitizerKey(static_cast<int32_t>(value));
}

constexpr DigitizerAnalogue operator"" _digitizeranalogue(unsigned long long value) {
    return DigitizerAnalogue(static_cast<int32_t>(value));
}

constexpr SpacemouseKey operator"" _spacemouse(unsigned long long value) {
    return SpacemouseKey(static_cast<int32_t>(value));
}

constexpr SpacemouseAnalogue operator"" _spaceanalogue(unsigned long long value) {
    return SpacemouseAnalogue(static_cast<int32_t>(value));
}

constexpr HapticKey operator"" _haptic(unsigned long long value) {
    return HapticKey(static_cast<int32_t>(value));
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

class SQUIDHID;

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
    NKROKey            nkro_key;
    ModKey             mod_key;
    MediaKey           media_key;
    StenoKey           steno_key;
    GamepadButton      gamepad_button;
    GamepadHat         gamepad_hat;
    GamepadAnalogue    gamepad_analogue;
    MouseKey           mouse_key;
    MouseAnalogue      mouse_analogue;
    DigitizerKey       digitizer_key;
    DigitizerAnalogue  digitizer_analogue;
    SpacemouseKey      spacemouse_key;
    SpacemouseAnalogue spacemouse_analogue;
    HapticKey          haptic_key;
    
    KeymapValue() : nkro_key(NKROKey{0}) {}
    KeymapValue(NKROKey k) : nkro_key(k) {}
    KeymapValue(ModKey k) : mod_key(k) {}
    KeymapValue(MediaKey k) : media_key(k) {}
    KeymapValue(StenoKey k) : steno_key(k) {}
    KeymapValue(GamepadButton k) : gamepad_button(k) {}
    KeymapValue(GamepadHat k) : gamepad_hat(k) {}
    KeymapValue(GamepadAnalogue k) : gamepad_analogue(k) {}
    KeymapValue(MouseKey k) : mouse_key(k) {}
    KeymapValue(MouseAnalogue k) : mouse_analogue(k) {}
    KeymapValue(DigitizerKey k) : digitizer_key(k) {}
    KeymapValue(DigitizerAnalogue k) : digitizer_analogue(k) {}
    KeymapValue(SpacemouseKey k ) : spacemouse_key(k) {}
    KeymapValue(SpacemouseAnalogue k) : spacemouse_analogue(k) {}
    KeymapValue(HapticKey k) : haptic_key(k) {}
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
    MOUSE_ANALOGUE,
    DIGITIZER_KEY,
    DIGITIZER_ANALOGUE,
    SPACEMOUSE_KEY,
    SPACEMOUSE_ANALOGUE,
    HAPTIC_KEY
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
    KeymapEntry(MouseAnalogue k) : type(KeypressType::MOUSE_ANALOGUE), key(k) {}
    KeymapEntry(DigitizerKey k) : type(KeypressType::DIGITIZER_KEY), key(k) {}
    KeymapEntry(DigitizerAnalogue k) : type(KeypressType::DIGITIZER_ANALOGUE), key(k) {}
    KeymapEntry(SpacemouseKey k) : type(KeypressType::SPACEMOUSE_KEY), key(k) {}
    KeymapEntry(SpacemouseAnalogue k) : type(KeypressType::SPACEMOUSE_ANALOGUE), key(k) {}
    KeymapEntry(HapticKey k) : type(KeypressType::HAPTIC_KEY), key(k) {}
    
    // Default constructor
    KeymapEntry() : type(KeypressType::NKRO_KEY), key(NKROKey{0}) {}
};

namespace std {
    template<>
    struct hash<KeymapEntry> {
        std::size_t operator()(const KeymapEntry& k) const noexcept {
            // Combine type and value hash
            std::size_t h1 = std::hash<int>{}(static_cast<int>(k.type));
            std::size_t h2;
            
            switch (k.type) {
                case KeypressType::NKRO_KEY:
                    h2 = std::hash<int32_t>{}(k.key.nkro_key.get());
                    break;
                case KeypressType::MOD_KEY:
                    h2 = std::hash<int32_t>{}(k.key.mod_key.get());
                    break;
                case KeypressType::MEDIA_KEY:
                    h2 = std::hash<int32_t>{}(k.key.media_key.get());
                    break;
                // ... add cases for other key types
                default:
                    h2 = std::hash<int32_t>{}(0);
            }
            
            return h1 ^ (h2 << 1);
        }
    };
}

inline bool operator==(const KeymapEntry& lhs, const KeymapEntry& rhs) {
    if (lhs.type != rhs.type) return false;
    
    switch (lhs.type) {
        case KeypressType::NKRO_KEY:
            return lhs.key.nkro_key == rhs.key.nkro_key;
        case KeypressType::MOD_KEY:
            return lhs.key.mod_key == rhs.key.mod_key;
        case KeypressType::MEDIA_KEY:
            return lhs.key.media_key == rhs.key.media_key;
        // ... add cases for other key types
        default:
            return memcmp(&lhs.key, &rhs.key, sizeof(KeymapValue)) == 0;
    }
}

// Type alias for keymap
class squid_map : public std::vector<KeymapEntry> {
public:
    // Default constructor
    squid_map() = default;
    
    // Constructor from initializer list
    squid_map(std::initializer_list<KeymapEntry> keys) 
        : std::vector<KeymapEntry>(keys) {}
    
};

// Early combo timeout tracking
struct EarlyTimeoutInfo {
    uint32_t last_key_press_time;
    uint32_t last_key_release_time;
    size_t active_key_count;
        
    EarlyTimeoutInfo() : last_key_press_time(0), last_key_release_time(0), active_key_count(0) {}
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

// Combo configuration structure
struct ComboKeySpec {
    enum class Type {
        POSITION,    // Switch index
        KEYCODE,     // Keycode from keymap
        ANY_POSITION // Match any position with a specific keycode
    };
    
    Type type;
    
    // Use a union with proper constructors
    union Value {
        size_t position;
        KeymapEntry keycode;
        
        Value() : position(0) {}  // Default construct with position
        Value(size_t pos) : position(pos) {}
        Value(const KeymapEntry& key) : keycode(key) {}
        ~Value() {}  // Union members are trivially destructible
        
        Value& operator=(const Value& other) {
            if (this != &other) {
                // Since we don't know which member is active, we just copy bytes
                memcpy(this, &other, sizeof(Value));
            }
            return *this;
        }
    } value;
    
    // Constructors
    ComboKeySpec(size_t pos) : type(Type::POSITION), value(pos) {}
    ComboKeySpec(const KeymapEntry& key) : type(Type::KEYCODE), value(key) {}
    ComboKeySpec(Type t, const KeymapEntry& key) : type(t), value(key) {}
    
    ComboKeySpec() : type(Type::POSITION), value(0) {}
    
    // Copy constructor
    ComboKeySpec(const ComboKeySpec& other) : type(other.type), value() {
        value = other.value;
    }
    
    // Assignment operator
    ComboKeySpec& operator=(const ComboKeySpec& other) {
        if (this != &other) {
            type = other.type;
            value = other.value;
        }
        return *this;
    }
};

// Combo state tracking
struct ComboState {
    std::vector<bool> pressed_keys;       // Which keys in the combo are pressed
    uint32_t start_time;                  // When the combo sequence started
    bool triggered;                       // Whether combo has been triggered
    bool sent;                            // Whether action has been sent
    
    ComboState(size_t key_count) 
        : pressed_keys(key_count, false), start_time(0), triggered(false), sent(false) {}
};

// Combo general structure
struct KeyComboConfig {
    std::vector<ComboKeySpec> key_specs;    // Flexible key specifications
    uint16_t timeout_ms;                    // Time window for key presses
    KeymapEntry action;                     // Action to trigger when combo is pressed
    
    KeyComboConfig(std::vector<ComboKeySpec> specs, KeymapEntry act, uint16_t timeout = 200)
        : key_specs(specs), action(act), timeout_ms(timeout) {}
    
    // Backward compatibility constructor
    KeyComboConfig(std::vector<size_t> positions, KeymapEntry act, uint16_t timeout = 200)
        : action(act), timeout_ms(timeout) {
        for (auto pos : positions) {
            key_specs.push_back(ComboKeySpec(pos));
        }
    }
    
    // Default constructor
    KeyComboConfig() : timeout_ms(200) {}
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
    
    LayerKeymapEntry(MouseAnalogue k)
        : action_type(LayerActionType::NORMAL_KEY) {
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(DigitizerKey k) 
        : action_type(LayerActionType::NORMAL_KEY) { 
        action.key = KeymapEntry(k);
    }
    
    LayerKeymapEntry(DigitizerAnalogue k)
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
    
    LayerKeymapEntry(SpacemouseKey k)
        : action_type(LayerActionType::NORMAL_KEY) {
        action.key = KeymapEntry(k);   
    }
    
    LayerKeymapEntry(SpacemouseAnalogue k)
        : action_type(LayerActionType::NORMAL_KEY) {
        action.key = KeymapEntry(k);
    }

    LayerKeymapEntry(HapticKey k) 
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

// Helper for creating keymap and matrix definitions
#define MATRIX(pin_entries) squid_matrix pin_entries
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
    
    // GPIO function pointers for the matrix scanning
    std::function<void(uint8_t, uint8_t)> _pinModeFunc;
    std::function<void(uint8_t, uint8_t)> _digitalWriteFunc;
    std::function<uint8_t(uint8_t)> _digitalReadFunc;
    
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
    
    // Smart pinmode detection methods
    bool detectPinNeedsPullup(int pin);
    void detectAllPinPullupRequirements();
    bool getOptimalPinMode(int pin);
    
    // Unified GPIO helper methods
    void unifiedPinMode(uint8_t pin, uint8_t mode);
    void unifiedDigitalWrite(uint8_t pin, uint8_t value);
    uint8_t unifiedDigitalRead(uint8_t pin);
    
    // Scanning methods
    void scanWithTimeDivision();
    void scanDirectGND();
    
public:
    SQUIDMATRIX();
    
    void begin(const squid_matrix& matrix, 
               std::function<void(size_t, bool)> key_event_callback = nullptr,
               std::function<void(uint8_t, uint8_t)> pinModeFunc = nullptr,
               std::function<void(uint8_t, uint8_t)> digitalWriteFunc = nullptr,
               std::function<uint8_t(uint8_t)> digitalReadFunc = nullptr);
    
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
    std::vector<std::vector<LayerKeymapEntry>> _layers;
    LayerState _layer_state;
    
    std::function<void(const KeymapEntry&)> _press_callback;
    std::function<void(const KeymapEntry&)> _release_callback;
    std::function<void(uint8_t)> _layer_change_callback;
    std::unordered_map<KeymapEntry, std::vector<size_t>> _keycode_to_positions;
    std::vector<KeyComboConfig> _key_combos;
    std::unordered_set<size_t> _keys_in_active_combos;
    std::unordered_map<size_t, std::vector<size_t>> _combo_key_to_combo_idx;
    std::unordered_map<KeymapEntry, std::vector<size_t>> _combo_keycode_to_combo_idx;
    std::vector<ComboState> _combo_states;
    std::vector<size_t> getPositionsForComboKey(const ComboKeySpec& spec);
    
    // Combo timing
    uint16_t _combo_timeout_ms = 200;
    uint32_t _last_combo_check = 0;
    void updateEarlyTimeoutInfo(size_t combo_idx, bool key_pressed);
    bool shouldEarlyTimeout(size_t combo_idx) const;
    std::vector<EarlyTimeoutInfo> _early_timeout_info;
    
    // Combo methods
    void initializeCombos();
    void updateCombos();
    void updateComboForKey(size_t switch_index, bool pressed);
    void updateKeycodeMappings();
    void checkCombo(size_t combo_idx);
    bool checkComboKeyPressed(const ComboKeySpec& spec, const std::vector<bool>& key_states);
    bool isKeyInActiveCombo(size_t switch_index) const;
    bool isAnyComboTriggered() const;
    void triggerCombo(size_t combo_idx, bool pressed);
    void sendComboAction(const KeymapEntry& action, bool pressed);
    
    // Combo debug
    bool _combo_debug_enabled = false;
    
public:
    SQUIDKEYMAP();
    
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
    
    // Combo configuration
    void addCombo(const KeyComboConfig& combo);
    void setCombos(const std::vector<KeyComboConfig>& combos);
    void clearCombos();
    void resetComboState(size_t combo_idx);
    void setComboTimeout(uint16_t timeout_ms);
    
    // State queries
    size_t getComboCount() const;
    bool isComboActive(size_t combo_idx) const;
    
    // Debugging helpers
    void enableComboDebug(bool enabled) { _combo_debug_enabled = enabled; }
};

// ============================================================================
// Helper Functions and Macros for Sketch Syntax
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

struct SimpleCombo {
    std::vector<ComboKeySpec> keys;  // Use vector instead of initializer_list
    KeymapEntry action;
    uint16_t timeout_ms;
    
    // Constructor for position indices (backward compatibility)
    template<typename T>
    SimpleCombo(std::initializer_list<size_t> positions, T key_action, uint16_t t = 200) 
        : action(KeymapEntry(key_action)), timeout_ms(t) {
        keys.reserve(positions.size());
        for (size_t pos : positions) {
            keys.push_back(ComboKeySpec(pos));
        }
    }
    
    // Constructor for ComboKeySpec list
    template<typename T>
    SimpleCombo(std::initializer_list<ComboKeySpec> key_specs, T key_action, uint16_t t = 200) 
        : action(KeymapEntry(key_action)), timeout_ms(t) {
        keys.reserve(key_specs.size());
        for (const auto& spec : key_specs) {
            keys.push_back(spec);
        }
    }
    
    // Conversion to KeyComboConfig
    operator KeyComboConfig() const {
        return KeyComboConfig(keys, action, timeout_ms);
    }
};

// Helper function to convert SimpleCombo list to vector
inline std::vector<KeyComboConfig> make_combos_list(std::initializer_list<SimpleCombo> combos) {
    std::vector<KeyComboConfig> result;
    for (const auto& combo : combos) {
        result.push_back(combo);
    }
    return result;
}

inline ComboKeySpec POS(size_t pos) { return ComboKeySpec(pos); }

template<typename T>
inline ComboKeySpec KEY(T key) { 
    return ComboKeySpec(KeymapEntry(key)); 
}

template<typename T>
inline ComboKeySpec ANY(T key) { 
    return ComboKeySpec(ComboKeySpec::Type::ANY_POSITION, KeymapEntry(key)); 
}

#define COMBO(var_name) \
    auto var_name = [](std::initializer_list<SimpleCombo> combos) -> std::vector<KeyComboConfig> { \
        std::vector<KeyComboConfig> result; \
        for (const auto& combo : combos) { \
            result.push_back(combo); \
        } \
        return result; \
    }
    
#endif // TYPES_H
