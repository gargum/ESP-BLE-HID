/**
 * @file Keymap.h
 * @brief Header file for the keymap implementations
 */

#ifndef KEYMAP_H
#define KEYMAP_H

#include <unordered_set>
#include "drivers/Software/Log/Log.h"
#include "drivers/Software/Event/Types.h"
#include "drivers/Software/Basic/Matrix/Matrix.h"

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

// Individual key tap tracking for combos
struct KeyTapInfo {
    uint32_t press_time;
    uint32_t release_time;
    bool is_tap;           // True if this was a quick tap (not part of a combo)
    bool sent_as_normal;   // Whether normal key event was already sent
    bool press_sent;       // Whether press event was sent
    bool release_sent;     // Whether release event was sent
    uint8_t tap_count;     // For detecting rapid taps
    
    KeyTapInfo() : press_time(0), release_time(0), is_tap(false), 
                   sent_as_normal(false), press_sent(false), 
                   release_sent(false), tap_count(0) {}
    
    void reset() {
        press_time = 0;
        release_time = 0;
        is_tap = false;
        sent_as_normal = false;
        press_sent = false;
        release_sent = false;
        // Note: don't reset tap_count for rollover detection
    }
};

// ============================================================================
// Layer Definitions
// ============================================================================

// Layer action types
enum class LayerActionType {
    NORMAL_KEY,           // Regular keypress
    TAP_HOLD_KEY,         // Tap/Hold functionality
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
    KeymapEntry key;                   // For NORMAL_KEY and TAP_HOLD_KEY tap action
    uint8_t layer_index;               // For layer actions
    KeymapEntry hold_action;           // For TAP_HOLD_KEY - hold action
    // Simple constructor
    LayerActionValue() : key(), layer_index(0), hold_action() {}
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

inline LayerKeymapEntry TAPHOLD(KeymapEntry tap_action, KeymapEntry hold_action) {
    LayerKeymapEntry entry(LayerActionType::TAP_HOLD_KEY, 0);
    entry.action.key = tap_action;
    entry.action.hold_action = hold_action;
    return entry;
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

// Simplified TH macro:
#define TH(tap_key, hold_key) TAPHOLD(KeymapEntry(tap_key), KeymapEntry(hold_key))

// ============================================================================
// Tap/Hold & Tap Dance Definitions
// ============================================================================

// Tap-Hold state object
struct TapHoldState {
    bool is_tap_hold_key;           // Whether this key has tap/hold behavior
    bool pending_tap;               // Whether a tap is pending
    bool is_held;                   // Whether currently held
    uint32_t press_time;            // When the key was pressed
    uint32_t tap_timeout;           // When the tap timeout expires
    KeymapEntry tap_action;         // Action to send on tap
    KeymapEntry hold_action;        // Action to send on hold
    uint16_t tap_timeout_ms;        // Configurable tap timeout
    uint16_t hold_threshold_ms;     // Configurable hold threshold
    bool hold_action_sent;          // Whether the hold action has been sent
    
    TapHoldState() : is_tap_hold_key(false), pending_tap(false), is_held(false),
                     press_time(0), tap_timeout(0), tap_action(), hold_action(),
                     tap_timeout_ms(200), hold_threshold_ms(150),
                     hold_action_sent(false) {}
    
    void reset() {
        pending_tap = false;
        is_held = false;
        tap_timeout = 0;
        press_time = 0;
        hold_action_sent = false;
    }
};

// Per-combo tap/hold tracking
struct ComboTapHoldInfo {
    bool is_dual_function;           // Whether this combo has tap/hold behavior
    uint16_t tap_timeout_ms;         // Max time to consider it a tap
    std::vector<size_t> combo_keys;  // Keys involved in this combo
        
    ComboTapHoldInfo() : is_dual_function(false), tap_timeout_ms(200) {}
};

struct DelayedKeyEvent {
    size_t switch_index;
    bool pressed;
    uint32_t scheduled_time;
        
    DelayedKeyEvent(size_t idx, bool p, uint32_t time) 
        : switch_index(idx), pressed(p), scheduled_time(time) {}
};

// ============================================================================
// Keymap Class Implementation
// ============================================================================

class SQUIDKEYMAP {
private:
    std::vector<std::vector<LayerKeymapEntry>> _layers;
    LayerState _layer_state;
    std::vector<TapHoldState> _tap_hold_states;
    std::function<void(const KeymapEntry&)> _press_callback;
    std::function<void(const KeymapEntry&)> _release_callback;
    std::vector<DelayedKeyEvent> _delayed_key_events;
    std::vector<KeyTapInfo> _key_tap_info;
    std::function<void(uint8_t)> _layer_change_callback;
    std::unordered_map<KeymapEntry, std::vector<size_t>> _keycode_to_positions;
    std::vector<KeyComboConfig> _key_combos;
    std::unordered_set<size_t> _keys_in_active_combos;
    std::unordered_map<size_t, std::vector<size_t>> _combo_key_to_combo_idx;
    std::unordered_map<KeymapEntry, std::vector<size_t>> _combo_keycode_to_combo_idx;
    std::vector<ComboState> _combo_states;
    std::vector<ComboTapHoldInfo> _combo_tap_hold_info;
    std::vector<size_t> getPositionsForComboKey(const ComboKeySpec& spec);
    
    // Combo timing
    uint16_t _combo_timeout_ms                      = 200;
    static constexpr uint32_t TAP_TIMEOUT_MS        = 150;  // Max time for a tap
    static constexpr uint32_t HOLD_THRESHOLD_MS     = 50;   // Min time for a hold
    static constexpr uint32_t TAP_GRACE_PERIOD      = 30;   // Tap/Combo timeout
    static constexpr uint32_t ROLLOVER_THRESHOLD_MS = 50;   // Max time between keys for roll
    static constexpr uint32_t TYPING_FLOW_THRESHOLD = 50;   // ms between keys
    
    uint32_t _last_combo_check = 0;
    void updateEarlyTimeoutInfo(size_t combo_idx, bool key_pressed);
    bool shouldEarlyTimeout(size_t combo_idx) const;
    std::vector<EarlyTimeoutInfo> _early_timeout_info;
    bool isKeyTap(size_t switch_index) const;
    void updateKeyTapInfo(size_t switch_index, bool pressed);
    bool shouldProcessAsNormalKey(size_t switch_index) const;
    
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
    
    // Tap/Hold methods
    // Methods for tap/hold handling
    void updateTapHoldState(size_t switch_index, bool pressed);
    bool isTapHoldKey(size_t switch_index) const;
    void sendTapAction(size_t switch_index);
    void cancelPendingTap(size_t switch_index);
    void processDelayedEvents();
    
    // Method to determine if keys are being tapped or held for a combo
    bool areComboKeysBeingHeld(size_t combo_idx) const;
    bool areComboKeysBeingTapped(size_t combo_idx) const; 
    
    // Method to detect if this is a typing pattern vs combo attempt
    bool isTypingPattern(size_t switch_index, bool pressed) const;
    bool detectTypingFlow(size_t switch_index, bool pressed);
    bool shouldSuppressForCombo(size_t switch_index, bool pressed) const;
    
    // Track typing patterns
    uint32_t _last_any_key_press_time;
    size_t _last_key_pressed;
    uint32_t _last_normal_key_time;
    bool _in_typing_flow;
    uint32_t _typing_flow_start;
    
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
    
    // Tap/Hold configuration
    void processTapHoldKey(size_t switch_index, bool pressed, const LayerKeymapEntry& action);
    
    // State queries
    size_t getComboCount() const;
    bool isComboActive(size_t combo_idx) const;
    void processNormalKey(size_t switch_index, bool pressed);
    void processDelayedNormalKey(size_t switch_index);
    void setComboTapHold(size_t combo_idx, bool enabled, uint16_t tap_timeout = 150);
    
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
    
#endif // KEYMAP_H
