/**
 * @file Types.cpp
 * @brief Implementation of matrix and keymap functionality
 */

#include "Types.h"

// ============================================================================
// Matrix Implementation
// ============================================================================

static const char* MATRIX_TAG = "SQUIDMATRIX";

SQUIDMATRIX::SQUIDMATRIX() 
    : _key_event_callback(nullptr), 
      _current_active_to_pin(0),
      _scan_initialized(false) {}

void SQUIDMATRIX::begin(const squid_matrix& matrix,
                       std::function<void(size_t, bool)> key_event_callback) {
    _matrix = matrix;
    _key_event_callback = key_event_callback;
    
    // Initialize state vectors
    _current_state.assign(_matrix.size(), false);
    _previous_state.assign(_matrix.size(), false);
    
    // Extract all unique pins
    extractUniquePins();
    
    // Detect pull-up requirements for all pins
    detectAllPinPullupRequirements();
    
    // Initialize pins with optimal configurations
    initializePins();
    
    _scan_initialized = true;
    
    SQUID_LOG_INFO(MATRIX_TAG, "Smart matrix initialized with %zu switches", _matrix.size());
    SQUID_LOG_INFO(MATRIX_TAG, "Total unique pins: %zu", _unique_from_pins.size());
    
    // Print pull-up detection results
    printPinPullupInfo();
}

void SQUIDMATRIX::extractUniquePins() {
    _unique_from_pins.clear();
    _unique_to_pins.clear();
    _pin_needs_pullup.clear();
    
    // Extract ALL unique pins
    for (size_t switch_idx = 0; switch_idx < _matrix.size(); ++switch_idx) {
        const auto& pin_pair = _matrix[switch_idx];
        
        // Add FROM pin to unique list
        if (std::find(_unique_from_pins.begin(), _unique_from_pins.end(), pin_pair.from_pin) == _unique_from_pins.end()) {
            _unique_from_pins.push_back(pin_pair.from_pin);
        }
        
        // Add TO pin to unique list (skip GND)
        if (!pin_pair.is_ground) {
            if (std::find(_unique_from_pins.begin(), _unique_from_pins.end(), pin_pair.to_pin) == _unique_from_pins.end()) {
                _unique_from_pins.push_back(pin_pair.to_pin);
            }
            // Also add to TO pins list for scanning
            if (std::find(_unique_to_pins.begin(), _unique_to_pins.end(), pin_pair.to_pin) == _unique_to_pins.end()) {
                _unique_to_pins.push_back(pin_pair.to_pin);
            }
        }
    }
    
    // Sort for consistency
    std::sort(_unique_from_pins.begin(), _unique_from_pins.end());
    std::sort(_unique_to_pins.begin(), _unique_to_pins.end());
    
    SQUID_LOG_DEBUG(MATRIX_TAG, "Unique pins: %zu, TO pins: %zu", 
                   _unique_from_pins.size(), _unique_to_pins.size());
}

bool SQUIDMATRIX::detectPinNeedsPullup(int pin) {
    SQUID_LOG_DEBUG(MATRIX_TAG, "Detecting pull-up requirement for pin %d", pin);
    
    // Step 1: Configure pin as INPUT (no pull-up) and read initial state
    pinMode(pin, INPUT);
    delayMicroseconds(25); // Allow signal to stabilize
    
    int initial_state = digitalRead(pin);
    SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d initial state (INPUT): %d", pin, initial_state);
    
    // Step 2: If pin reads HIGH with no pull-up, it has external pull-up
    if (initial_state == HIGH) {
        SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d has EXTERNAL pull-up resistor", pin);
        return false; // No need for internal pull-up
    }
    
    // Step 3: If pin reads LOW, configure as INPUT_PULLUP and check if it goes HIGH
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(25); // Allow internal pull-up to activate
    
    int pullup_state = digitalRead(pin);
    SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d state (INPUT_PULLUP): %d", pin, pullup_state);
    
    // Step 4: If pin goes HIGH with internal pull-up, it needs internal pull-up
    if (pullup_state == HIGH) {
        SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d needs INTERNAL pull-up resistor", pin);
        return true; // Needs internal pull-up
    }
    
    // Step 5: If pin stays LOW even with pull-up, it might be shorted or have very strong pull-down
    SQUID_LOG_WARN(MATRIX_TAG, "Pin %d stays LOW even with pull-up - may be shorted or have strong pull-down", pin);
    
    // Default to using pull-up for safety
    return true;
}

void SQUIDMATRIX::detectAllPinPullupRequirements() {
    SQUID_LOG_INFO(MATRIX_TAG, "Detecting pull-up requirements for all pins...");
    
    for (int pin : _unique_from_pins) {
        bool needs_pullup = detectPinNeedsPullup(pin);
        _pin_needs_pullup[pin] = needs_pullup;
        
        // Log the result
        if (needs_pullup) {
            SQUID_LOG_INFO(MATRIX_TAG, "Pin %d: USING internal pull-up", pin);
        } else {
            SQUID_LOG_INFO(MATRIX_TAG, "Pin %d: USING external pull-up (INPUT mode)", pin);
        }
    }
    
    // Count statistics
    int internal_pullups = 0;
    int external_pullups = 0;
    
    for (const auto& [pin, needs_pullup] : _pin_needs_pullup) {
        if (needs_pullup) internal_pullups++;
        else external_pullups++;
    }
    
    SQUID_LOG_INFO(MATRIX_TAG, "Pull-up detection complete: %d internal, %d external", 
                   internal_pullups, external_pullups);
}

bool SQUIDMATRIX::getOptimalPinMode(int pin) {
    auto it = _pin_needs_pullup.find(pin);
    if (it != _pin_needs_pullup.end()) {
        return it->second; // true = needs INPUT_PULLUP, false = needs INPUT
    }
    
    // If not detected (shouldn't happen), default to INPUT_PULLUP for safety
    SQUID_LOG_WARN(MATRIX_TAG, "Pin %d not in pull-up cache, defaulting to INPUT_PULLUP", pin);
    return true;
}

void SQUIDMATRIX::initializePins() {
    // Configure all pins with their optimal modes based on detection
    for (int pin : _unique_from_pins) {
        bool needs_pullup = getOptimalPinMode(pin);
        
        if (needs_pullup) {
            pinMode(pin, INPUT_PULLUP);
            SQUID_LOG_DEBUG(MATRIX_TAG, "Configured pin %d as INPUT_PULLUP", pin);
        } else {
            pinMode(pin, INPUT);
            SQUID_LOG_DEBUG(MATRIX_TAG, "Configured pin %d as INPUT (external pull-up)", pin);
        }
    }
    
    SQUID_LOG_INFO(MATRIX_TAG, "All pins initialized with optimal configurations");
}

void SQUIDMATRIX::printPinPullupInfo() {
    SQUID_LOG_INFO(MATRIX_TAG, "=== Pin Pull-Up Configuration ===");
    for (int pin : _unique_from_pins) {
        bool needs_pullup = getOptimalPinMode(pin);
        SQUID_LOG_INFO(MATRIX_TAG, "Pin %d: %s", pin, 
                       needs_pullup ? "INPUT_PULLUP (internal)" : "INPUT (external pull-up)");
    }
    SQUID_LOG_INFO(MATRIX_TAG, "=== End Pin Configuration ===");
}

void SQUIDMATRIX::update() {
    scanMatrix();
}

void SQUIDMATRIX::scanMatrix() {
    if (!_scan_initialized) return;
    
    // Save previous state
    _previous_state = _current_state;
    
    // Reset all pins to their optimal safe states before scanning
    for (int pin : _unique_from_pins) {
        bool needs_pullup = getOptimalPinMode(pin);
        if (needs_pullup) {
            pinMode(pin, INPUT_PULLUP);
        } else {
            pinMode(pin, INPUT);
        }
    }
    
    // Perform scanning based on matrix type
    if (!_unique_to_pins.empty()) {
        // Time-division multiplexing for matrices with explicit TO pins
        scanWithTimeDivision();
    } else {
        // Direct scanning for GND-only matrices
        scanDirectGND();
    }
}

void SQUIDMATRIX::scanWithTimeDivision() {
    int current_to_pin = _unique_to_pins[_current_active_to_pin];
    
    // Reduce stabilization delay
    pinMode(current_to_pin, OUTPUT);
    digitalWrite(current_to_pin, LOW);
    delayMicroseconds(3); // Reduced from 10μs
    
    for (size_t switch_idx = 0; switch_idx < _matrix.size(); ++switch_idx) {
        const auto& pin_pair = _matrix[switch_idx];
        if (pin_pair.is_ground || pin_pair.to_pin != current_to_pin) continue;
        
        // Pins should already be in correct state so just read immediately
        bool pressed = (digitalRead(pin_pair.from_pin) == LOW);
        
        _current_state[switch_idx] = pressed;
        
        // Immediate callback on state change
        if (_current_state[switch_idx] != _previous_state[switch_idx] && _key_event_callback) {
            _key_event_callback(switch_idx, pressed);
        }
    }
    
    // Restore TO pin
    bool to_pin_needs_pullup = getOptimalPinMode(current_to_pin);
    pinMode(current_to_pin, to_pin_needs_pullup ? INPUT_PULLUP : INPUT);
    
    _current_active_to_pin = (_current_active_to_pin + 1) % _unique_to_pins.size();
}

void SQUIDMATRIX::scanDirectGND() {
    // Direct scanning for GND-based switches (no TO pins)
    for (size_t switch_idx = 0; switch_idx < _matrix.size(); ++switch_idx) {
        const auto& pin_pair = _matrix[switch_idx];
        
        // Skip non-GND switches
        if (!pin_pair.is_ground) {
            continue;
        }
        
        // Use pre-detected optimal pin mode
        bool needs_pullup = getOptimalPinMode(pin_pair.from_pin);
        if (needs_pullup) {
            pinMode(pin_pair.from_pin, INPUT_PULLUP);
        } else {
            pinMode(pin_pair.from_pin, INPUT);
        }
        
        delayMicroseconds(3);
        
        // Read the pin - LOW means pressed in both modes
        bool pressed = (digitalRead(pin_pair.from_pin) == LOW);
        
        // Update state
        _current_state[switch_idx] = pressed;
        
        // Check for state change
        if (_current_state[switch_idx] != _previous_state[switch_idx]) {
            SQUID_LOG_DEBUG(MATRIX_TAG, "GND switch: TO=%zu, %s (Mode: %s)", 
                         switch_idx, pressed ? "PRESSED" : "RELEASED",
                         needs_pullup ? "PULLUP" : "INPUT");
            
            if (_key_event_callback) {
                _key_event_callback(switch_idx, pressed);
            }
        }
        
        // Restore pin to optimal safe state
        if (needs_pullup) {
            pinMode(pin_pair.from_pin, INPUT_PULLUP);
        } else {
            pinMode(pin_pair.from_pin, INPUT);
        }
    }
}

bool SQUIDMATRIX::isPressed(size_t switch_index) const {
    if (switch_index < _current_state.size()) {
        return _current_state[switch_index];
    }
    return false;
}

size_t SQUIDMATRIX::getSwitchCount() const {
    return _matrix.size();
}

void SQUIDMATRIX::printMatrixState() {
    SQUID_LOG_DEBUG(MATRIX_TAG, "Current matrix state:");
    std::string state_str;
    for (size_t switch_idx = 0; switch_idx < _current_state.size(); ++switch_idx) {
        state_str += _current_state[switch_idx] ? "1" : "0";
        if (switch_idx < _current_state.size() - 1) {
            state_str += " ";
        }
    }
    SQUID_LOG_DEBUG(MATRIX_TAG, "Switches: [%s]", state_str.c_str());
}

// ============================================================================
// Keymap Implementation
// ============================================================================

static const char* KEYMAP_TAG = "SQUIDKEYMAP";

SQUIDKEYMAP::SQUIDKEYMAP() 
    : _press_callback(nullptr), _release_callback(nullptr) {}

void SQUIDKEYMAP::begin(const squid_map& keymap,
                       std::function<void(const KeymapEntry&)> press_callback,
                       std::function<void(const KeymapEntry&)> release_callback) {
    _keymap = keymap;
    _press_callback = press_callback;
    _release_callback = release_callback;
    
    SQUID_LOG_INFO(KEYMAP_TAG, "Keymap initialized with %zu keys", keymap.size());
}

void SQUIDKEYMAP::handleKeyEvent(size_t switch_index, bool pressed) {
    if (switch_index < _keymap.size()) {
        const auto& key_entry = _keymap[switch_index];
        
        SQUID_LOG_DEBUG(KEYMAP_TAG, "Key event: switch_index=%zu, type=%d, pressed=%s", 
                     switch_index, static_cast<int>(key_entry.type), pressed ? "true" : "false");
        
        if (pressed && _press_callback) {
            _press_callback(key_entry);
        } else if (!pressed && _release_callback) {
            _release_callback(key_entry);
        }
    } else {
        SQUID_LOG_WARN(KEYMAP_TAG, "Invalid key position: switch_index=%zu", switch_index);
    }
}

KeymapEntry SQUIDKEYMAP::getKeyAt(size_t switch_index) const {
    if (switch_index < _keymap.size()) {
        return _keymap[switch_index];
    }
    return KeymapEntry(NKROKey{0}); // Return null key
}

size_t SQUIDKEYMAP::getKeyCount() const {
    return _keymap.size();
}

// ============================================================================
// Layering Implementation
// ============================================================================

static const char* LAYER_KEYMAP_TAG = "SQUIDLAYERKEYMAP";

SquidLayerKeymap::SquidLayerKeymap() 
    : _press_callback(nullptr), 
      _release_callback(nullptr),
      _layer_change_callback(nullptr) {}

void SquidLayerKeymap::begin(
    const std::vector<std::vector<LayerKeymapEntry>>& layers,
    std::function<void(const KeymapEntry&)> press_callback,
    std::function<void(const KeymapEntry&)> release_callback,
    std::function<void(uint8_t)> layer_change_callback) {
    
    _layers = layers;
    _press_callback = press_callback;
    _release_callback = release_callback;
    _layer_change_callback = layer_change_callback;
    
    // Initialize layer state
    _layer_state.default_layer = 0;
    _layer_state.layer_states.assign(_layers.size(), false);
    _layer_state.layer_states[0] = true; // Default layer active
    _layer_state.active_layers.push_back(0);
    
    SQUID_LOG_INFO(LAYER_KEYMAP_TAG, "Layer keymap initialized with %zu layers", _layers.size());
}

void SquidLayerKeymap::handleKeyEvent(size_t switch_index, bool pressed) {
    if (switch_index >= getKeyCount()) {
        SQUID_LOG_WARN(LAYER_KEYMAP_TAG, "Invalid key position: %zu", switch_index);
        return;
    }
    
    // Find the highest priority non-transparent key
    LayerKeymapEntry action;
    for (int i = _layer_state.active_layers.size() - 1; i >= 0; i--) {
        uint8_t layer = _layer_state.active_layers[i];
        if (switch_index < _layers[layer].size()) {
            action = _layers[layer][switch_index];
            if (action.action_type != LayerActionType::TRANSPARENT) {
                break;
            }
        }
    }
    
    // Handle the action
    switch (action.action_type) {
        case LayerActionType::NORMAL_KEY:
            if (_press_callback && pressed) {
                _press_callback(action.action.key);
            }
            if (_release_callback && !pressed) {
                _release_callback(action.action.key);
            }
            break;
            
        case LayerActionType::LAYER_MOMENTARY:
            momentaryLayer(action.action.layer_index, pressed);
            break;
            
        case LayerActionType::LAYER_TOGGLE:
            if (pressed) toggleLayer(action.action.layer_index);
            break;
            
        case LayerActionType::LAYER_ON:
            if (pressed) layerOn(action.action.layer_index);
            break;
            
        case LayerActionType::LAYER_OFF:
            if (pressed) layerOff(action.action.layer_index);
            break;
            
        case LayerActionType::LAYER_DEFAULT:
            if (pressed) setDefaultLayer(action.action.layer_index);
            break;
            
        case LayerActionType::LAYER_MOD:
            momentaryLayer(action.action.layer_index, pressed);
            break;
            
        case LayerActionType::TRANSPARENT:
            break;
    }
}

void SquidLayerKeymap::update() {
    // This does nothing rn but it exists for Tap Dance implementations later down the line
}

void SquidLayerKeymap::setDefaultLayer(uint8_t layer) {
    if (layer < _layers.size()) {
        _layer_state.default_layer = layer;
        
        // Reset to just the default layer
        _layer_state.active_layers.clear();
        _layer_state.active_layers.push_back(layer);
        _layer_state.layer_states.assign(_layers.size(), false);
        _layer_state.layer_states[layer] = true;
        
        if (_layer_change_callback) {
            _layer_change_callback(layer);
        }
        
        SQUID_LOG_INFO(LAYER_KEYMAP_TAG, "Default layer set to %d", layer);
    }
}

void SquidLayerKeymap::momentaryLayer(uint8_t layer, bool pressed) {
    if (layer >= _layers.size()) return;
    
    auto it = std::find(_layer_state.active_layers.begin(), 
                       _layer_state.active_layers.end(), layer);
    
    if (pressed) {
        if (it == _layer_state.active_layers.end()) {
            _layer_state.active_layers.push_back(layer);
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Layer %d activated (momentary)", layer);
        }
    } else {
        if (it != _layer_state.active_layers.end()) {
            _layer_state.active_layers.erase(it);
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Layer %d deactivated (momentary)", layer);
        }
    }
}

void SquidLayerKeymap::toggleLayer(uint8_t layer) {
    if (layer >= _layers.size()) return;
    
    _layer_state.layer_states[layer] = !_layer_state.layer_states[layer];
    
    // Update active layers list
    _layer_state.active_layers.clear();
    _layer_state.active_layers.push_back(_layer_state.default_layer);
    
    for (uint8_t i = 0; i < _layer_state.layer_states.size(); i++) {
        if (_layer_state.layer_states[i] && i != _layer_state.default_layer) {
            _layer_state.active_layers.push_back(i);
        }
    }
    
    SQUID_LOG_INFO(LAYER_KEYMAP_TAG, "Layer %d toggled %s", layer, 
                  _layer_state.layer_states[layer] ? "ON" : "OFF");
    
    if (_layer_change_callback) {
        _layer_change_callback(getActiveLayer());
    }
}

void SquidLayerKeymap::layerOn(uint8_t layer) {
    if (layer < _layers.size() && !_layer_state.layer_states[layer]) {
        toggleLayer(layer);
    }
}

void SquidLayerKeymap::layerOff(uint8_t layer) {
    if (layer < _layers.size() && _layer_state.layer_states[layer]) {
        toggleLayer(layer);
    }
}

uint8_t SquidLayerKeymap::getActiveLayer() const {
    return _layer_state.active_layers.empty() ? 0 : _layer_state.active_layers.back();
}

bool SquidLayerKeymap::isLayerActive(uint8_t layer) const {
    return std::find(_layer_state.active_layers.begin(), 
                    _layer_state.active_layers.end(), layer) != _layer_state.active_layers.end();
}

LayerKeymapEntry SquidLayerKeymap::getKeyAt(size_t switch_index) const {
    if (switch_index < getKeyCount()) {
        for (int i = _layer_state.active_layers.size() - 1; i >= 0; i--) {
            uint8_t layer = _layer_state.active_layers[i];
            if (switch_index < _layers[layer].size()) {
                LayerKeymapEntry entry = _layers[layer][switch_index];
                if (entry.action_type != LayerActionType::TRANSPARENT) {
                    return entry;
                }
            }
        }
    }
    return LayerKeymapEntry(); // Return transparent if nothing found
}

KeymapEntry SquidLayerKeymap::getEffectiveKeyAt(size_t switch_index) const {
    LayerKeymapEntry layer_entry = getKeyAt(switch_index);
    if (layer_entry.action_type == LayerActionType::NORMAL_KEY) {
        return layer_entry.action.key;
    }
    return KeymapEntry(); // Return null key for non-normal actions
}

size_t SquidLayerKeymap::getLayerCount() const {
    return _layers.size();
}

size_t SquidLayerKeymap::getKeyCount() const {
    // Return the maximum key count across all layers
    size_t max_keys = 0;
    for (const auto& layer : _layers) {
        if (layer.size() > max_keys) {
            max_keys = layer.size();
        }
    }
    return max_keys;
}
