/**
 * @file Matrix.h
 * @brief Header file for the key matrix implementations
 */

#ifndef MATRIX_H
#define MATRIX_H

#include "drivers/Data.h"

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

#endif // MATRIX_H
