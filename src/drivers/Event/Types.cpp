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
      _current_scan_col(0),
      _scan_initialized(false) {}

void SQUIDMATRIX::begin(const squid_matrix& matrix,
                       std::function<void(size_t, size_t, bool)> key_event_callback) {
    _matrix = matrix;
    _key_event_callback = key_event_callback;
    
    // Initialize state matrices
    _current_state.clear();
    _previous_state.clear();
    
    for (size_t row = 0; row < _matrix.size(); ++row) {
        _current_state.emplace_back(_matrix[row].size(), false);
        _previous_state.emplace_back(_matrix[row].size(), false);
    }
    
    // Extract all unique pins
    extractUniquePins();
    
    // Detect pull-up requirements for all pins
    detectAllPinPullupRequirements();
    
    // Initialize pins with optimal configurations
    initializePins();
    
    _scan_initialized = true;
    
    SQUID_LOG_INFO(MATRIX_TAG, "Smart matrix initialized with %zu rows", _matrix.size());
    SQUID_LOG_INFO(MATRIX_TAG, "Total unique pins: %zu", _unique_from_pins.size());
    
    // Print pull-up detection results
    printPinPullupInfo();
}

void SQUIDMATRIX::extractUniquePins() {
    _unique_from_pins.clear();
    _unique_to_pins.clear();
    _pin_needs_pullup.clear();
    
    // Extract ALL unique pins
    for (size_t row = 0; row < _matrix.size(); ++row) {
        for (size_t col = 0; col < _matrix[row].size(); ++col) {
            const auto& pin_pair = _matrix[row][col];
            
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
    delayMicroseconds(100); // Allow signal to stabilize
    
    int initial_state = digitalRead(pin);
    SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d initial state (INPUT): %d", pin, initial_state);
    
    // Step 2: If pin reads HIGH with no pull-up, it has external pull-up
    if (initial_state == HIGH) {
        SQUID_LOG_DEBUG(MATRIX_TAG, "Pin %d has EXTERNAL pull-up resistor", pin);
        return false; // No need for internal pull-up
    }
    
    // Step 3: If pin reads LOW, configure as INPUT_PULLUP and check if it goes HIGH
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(100); // Allow internal pull-up to activate
    
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
    // Get current TO pin to activate
    int current_to_pin = _unique_to_pins[_current_scan_col];
    
    // Configure TO pin as OUTPUT LOW for activation
    pinMode(current_to_pin, OUTPUT);
    digitalWrite(current_to_pin, LOW);
    
    // Small delay for signal stabilization
    delayMicroseconds(10); // Reduced from 15ms due to optimized detection
    
    // Scan all switches connected to this TO pin
    for (size_t row = 0; row < _matrix.size(); ++row) {
        for (size_t col = 0; col < _matrix[row].size(); ++col) {
            const auto& pin_pair = _matrix[row][col];
            
            // Skip GND switches and switches not connected to current TO pin
            if (pin_pair.is_ground || pin_pair.to_pin != current_to_pin) {
                continue;
            }
            
            // Use pre-detected optimal pin mode for FROM pin
            bool needs_pullup = getOptimalPinMode(pin_pair.from_pin);
            if (needs_pullup) {
                pinMode(pin_pair.from_pin, INPUT_PULLUP);
            } else {
                pinMode(pin_pair.from_pin, INPUT);
            }
            
            delayMicroseconds(3); // Reduced stabilization time
            
            // Read the pin - LOW means pressed in both modes
            bool pressed = (digitalRead(pin_pair.from_pin) == LOW);
            
            // Update state
            _current_state[row][col] = pressed;
            
            // Check for state change
            if (_current_state[row][col] != _previous_state[row][col]) {
                SQUID_LOG_DEBUG(MATRIX_TAG, "Matrix key: row=%zu, col=%zu, %s (TO: %d, Mode: %s)", 
                             row, col, pressed ? "PRESSED" : "RELEASED", 
                             current_to_pin, needs_pullup ? "PULLUP" : "INPUT");
                
                if (_key_event_callback) {
                    _key_event_callback(row, col, pressed);
                }
            }
            
            // Restore FROM pin to its optimal safe state
            if (needs_pullup) {
                pinMode(pin_pair.from_pin, INPUT_PULLUP);
            } else {
                pinMode(pin_pair.from_pin, INPUT);
            }
        }
    }
    
    // Deactivate current column - restore TO pin to optimal safe state
    bool to_pin_needs_pullup = getOptimalPinMode(current_to_pin);
    if (to_pin_needs_pullup) {
        pinMode(current_to_pin, INPUT_PULLUP);
    } else {
        pinMode(current_to_pin, INPUT);
    }
    
    // Move to next column for next scan
    _current_scan_col = (_current_scan_col + 1) % _unique_to_pins.size();
}

void SQUIDMATRIX::scanDirectGND() {
    // Direct scanning for GND-based switches (no TO pins)
    for (size_t row = 0; row < _matrix.size(); ++row) {
        for (size_t col = 0; col < _matrix[row].size(); ++col) {
            const auto& pin_pair = _matrix[row][col];
            
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
            
            delayMicroseconds(3); // Reduced stabilization time
            
            // Read the pin - LOW means pressed in both modes
            bool pressed = (digitalRead(pin_pair.from_pin) == LOW);
            
            // Update state
            _current_state[row][col] = pressed;
            
            // Check for state change
            if (_current_state[row][col] != _previous_state[row][col]) {
                SQUID_LOG_DEBUG(MATRIX_TAG, "GND key: row=%zu, col=%zu, %s (Mode: %s)", 
                             row, col, pressed ? "PRESSED" : "RELEASED",
                             needs_pullup ? "PULLUP" : "INPUT");
                
                if (_key_event_callback) {
                    _key_event_callback(row, col, pressed);
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
}

bool SQUIDMATRIX::isPressed(size_t row, size_t col) const {
    if (row < _current_state.size() && col < _current_state[row].size()) {
        return _current_state[row][col];
    }
    return false;
}

size_t SQUIDMATRIX::getRowCount() const {
    return _matrix.size();
}

size_t SQUIDMATRIX::getColCount() const {
    if (_matrix.empty()) return 0;
    return _matrix[0].size();
}

void SQUIDMATRIX::printMatrixState() {
    SQUID_LOG_DEBUG(MATRIX_TAG, "Current matrix state:");
    for (size_t row = 0; row < _current_state.size(); ++row) {
        std::string row_state;
        for (size_t col = 0; col < _current_state[row].size(); ++col) {
            row_state += _current_state[row][col] ? "1" : "0";
            if (col < _current_state[row].size() - 1) {
                row_state += " ";
            }
        }
        SQUID_LOG_DEBUG(MATRIX_TAG, "Row %zu: [%s]", row, row_state.c_str());
    }
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
    
    SQUID_LOG_INFO(KEYMAP_TAG, "Keymap initialized with %zu layers", keymap.size());
    for (size_t layer = 0; layer < keymap.size(); ++layer) {
        SQUID_LOG_DEBUG(KEYMAP_TAG, "Layer %zu has %zu keys", layer, keymap[layer].size());
    }
}

void SQUIDKEYMAP::handleKeyEvent(size_t row, size_t col, bool pressed) {
    if (row < _keymap.size() && col < _keymap[row].size()) {
        const auto& key_entry = _keymap[row][col];
        
        SQUID_LOG_DEBUG(KEYMAP_TAG, "Key event: row=%zu, col=%zu, type=%d, pressed=%s", 
                     row, col, static_cast<int>(key_entry.type), pressed ? "true" : "false");
        
        if (pressed && _press_callback) {
            _press_callback(key_entry);
        } else if (!pressed && _release_callback) {
            _release_callback(key_entry);
        }
    } else {
        SQUID_LOG_WARN(KEYMAP_TAG, "Invalid key position: row=%zu, col=%zu", row, col);
    }
}

KeymapEntry SQUIDKEYMAP::getKeyAt(size_t row, size_t col) const {
    if (row < _keymap.size() && col < _keymap[row].size()) {
        return _keymap[row][col];
    }
    return KeymapEntry(NKROKey{0}); // Return null key
}

size_t SQUIDKEYMAP::getLayerCount() const {
    return _keymap.size();
}

size_t SQUIDKEYMAP::getKeyCount(size_t layer) const {
    if (layer < _keymap.size()) {
        return _keymap[layer].size();
    }
    return 0;
}
