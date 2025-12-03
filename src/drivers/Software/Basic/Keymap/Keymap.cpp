/**
 * @file Keymap.cpp
 * @brief Implementation of the keymap functionality
 */

#include "Keymap.h"

// ============================================================================
// Keymap Implementation
// ============================================================================

static const char* LAYER_KEYMAP_TAG = "SQUIDKEYMAP";

SQUIDKEYMAP::SQUIDKEYMAP() 
    : _press_callback(nullptr), 
      _release_callback(nullptr),
      _layer_change_callback(nullptr),
      _last_any_key_press_time(0),
      _last_key_pressed(SIZE_MAX),
      _last_normal_key_time(0) {}

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
    
    // Initialize key tap tracking
    _key_tap_info.clear();
    if (getKeyCount() > 0) {
        _key_tap_info.resize(getKeyCount());
    }
    
    // Initialize tap/hold tracking - FIX: Scan layers for tap-hold keys
    _tap_hold_states.clear();
    if (getKeyCount() > 0) {
        _tap_hold_states.resize(getKeyCount());
    }
    
    // Initialize typing flow
    _in_typing_flow = false;
    _typing_flow_start = 0;
    
    updateKeycodeMappings();
    
    SQUID_LOG_INFO(LAYER_KEYMAP_TAG, "Layer keymap initialized with %zu layers", _layers.size());
}

void SQUIDKEYMAP::handleKeyEvent(size_t switch_index, bool pressed) {
    if (switch_index >= getKeyCount()) {
        SQUID_LOG_WARN(LAYER_KEYMAP_TAG, "Invalid key position: %zu", switch_index);
        return;
    }
    
    uint32_t now = millis();
    
    // Update typing flow detection
    bool was_in_typing_flow = _in_typing_flow;
    _in_typing_flow = detectTypingFlow(switch_index, pressed);
    
    if (pressed) {
        _last_any_key_press_time = now;
        _last_key_pressed = switch_index;
    }
    
    // Update tap tracking FIRST
    updateKeyTapInfo(switch_index, pressed);
    
    // Check if this key is part of any combo
    bool is_part_of_combo = (_combo_key_to_combo_idx.find(switch_index) != _combo_key_to_combo_idx.end());
    
    // Update combo states if this key is part of a combo
    if (is_part_of_combo) {
        updateComboForKey(switch_index, pressed);
    }
    
    // CRITICAL CHANGE: For keys that are part of combos, delay processing
    // to see if they form a combo. Only process immediately if we're in typing flow
    if (is_part_of_combo && !pressed) {
        // Key release - check if we need to send delayed press/release
        if (switch_index < _key_tap_info.size()) {
            auto& tap_info = _key_tap_info[switch_index];
            if (!tap_info.sent_as_normal && tap_info.is_tap) {
                // This was a quick tap that was suppressed - send it now
                tap_info.sent_as_normal = true;
                processNormalKey(switch_index, true);
                processNormalKey(switch_index, false);
                
                if (_combo_debug_enabled) {
                    SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Sending suppressed tap for key %zu", switch_index);
                }
            }
        }
        return;
    }
    
    // For key presses that are part of combos, add a small delay
    if (is_part_of_combo && pressed) {
        // Add a small delay (e.g., 30ms) to see if this becomes part of a combo
        _delayed_key_events.emplace_back(switch_index, pressed, now + 30);
        return;
    }
    
    // Process normal keys immediately
    processNormalKey(switch_index, pressed);
}

void SQUIDKEYMAP::updateCombos() {
    uint32_t now = millis();
    
    // Process any delayed key events
    processDelayedEvents();
    
    // Check for combo timeouts (both regular and early)
    if (!_key_combos.empty()) {
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
            // If combo is triggered, cancel any delayed events for its keys
            else if (state.triggered) {
                const auto& combo = _key_combos[i];
                for (const auto& key_spec : combo.key_specs) {
                    if (key_spec.type == ComboKeySpec::Type::POSITION) {
                        // Cancel delayed events for this key
                        auto it = std::remove_if(_delayed_key_events.begin(), _delayed_key_events.end(),
                            [&key_spec](const DelayedKeyEvent& event) {
                                return event.switch_index == key_spec.value.position && event.pressed;
                            });
                        _delayed_key_events.erase(it, _delayed_key_events.end());
                    }
                }
            }
        }
    }
}

void SQUIDKEYMAP::update() {
    uint32_t now = millis();
    
    // Process delayed key events
    processDelayedEvents();
    
    // Check for tap/hold timeouts
    for (size_t i = 0; i < _tap_hold_states.size(); i++) {
        auto& tap_hold = _tap_hold_states[i];
        
        if (tap_hold.is_tap_hold_key && tap_hold.pending_tap) {
            if (now > tap_hold.tap_timeout && !tap_hold.is_held) {
                // Tap timed out - this becomes a hold
                tap_hold.is_held = true;
                tap_hold.pending_tap = false;
                
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Tap/Hold key %zu - switched to HOLD action", i);
                
                // Send hold action press
                if (_press_callback) {
                    _press_callback(tap_hold.hold_action);
                    tap_hold.hold_action_sent = true;
                }
            }
        }
    }
    
    // Update combos
    updateCombos();
}

void SQUIDKEYMAP::processDelayedEvents() {
    uint32_t now = millis();
    
    // Process delayed key events in order
    auto it = _delayed_key_events.begin();
    while (it != _delayed_key_events.end()) {
        if (now >= it->scheduled_time) {
            // Time to process this event
            // Check if this key is still part of an active combo
            if (!isKeyInActiveCombo(it->switch_index)) {
                processNormalKey(it->switch_index, it->pressed);
            }
            it = _delayed_key_events.erase(it);
        } else {
            ++it;
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
    
    // Initialize tap/hold info for this combo
    _combo_tap_hold_info.emplace_back();
    auto& tap_hold_info = _combo_tap_hold_info.back();
    
    // Track which keys are part of this combo
    for (const auto& key_spec : combo.key_specs) {
        if (key_spec.type == ComboKeySpec::Type::POSITION) {
            tap_hold_info.combo_keys.push_back(key_spec.value.position);
        }
    }
    
    // Resize tap_hold_states if needed
    size_t max_key = getKeyCount();
    if (_tap_hold_states.size() < max_key) {
        _tap_hold_states.resize(max_key);
    }
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
    _combo_tap_hold_info.clear();
    
    // Reset all tap info
    for (auto& tap_info : _key_tap_info) {
        tap_info = KeyTapInfo();
    }
    
    // Reset tap/hold states
    for (auto& tap_hold : _tap_hold_states) {
        tap_hold = TapHoldState();
    }
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
        
        // If this is a key release and it was a quick tap, don't update combo state
        // (let it be processed as a normal key)
        if (!pressed && isKeyTap(switch_index)) {
            if (_combo_debug_enabled) {
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Key %zu was a tap - not updating combo %zu", 
                              switch_index, combo_idx);
            }
            continue;
        }
        
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
        // Timeout - reset (but DON'T send delayed keys here)
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
            size_t pos = combo.key_specs[i].value.position;
            _keys_in_active_combos.erase(pos);
            
            // Reset tap info for this key
            if (pos < _key_tap_info.size()) {
                _key_tap_info[pos].reset();
            }
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

void SQUIDKEYMAP::updateKeyTapInfo(size_t switch_index, bool pressed) {
    if (switch_index >= _key_tap_info.size()) return;
    
    auto& tap_info = _key_tap_info[switch_index];
    uint32_t now = millis();
    
    if (pressed) {
        // Reset if it's been a while since last activity
        if (now - tap_info.release_time > ROLLOVER_THRESHOLD_MS * 2) {
            tap_info.tap_count = 0;
        }
        
        // Reset tracking for new press
        tap_info.press_time = now;
        tap_info.release_time = 0;
        tap_info.is_tap = false;
        tap_info.sent_as_normal = false;
        tap_info.press_sent = false;
        tap_info.release_sent = false;
        
        if (_combo_debug_enabled) {
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Key %zu pressed at %u (tap count: %u)", 
                          switch_index, now, tap_info.tap_count);
        }
    } else {
        tap_info.release_time = now;
        
        // Determine if this was a tap (quick press-release)
        if (tap_info.press_time > 0) {
            uint32_t press_duration = now - tap_info.press_time;
            
            if (press_duration <= TAP_TIMEOUT_MS) {
                tap_info.is_tap = true;
                tap_info.tap_count++;
                
                if (_combo_debug_enabled) {
                    SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Key %zu was a tap (%ums, count: %u)", 
                                  switch_index, press_duration, tap_info.tap_count);
                }
            } else {
                tap_info.is_tap = false;
                
                if (_combo_debug_enabled) {
                    SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Key %zu was a hold (%ums)", 
                                  switch_index, press_duration);
                }
            }
        }
    }
}

bool SQUIDKEYMAP::isKeyTap(size_t switch_index) const {
    if (switch_index >= _key_tap_info.size()) return false;
    return _key_tap_info[switch_index].is_tap;
}

bool SQUIDKEYMAP::isTypingPattern(size_t switch_index, bool pressed) const {
    if (!pressed) return false;
    
    uint32_t now = millis();
    
    // If this is the first key press in a while, it might be typing
    if (now - _last_any_key_press_time > ROLLOVER_THRESHOLD_MS) {
        return true;
    }
    
    // If we're pressing the same key repeatedly, it's definitely typing
    if (_last_key_pressed == switch_index) {
        return true;
    }
    
    // If we just sent a normal key and now pressing another key quickly, it's typing
    if (now - _last_normal_key_time < ROLLOVER_THRESHOLD_MS) {
        return true;
    }
    
    return false;
}

void SQUIDKEYMAP::setComboTapHold(size_t combo_idx, bool enabled, uint16_t tap_timeout) {
    if (combo_idx >= _combo_tap_hold_info.size()) return;
    
    auto& tap_hold_info = _combo_tap_hold_info[combo_idx];
    tap_hold_info.is_dual_function = enabled;
    tap_hold_info.tap_timeout_ms = tap_timeout;
    
    // Mark the keys involved as having tap/hold behavior
    for (size_t key_idx : tap_hold_info.combo_keys) {
        if (key_idx < _tap_hold_states.size()) {
            _tap_hold_states[key_idx].is_tap_hold_key = enabled;
        }
    }
    
    if (_combo_debug_enabled) {
        SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Combo %zu tap/hold %s (timeout: %ums)", 
                      combo_idx, enabled ? "enabled" : "disabled", tap_timeout);
    }
}

bool SQUIDKEYMAP::shouldSuppressForCombo(size_t switch_index, bool pressed) const {
    if (!pressed) {
        // Never suppress key releases
        return false;
    }
    
    // Check if this key is part of any combo
    auto it = _combo_key_to_combo_idx.find(switch_index);
    if (it == _combo_key_to_combo_idx.end()) {
        return false; // Not part of any combo
    }
    
    // If this looks like a typing pattern, don't suppress
    if (isTypingPattern(switch_index, pressed)) {
        return false;
    }
    
    // Check if any combo involving this key is currently "active" (timing)
    for (size_t combo_idx : it->second) {
        if (combo_idx < _combo_states.size() && 
            _combo_states[combo_idx].start_time > 0) {
            
            // Only suppress if we're past the tap threshold
            uint32_t now = millis();
            if (switch_index < _key_tap_info.size() && 
                _key_tap_info[switch_index].press_time > 0) {
                uint32_t press_duration = now - _key_tap_info[switch_index].press_time;
                
                // Don't suppress for the first 50ms - allow quick taps
                if (press_duration < 50) {
                    return false;
                }
            }
            
            return true;
        }
    }
    
    return false;
}

bool SQUIDKEYMAP::shouldProcessAsNormalKey(size_t switch_index) const {
    if (switch_index >= _key_tap_info.size()) return true;
    
    const auto& tap_info = _key_tap_info[switch_index];
    
    // If this key was a tap (quick press-release), process it as normal
    if (tap_info.is_tap) {
        return true;
    }
    
    // If this key is currently pressed but hasn't been held long enough to be part of a combo
    uint32_t now = millis();
    if (tap_info.press_time > 0 && tap_info.release_time == 0) {
        // Key is still pressed - check if it's part of any active combo sequence
        auto it = _combo_key_to_combo_idx.find(switch_index);
        if (it != _combo_key_to_combo_idx.end()) {
            for (size_t combo_idx : it->second) {
                if (combo_idx < _combo_states.size() && 
                    _combo_states[combo_idx].start_time > 0) {
                    // This key is part of an active combo sequence
                    // Only process as normal if it's a very quick press
                    if ((now - tap_info.press_time) < 30) { // 30ms grace period
                        return true;
                    }
                    return false;
                }
            }
        }
    }
    
    return true;
}

void SQUIDKEYMAP::processNormalKey(size_t switch_index, bool pressed) {
    // Check if this is a tap/hold key
    LayerKeymapEntry action = getKeyAt(switch_index);
    
    if (action.action_type == LayerActionType::TAP_HOLD_KEY) {
        processTapHoldKey(switch_index, pressed, action);
        return;
    }
    
    // Check if we've already sent this event
    if (switch_index < _key_tap_info.size()) {
        auto& tap_info = _key_tap_info[switch_index];
        
        if (pressed && tap_info.press_sent) {
            if (_combo_debug_enabled) {
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Skipping duplicate press for key %zu", switch_index);
            }
            return;
        }
        
        if (!pressed && tap_info.release_sent) {
            if (_combo_debug_enabled) {
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Skipping duplicate release for key %zu", switch_index);
            }
            return;
        }
    }
    
    // Find the highest priority non-transparent key
    for (int i = _layer_state.active_layers.size() - 1; i >= 0; i--) {
        uint8_t layer = _layer_state.active_layers[i];
        if (switch_index < _layers[layer].size()) {
            action = _layers[layer][switch_index];
            if (action.action_type != LayerActionType::TRANSPARENT) {
                break;
            }
        }
    }
    
    // Mark that we sent this as a normal key
    if (switch_index < _key_tap_info.size()) {
        auto& tap_info = _key_tap_info[switch_index];
        tap_info.sent_as_normal = true;
        
        if (pressed) {
            tap_info.press_sent = true;
        } else {
            tap_info.release_sent = true;
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

void SQUIDKEYMAP::processDelayedNormalKey(size_t switch_index) {
    if (switch_index >= _key_tap_info.size()) return;
    
    auto& tap_info = _key_tap_info[switch_index];
    
    // Don't send if we've already sent a normal key for this press
    if (tap_info.sent_as_normal) {
        return;
    }
    
    // Don't send if this key is currently part of an active combo
    if (isKeyInActiveCombo(switch_index)) {
        return;
    }
    
    // Only send if this was actually a tap (quick press-release)
    if (!tap_info.is_tap) {
        return;
    }
    
    // Mark as sent
    tap_info.sent_as_normal = true;
    
    // Send both press and release events
    processNormalKey(switch_index, true);
    processNormalKey(switch_index, false);
    
    if (_combo_debug_enabled) {
        SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Sent delayed normal key for tap %zu", switch_index);
    }
}

void SQUIDKEYMAP::processTapHoldKey(size_t switch_index, bool pressed, const LayerKeymapEntry& action) {
    if (switch_index >= _tap_hold_states.size()) return;
    
    auto& tap_hold = _tap_hold_states[switch_index];
    uint32_t now = millis();
    
    // Check if this key actually has tap/hold behavior in current layer
    if (!isTapHoldKey(switch_index)) {
        // Fall back to normal key processing
        processNormalKey(switch_index, pressed);
        return;
    }
    
    if (pressed) {
        // Key pressed - start tap/hold detection
        tap_hold.press_time = now;
        tap_hold.pending_tap = true;
        tap_hold.is_held = false;
        tap_hold.hold_action_sent = false;
        tap_hold.tap_timeout = now + tap_hold.tap_timeout_ms;
        
        // Store actions from current layer's keymap entry
        tap_hold.tap_action = action.action.key;
        tap_hold.hold_action = action.action.hold_action;
        
        SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Tap/Hold key %zu pressed - tap timeout: %u (now: %u)", 
                       switch_index, tap_hold.tap_timeout, now);
    } else {
        // Key released - CRITICAL: Check if we've passed the tap timeout
        uint32_t press_duration = now - tap_hold.press_time;
        
        // FIRST: Check if the hold action was already sent (timeout occurred while key was pressed)
        if (tap_hold.is_held && tap_hold.hold_action_sent) {
            // This was a hold - release the hold action
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Tap/Hold key %zu - releasing HOLD action (press duration: %ums)", 
                           switch_index, press_duration);
            if (_release_callback) {
                _release_callback(tap_hold.hold_action);
            }
        } 
        // SECOND: Check if this is still within tap window
        else if (tap_hold.pending_tap && !tap_hold.is_held) {
            // Check if enough time has passed to be considered a hold
            if (press_duration >= tap_hold.tap_timeout_ms) {
                // Actually, this should have been a hold but hold action wasn't sent yet
                // This can happen if update() hasn't run since the timeout
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Tap/Hold key %zu - LATE HOLD detected (duration: %ums)", 
                               switch_index, press_duration);
                
                // Send hold press and release
                if (_press_callback) {
                    _press_callback(tap_hold.hold_action);
                }
                if (_release_callback) {
                    _release_callback(tap_hold.hold_action);
                }
            } else {
                // This was a genuine tap
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Tap/Hold key %zu - sending TAP action (duration: %ums)", 
                               switch_index, press_duration);
                if (_press_callback) {
                    _press_callback(tap_hold.tap_action);
                }
                if (_release_callback) {
                    _release_callback(tap_hold.tap_action);
                }
            }
        }
        
        tap_hold.reset();
    }
}

void SQUIDKEYMAP::updateTapHoldState(size_t switch_index, bool pressed) {
    if (switch_index >= _tap_hold_states.size()) return;
    
    auto& tap_hold = _tap_hold_states[switch_index];
    uint32_t now = millis();
    
    if (pressed) {
        tap_hold.pending_tap = true;
        tap_hold.tap_timeout = now + 200; // Default 200ms tap timeout
    } else {
        if (tap_hold.pending_tap && now <= tap_hold.tap_timeout) {
            // Send tap action
            sendTapAction(switch_index);
        }
        tap_hold.reset();
    }
}

bool SQUIDKEYMAP::isTapHoldKey(size_t switch_index) const {
    if (switch_index >= getKeyCount()) return false;
    
    // Check current active layers for tap/hold behavior
    for (int i = _layer_state.active_layers.size() - 1; i >= 0; i--) {
        uint8_t layer = _layer_state.active_layers[i];
        if (switch_index < _layers[layer].size()) {
            const auto& entry = _layers[layer][switch_index];
            if (entry.action_type == LayerActionType::TAP_HOLD_KEY) {
                return true;
            }
            if (entry.action_type != LayerActionType::TRANSPARENT) {
                // Found a non-transparent, non-taphold key
                return false;
            }
        }
    }
    return false;
}

void SQUIDKEYMAP::sendTapAction(size_t switch_index) {
    if (switch_index >= _tap_hold_states.size()) return;
    
    auto& tap_hold = _tap_hold_states[switch_index];
    
    // Find the normal key action for this position
    LayerKeymapEntry action = getKeyAt(switch_index);
    
    if (action.action_type == LayerActionType::NORMAL_KEY) {
        // Send the normal key action
        if (_press_callback) {
            _press_callback(action.action.key);
        }
        if (_release_callback) {
            // Small delay for realistic typing
            delay(10);
            _release_callback(action.action.key);
        }
        
        if (_combo_debug_enabled) {
            SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Sent tap action for key %zu", switch_index);
        }
    }
}

void SQUIDKEYMAP::cancelPendingTap(size_t switch_index) {
    if (switch_index >= _tap_hold_states.size()) return;
    _tap_hold_states[switch_index].reset();
}

bool SQUIDKEYMAP::areComboKeysBeingHeld(size_t combo_idx) const {
    if (combo_idx >= _combo_tap_hold_info.size()) return false;
    
    const auto& tap_hold_info = _combo_tap_hold_info[combo_idx];
    uint32_t now = millis();
    
    for (size_t key_idx : tap_hold_info.combo_keys) {
        if (key_idx < _key_tap_info.size()) {
            const auto& tap_info = _key_tap_info[key_idx];
            
            // Key is considered "held" if it's been pressed for > 50ms
            if (tap_info.press_time > 0 && tap_info.release_time == 0) {
                if ((now - tap_info.press_time) > 50) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool SQUIDKEYMAP::areComboKeysBeingTapped(size_t combo_idx) const {
    if (combo_idx >= _combo_tap_hold_info.size()) return false;
    
    const auto& tap_hold_info = _combo_tap_hold_info[combo_idx];
    
    for (size_t key_idx : tap_hold_info.combo_keys) {
        if (key_idx < _key_tap_info.size()) {
            const auto& tap_info = _key_tap_info[key_idx];
            
            // Key is considered "tapped" if it was a quick press-release
            if (!tap_info.is_tap) {
                return false;
            }
        }
    }
    
    return true;
}

bool SQUIDKEYMAP::detectTypingFlow(size_t switch_index, bool pressed) {
    if (!pressed) {
        // Releases don't affect typing flow
        return _in_typing_flow;
    }
    
    uint32_t now = millis();
    
    // If we're already in typing flow, check if we should stay in it
    if (_in_typing_flow) {
        if (now - _typing_flow_start > 1000) {
            // Typing flow expired (1 second of inactivity)
            _in_typing_flow = false;
        } else {
            // Still in typing flow
            _typing_flow_start = now;
            return true;
        }
    }
    
    // Check if this looks like typing
    if (_last_any_key_press_time > 0) {
        uint32_t time_since_last_key = now - _last_any_key_press_time;
        
        if (time_since_last_key < TYPING_FLOW_THRESHOLD) {
            // Rapid keypresses - enter typing flow
            _in_typing_flow = true;
            _typing_flow_start = now;
            
            if (_combo_debug_enabled) {
                SQUID_LOG_DEBUG(LAYER_KEYMAP_TAG, "Entered typing flow (gap: %ums)", time_since_last_key);
            }
            
            return true;
        }
    }
    
    return false;
}
