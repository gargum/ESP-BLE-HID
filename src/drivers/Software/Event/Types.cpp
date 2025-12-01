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
      _pinModeFunc(nullptr),
      _digitalWriteFunc(nullptr),
      _digitalReadFunc(nullptr),
      _current_active_to_pin(0),
      _scan_initialized(false) {}

void SQUIDMATRIX::begin(const squid_matrix& matrix, 
                       std::function<void(size_t, bool)> key_event_callback,
                       std::function<void(uint8_t, uint8_t)> pinModeFunc,
                       std::function<void(uint8_t, uint8_t)> digitalWriteFunc,
                       std::function<uint8_t(uint8_t)> digitalReadFunc) {
    _matrix = matrix;
    _key_event_callback = key_event_callback;
    _pinModeFunc = pinModeFunc;
    _digitalWriteFunc = digitalWriteFunc;
    _digitalReadFunc = digitalReadFunc;
    
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

void SQUIDMATRIX::unifiedPinMode(uint8_t pin, uint8_t mode) {
    if (_pinModeFunc) {
        _pinModeFunc(pin, mode);
    } else {
        // Fallback to direct Arduino calls
        ::pinMode(pin, mode);
    }
}

void SQUIDMATRIX::unifiedDigitalWrite(uint8_t pin, uint8_t value) {
    if (_digitalWriteFunc) {
        _digitalWriteFunc(pin, value);
    } else {
        // Fallback to direct Arduino calls
        ::digitalWrite(pin, value);
    }
}

uint8_t SQUIDMATRIX::unifiedDigitalRead(uint8_t pin) {
    if (_digitalReadFunc) {
        return _digitalReadFunc(pin);
    } else {
        // Fallback to direct Arduino calls
        return ::digitalRead(pin);
    }
}

bool SQUIDMATRIX::detectPinNeedsPullup(int pin) {
    SQUID_LOG_DEBUG(MATRIX_TAG, "Detecting pull-up requirement for pin %d", pin);
    
    // Step 1: Configure pin as INPUT (no pull-up) and read initial state
    unifiedPinMode(pin, INPUT);
    delayMicroseconds(25); // Allow signal to stabilize
    
    int initial_state = unifiedDigitalRead(pin);
    SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d initial state (INPUT): %d", pin, initial_state);
    
    // Step 2: If pin reads HIGH with no pull-up, it has external pull-up
    if (initial_state == HIGH) {
        SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d has EXTERNAL pull-up resistor", pin);
        return false; // No need for internal pull-up
    }
    
    // Step 3: If pin reads LOW, configure as INPUT_PULLUP and check if it goes HIGH
    unifiedPinMode(pin, INPUT_PULLUP);
    delayMicroseconds(25); // Allow internal pull-up to activate
    
    int pullup_state = unifiedDigitalRead(pin);
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
            unifiedPinMode(pin, INPUT_PULLUP);
            SQUID_LOG_DEBUG(MATRIX_TAG, "Configured pin %d as INPUT_PULLUP", pin);
        } else {
            unifiedPinMode(pin, INPUT);
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
            unifiedPinMode(pin, INPUT_PULLUP);
        } else {
            unifiedPinMode(pin, INPUT);
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
    unifiedPinMode(current_to_pin, OUTPUT);
    unifiedDigitalWrite(current_to_pin, LOW);
    delayMicroseconds(3); // Reduced from 10μs
    
    for (size_t switch_idx = 0; switch_idx < _matrix.size(); ++switch_idx) {
        const auto& pin_pair = _matrix[switch_idx];
        if (pin_pair.is_ground || pin_pair.to_pin != current_to_pin) continue;
        
        // Pins should already be in correct state so just read immediately
        bool pressed = (unifiedDigitalRead(pin_pair.from_pin) == LOW);
        
        _current_state[switch_idx] = pressed;
        
        // Immediate callback on state change
        if (_current_state[switch_idx] != _previous_state[switch_idx] && _key_event_callback) {
            _key_event_callback(switch_idx, pressed);
        }
    }
    
    // Restore TO pin
    bool to_pin_needs_pullup = getOptimalPinMode(current_to_pin);
    if (to_pin_needs_pullup) {
        unifiedPinMode(current_to_pin, INPUT_PULLUP);
    } else {
        unifiedPinMode(current_to_pin, INPUT);
    }
    
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
            unifiedPinMode(pin_pair.from_pin, INPUT_PULLUP);
        } else {
            unifiedPinMode(pin_pair.from_pin, INPUT);
        }
        
        delayMicroseconds(3);
        
        // Read the pin - LOW means pressed in both modes
        bool pressed = (unifiedDigitalRead(pin_pair.from_pin) == LOW);
        
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
            unifiedPinMode(pin_pair.from_pin, INPUT_PULLUP);
        } else {
            unifiedPinMode(pin_pair.from_pin, INPUT);
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

static const char* LAYER_KEYMAP_TAG = "SQUIDKEYMAP";

SQUIDKEYMAP::SQUIDKEYMAP() 
    : _press_callback(nullptr), 
      _release_callback(nullptr),
      _layer_change_callback(nullptr) {}

void SQUIDKEYMAP::begin(
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
    
    // Initialize combo tracking
    _key_combos.clear();
    _combo_key_to_combo_idx.clear();
    _combo_timeout_ms = 200;
    
    updateKeycodeMappings();
    
    SQUID_LOG_INFO(LAYER_KEYMAP_TAG, "Layer keymap initialized with %zu layers", _layers.size());
}

void SQUIDKEYMAP::handleKeyEvent(size_t switch_index, bool pressed) {
    if (switch_index >= getKeyCount()) {
        SQUID_LOG_WARN(LAYER_KEYMAP_TAG, "Invalid key position: %zu", switch_index);
        return;
    }
    
    // First, check if this key is part of any active combo sequence
    bool is_in_active_combo = false;
    if (!_key_combos.empty()) {
        // Update combo states first
        updateComboForKey(switch_index, pressed);
        
        // Check if this key is part of any combo that's currently being processed
        auto it = _combo_key_to_combo_idx.find(switch_index);
        if (it != _combo_key_to_combo_idx.end()) {
            for (size_t combo_idx : it->second) {
                if (combo_idx < _combo_states.size() && 
                    _combo_states[combo_idx].start_time > 0) {
                    // This key is part of an active combo sequence
                    is_in_active_combo = true;
                    break;
                }
            }
        }
    }
    
    // Only process normal key actions if:
    // 1. No combo is currently triggered, AND
    // 2. This key is not part of an active combo sequence
    if (!isAnyComboTriggered() && !isKeyInActiveCombo(switch_index)) {
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
    } else if (is_in_active_combo) {
        // Key is part of an active combo sequence - suppress normal key action
        SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Suppressing key %zu (part of active combo)", switch_index);
    }
}

void SQUIDKEYMAP::update() {
    // Check for combo timeouts (both regular and early)
    if (!_key_combos.empty()) {
        uint32_t now = millis();
        
        for (size_t i = 0; i < _key_combos.size(); i++) {
            auto& state = _combo_states[i];
            
            // Only check timeouts if combo hasn't triggered
            if (!state.triggered && state.start_time > 0) {
                // Check for regular timeout
                if (now - state.start_time > _key_combos[i].timeout_ms) {
                    resetComboState(i);
                    SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu timed out (regular)", i);
                }
                // Check for early timeout
                else if (shouldEarlyTimeout(i)) {
                    resetComboState(i);
                    // Early timeout already logged in shouldEarlyTimeout
                }
            }
        }
    }
}

void SQUIDKEYMAP::setDefaultLayer(uint8_t layer) {
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

void SQUIDKEYMAP::momentaryLayer(uint8_t layer, bool pressed) {
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

void SQUIDKEYMAP::toggleLayer(uint8_t layer) {
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

void SQUIDKEYMAP::layerOn(uint8_t layer) {
    if (layer < _layers.size() && !_layer_state.layer_states[layer]) {
        toggleLayer(layer);
    }
}

void SQUIDKEYMAP::layerOff(uint8_t layer) {
    if (layer < _layers.size() && _layer_state.layer_states[layer]) {
        toggleLayer(layer);
    }
}

uint8_t SQUIDKEYMAP::getActiveLayer() const {
    return _layer_state.active_layers.empty() ? 0 : _layer_state.active_layers.back();
}

bool SQUIDKEYMAP::isLayerActive(uint8_t layer) const {
    return std::find(_layer_state.active_layers.begin(), 
                    _layer_state.active_layers.end(), layer) != _layer_state.active_layers.end();
}

LayerKeymapEntry SQUIDKEYMAP::getKeyAt(size_t switch_index) const {
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

KeymapEntry SQUIDKEYMAP::getEffectiveKeyAt(size_t switch_index) const {
    LayerKeymapEntry layer_entry = getKeyAt(switch_index);
    if (layer_entry.action_type == LayerActionType::NORMAL_KEY) {
        return layer_entry.action.key;
    }
    return KeymapEntry(); // Return null key for non-normal actions
}

size_t SQUIDKEYMAP::getLayerCount() const {
    return _layers.size();
}

size_t SQUIDKEYMAP::getKeyCount() const {
    // Return the maximum key count across all layers
    size_t max_keys = 0;
    for (const auto& layer : _layers) {
        if (layer.size() > max_keys) {
            max_keys = layer.size();
        }
    }
    return max_keys;
}

void SQUIDKEYMAP::addCombo(const KeyComboConfig& combo) {
    _key_combos.push_back(combo);
    size_t combo_idx = _key_combos.size() - 1;
    
    // Map each key in the combo to this combo index
    for (const auto& key_spec : combo.key_specs) {
        if (key_spec.type == ComboKeySpec::Type::POSITION) {
            _combo_key_to_combo_idx[key_spec.value.position].push_back(combo_idx);  
        } else {
            // For keycodes, we need to find all positions that have this keycode
            auto positions = getPositionsForComboKey(key_spec);
            for (size_t pos : positions) {
                _combo_key_to_combo_idx[pos].push_back(combo_idx);
            }
            // Also track by keycode for dynamic checking
            _combo_keycode_to_combo_idx[key_spec.value.keycode].push_back(combo_idx);  
        }
    }
    
    // Initialize state for this combo
    _combo_states.emplace_back(combo.key_specs.size());
    
    // Initialize early timeout tracking for this combo
    _early_timeout_info.emplace_back();
}

void SQUIDKEYMAP::setCombos(const std::vector<KeyComboConfig>& combos) {
    clearCombos();
    for (const auto& combo : combos) {
        addCombo(combo);
    }
}

void SQUIDKEYMAP::clearCombos() {
    _key_combos.clear();
    _combo_key_to_combo_idx.clear();
    _combo_states.clear();
    _early_timeout_info.clear();
}

void SQUIDKEYMAP::updateComboForKey(size_t switch_index, bool pressed) {
    auto it = _combo_key_to_combo_idx.find(switch_index);
    if (it == _combo_key_to_combo_idx.end()) {
        return;
    }
    
    uint32_t now = millis();
    
    for (size_t combo_idx : it->second) {
        if (combo_idx >= _key_combos.size()) continue;
        
        auto& combo = _key_combos[combo_idx];
        auto& state = _combo_states[combo_idx];
        
        // Update early timeout tracking
        updateEarlyTimeoutInfo(combo_idx, pressed);
        
        // Find which key in the combo this position corresponds to
        // We need to check all key specs in the combo
        for (size_t i = 0; i < combo.key_specs.size(); i++) {
            const auto& key_spec = combo.key_specs[i];
            
            if (key_spec.type == ComboKeySpec::Type::POSITION) {
                if (key_spec.value.position == switch_index) {  
                    state.pressed_keys[i] = pressed;
                    
                    if (pressed) {
                        // Start timing on first key press
                        if (state.start_time == 0) {
                            state.start_time = now;
                            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu started", combo_idx);
                        }
                    }
                    
                    // Check if combo conditions are met
                    checkCombo(combo_idx);
                    break;
                }
            } else if (key_spec.type == ComboKeySpec::Type::KEYCODE) {
                // For keycodes, we need to check if this position has the matching keycode
                auto positions = getPositionsForComboKey(key_spec);
                if (std::find(positions.begin(), positions.end(), switch_index) != positions.end()) {
                    state.pressed_keys[i] = pressed;
                    
                    if (pressed) {
                        // Start timing on first key press
                        if (state.start_time == 0) {
                            state.start_time = now;
                            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu started", combo_idx);
                        }
                    }
                    
                    // Check if combo conditions are met
                    checkCombo(combo_idx);
                    break;
                }
            }
            // ANY_POSITION types will need different handling
        }
    }
}

void SQUIDKEYMAP::checkCombo(size_t combo_idx) {
    if (combo_idx >= _key_combos.size()) return;
    
    auto& combo = _key_combos[combo_idx];
    auto& state = _combo_states[combo_idx];
    
    // Check for early timeout first
    if (!state.triggered && shouldEarlyTimeout(combo_idx)) {
        // Early timeout - reset immediately
        resetComboState(combo_idx);
        return;
    }
    
    // Get current pressed keys from the combo state
    bool all_pressed = true;
    for (bool pressed : state.pressed_keys) {
        if (!pressed) {
            all_pressed = false;
            break;
        }
    }
    
    if (state.triggered) {
        // Combo already triggered, check for release
        bool all_released = true;
        for (bool pressed : state.pressed_keys) {
            if (pressed) {
                all_released = false;
                break;
            }
        }
        
        if (all_released) {
            // Send release event for combo action
            sendComboAction(combo.action, false);
            state.triggered = false;
            state.sent = false;
            state.start_time = 0;
            
            // Clear any pending key events that were suppressed
            for (size_t i = 0; i < combo.key_specs.size(); i++) {
                if (combo.key_specs[i].type == ComboKeySpec::Type::POSITION) {
                    _keys_in_active_combos.erase(combo.key_specs[i].value.position);
                }
            }
            
            // Reset early timeout info
            if (combo_idx < _early_timeout_info.size()) {
                _early_timeout_info[combo_idx] = EarlyTimeoutInfo();
            }
            
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu released", combo_idx);
        }
        return;
    }
    
    // Check if combo should trigger
    uint32_t now = millis();
    if (state.start_time == 0 || now - state.start_time > combo.timeout_ms) {
        // Timeout - reset
        resetComboState(combo_idx);
        return;
    }
    
    // Check if all required keys are pressed
    if (all_pressed) {
        // Mark all keys in this combo as part of active combo
        for (size_t i = 0; i < combo.key_specs.size(); i++) {
            if (combo.key_specs[i].type == ComboKeySpec::Type::POSITION) {
                _keys_in_active_combos.insert(combo.key_specs[i].value.position);
            }
        }
        
        // Trigger the combo
        triggerCombo(combo_idx, true);
    }
}

void SQUIDKEYMAP::triggerCombo(size_t combo_idx, bool pressed) {
    if (combo_idx >= _key_combos.size()) return;
    
    auto& combo = _key_combos[combo_idx];
    auto& state = _combo_states[combo_idx];
    
    state.triggered = pressed;
    sendComboAction(combo.action, pressed);
    
    SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu %s", combo_idx, 
                  pressed ? "triggered" : "released");
}

void SQUIDKEYMAP::sendComboAction(const KeymapEntry& action, bool pressed) {
    if (pressed) {
        if (_press_callback) {
            _press_callback(action);
        }
    } else {
        if (_release_callback) {
            _release_callback(action);
        }
    }
}

bool SQUIDKEYMAP::isAnyComboTriggered() const {
    for (const auto& state : _combo_states) {
        if (state.triggered) return true;
    }
    return false;
}

size_t SQUIDKEYMAP::getComboCount() const {
    return _key_combos.size();
}

bool SQUIDKEYMAP::isComboActive(size_t combo_idx) const {
    if (combo_idx >= _combo_states.size()) return false;
    return _combo_states[combo_idx].triggered;
}

void SQUIDKEYMAP::updateKeycodeMappings() {
    _keycode_to_positions.clear();
    
    for (size_t pos = 0; pos < getKeyCount(); pos++) {
        KeymapEntry key = getEffectiveKeyAt(pos);
        if (key.type != KeypressType::NKRO_KEY || key.key.nkro_key.get() != 0) {
            _keycode_to_positions[key].push_back(pos);
        }
    }
}

std::vector<size_t> SQUIDKEYMAP::getPositionsForComboKey(const ComboKeySpec& spec) {
    std::vector<size_t> positions;
    
    switch (spec.type) {
        case ComboKeySpec::Type::POSITION:
            positions.push_back(spec.value.position);  
            break;
            
        case ComboKeySpec::Type::KEYCODE: {
            auto it = _keycode_to_positions.find(spec.value.keycode);  
            if (it != _keycode_to_positions.end()) {
                positions = it->second;
            }
            break;
        }
            
        case ComboKeySpec::Type::ANY_POSITION:
            // Will be checked dynamically in checkComboKeyPressed
            break;
    }
    
    return positions;
}

bool SQUIDKEYMAP::checkComboKeyPressed(const ComboKeySpec& spec, const std::vector<bool>& key_states) {
    switch (spec.type) {
        case ComboKeySpec::Type::POSITION:
            if (spec.value.position < key_states.size()) {  
                return key_states[spec.value.position];  
            }
            return false;
            
        case ComboKeySpec::Type::KEYCODE: {
            auto it = _keycode_to_positions.find(spec.value.keycode); 
            if (it != _keycode_to_positions.end()) {
                for (size_t pos : it->second) {
                    if (pos < key_states.size() && key_states[pos]) {
                        return true;
                    }
                }
            }
            return false;
        }
            
        case ComboKeySpec::Type::ANY_POSITION:
            // Check if any position has this keycode pressed
            for (size_t pos = 0; pos < key_states.size(); pos++) {
                if (key_states[pos]) {
                    KeymapEntry key = getEffectiveKeyAt(pos);
                    if (key.type == spec.value.keycode.type) {
                        // Compare based on type
                        switch (key.type) {
                            case KeypressType::NKRO_KEY:
                                if (key.key.nkro_key == spec.value.keycode.key.nkro_key) return true;
                                break;
                            case KeypressType::MOD_KEY:
                                if (key.key.mod_key == spec.value.keycode.key.mod_key) return true;
                                break;
                            case KeypressType::MEDIA_KEY:
                                if (key.key.media_key == spec.value.keycode.key.media_key) return true;
                                break;
                            case KeypressType::MOUSE_KEY:
                                if (key.key.mouse_key == spec.value.keycode.key.mouse_key) return true;
                                break;
                            case KeypressType::GAMEPAD_BUTTON:
                                if (key.key.gamepad_button == spec.value.keycode.key.gamepad_button) return true;
                                break;
                            case KeypressType::STENO_KEY:
                                if (key.key.steno_key == spec.value.keycode.key.steno_key) return true;
                                break;
                            default:
                                if (memcmp(&key.key, &spec.value.keycode.key, sizeof(KeymapValue)) == 0) {
                                    return true;
                                }
                        }
                    }
                }
            }
            return false;
    }
    
    return false;
}

bool SQUIDKEYMAP::isKeyInActiveCombo(size_t switch_index) const {
    // Check if key is marked as part of any active combo
    if (_keys_in_active_combos.find(switch_index) != _keys_in_active_combos.end()) {
        return true;
    }
    
    // Also check if it's part of any combo that has started timing
    auto it = _combo_key_to_combo_idx.find(switch_index);
    if (it != _combo_key_to_combo_idx.end()) {
        for (size_t combo_idx : it->second) {
            if (combo_idx < _combo_states.size() && 
                _combo_states[combo_idx].start_time > 0) {
                return true;
            }
        }
    }
    
    return false;
}

void SQUIDKEYMAP::resetComboState(size_t combo_idx) {
    if (combo_idx >= _key_combos.size() || combo_idx >= _combo_states.size()) {
        return;
    }
    
    auto& combo = _key_combos[combo_idx];
    auto& state = _combo_states[combo_idx];
    
    // Clear any pending key events that were suppressed
    for (size_t i = 0; i < combo.key_specs.size(); i++) {
        if (combo.key_specs[i].type == ComboKeySpec::Type::POSITION) {
            _keys_in_active_combos.erase(combo.key_specs[i].value.position);
        }
    }
    
    // Reset state
    state.start_time = 0;
    state.triggered = false;
    state.sent = false;
    state.pressed_keys.assign(state.pressed_keys.size(), false);
    
    // Reset early timeout info
    if (combo_idx < _early_timeout_info.size()) {
        _early_timeout_info[combo_idx] = EarlyTimeoutInfo();
    }
    
    SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu reset", combo_idx);
}

void SQUIDKEYMAP::updateEarlyTimeoutInfo(size_t combo_idx, bool key_pressed) {
    if (combo_idx >= _early_timeout_info.size()) return;
    
    auto& info = _early_timeout_info[combo_idx];
    uint32_t now = millis();
    
    if (key_pressed) {
        info.last_key_press_time = now;
        info.active_key_count++;
        
        if (_combo_debug_enabled) {
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu: Key pressed, active count: %zu", 
                          combo_idx, info.active_key_count);
        }
    } else {
        info.last_key_release_time = now;
        if (info.active_key_count > 0) {
            info.active_key_count--;
        }
        
        if (_combo_debug_enabled) {
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu: Key released, active count: %zu", 
                          combo_idx, info.active_key_count);
        }
    }
}

bool SQUIDKEYMAP::shouldEarlyTimeout(size_t combo_idx) const {
    if (combo_idx >= _early_timeout_info.size() || combo_idx >= _key_combos.size()) {
        return false;
    }
    
    const auto& info = _early_timeout_info[combo_idx];
    const auto& combo = _key_combos[combo_idx];
    
    // If no keys are currently pressed and some time has passed since the last key activity
    if (info.active_key_count == 0 && info.last_key_release_time > 0) {
        uint32_t now = millis();
        uint32_t time_since_last_release = now - info.last_key_release_time;
        
        // Early timeout: if it's been more than 50ms since the last key release
        // and we haven't triggered the combo yet, it's not going to happen
        if (time_since_last_release > 50) { // 50ms is enough time for a combo
            if (_combo_debug_enabled) {
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu: Early timeout after %ums", 
                              combo_idx, time_since_last_release);
            }
            return true;
        }
    }
    
    return false;
}
