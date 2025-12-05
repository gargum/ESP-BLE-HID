/**
 * @file Matrix.cpp
 * @brief Implementation of the key matrix functionality
 */

#include "Matrix.h"

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
    
    // Extract all unique pins
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
    
    unifiedPinMode(current_to_pin, OUTPUT);
    unifiedDigitalWrite(current_to_pin, LOW);
    delayMicroseconds(3);
    
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
