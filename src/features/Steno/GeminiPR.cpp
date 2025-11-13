/**
 * @file GeminiPR.cpp
 * @brief Implementation of the GeminiPR stenotype
 */

#include "GeminiPR.h"
#include "NimBLEDevice.h"
#include "../Log/Log.h"

static const char* STENO_TAG = "BLESTENO";

BLESTENO::BLESTENO() 
    : serialService(nullptr), serialInput(nullptr), serialOutput(nullptr), 
      serialOutputDescriptor(nullptr), serialConnected(false) {
    memset(&_geminiReport, 0, sizeof(_geminiReport));
    
    BLE_LOG_DEBUG(STENO_TAG, "Stenotype instance created");
}

BLESTENO::~BLESTENO() {
    // Clean up SPP resources
    if (serialOutputDescriptor) {
        delete serialOutputDescriptor;
        serialOutputDescriptor = nullptr;
        BLE_LOG_DEBUG(STENO_TAG, "Serial output descriptor cleaned up");
    }
    // Note: serialService, serialInput, serialOutput are managed by BLE stack
    BLE_LOG_DEBUG(STENO_TAG, "Stenotype instance destroyed");
}

void BLESTENO::begin(NimBLEService* service, NimBLECharacteristic* input, NimBLECharacteristic* output, uint32_t delay_ms) {
    serialService = service;
    serialInput = input;
    serialOutput = output;
    _delay_ms = delay_ms;
    
    // Initialize the report
    memset(&_geminiReport, 0, sizeof(_geminiReport));
    
    // Add CCCD descriptor for TX characteristic
    serialOutputDescriptor = new BLE2904();
    serialInput->addDescriptor(serialOutputDescriptor);
    
    // Start the serial service
    serialService->start();
    
    BLE_LOG_DEBUG(STENO_TAG, "SPP service initialized with delay: %lu ms", delay_ms);
    BLE_LOG_INFO(STENO_TAG, "SPP service started");
}

size_t BLESTENO::press(StenoKey stenoKey) {
    BLE_LOG_DEBUG(STENO_TAG, "Stenotype key press: %ld", stenoKey);
    
    uint8_t previousByte = 0;
    uint8_t newByte = 0;
    
    // Set the appropriate bit in the correct byte position
    switch (stenoKey) {
        // Byte 0
        case ST_FN:    previousByte = _geminiReport.byte0; _geminiReport.byte0 |= 0x01; newByte = _geminiReport.byte0; break;
        case ST_1:     previousByte = _geminiReport.byte0; _geminiReport.byte0 |= 0x02; newByte = _geminiReport.byte0; break;
        
        // Byte 1  
        case ST_S1:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x01; newByte = _geminiReport.byte1; break;
        case ST_S2:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x02; newByte = _geminiReport.byte1; break;
        case ST_T:     previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x04; newByte = _geminiReport.byte1; break;
        case ST_K:     previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x08; newByte = _geminiReport.byte1; break;
        case ST_P:     previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x10; newByte = _geminiReport.byte1; break;
        case ST_W:     previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x20; newByte = _geminiReport.byte1; break;
        case ST_H:     previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x40; newByte = _geminiReport.byte1; break;
        
        // Byte 2
        case ST_R:     previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x01; newByte = _geminiReport.byte2; break;
        case ST_A:     previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x02; newByte = _geminiReport.byte2; break;
        case ST_O:     previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x04; newByte = _geminiReport.byte2; break;
        case ST_ST1:   previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x08; newByte = _geminiReport.byte2; break;
        case ST_ST2:   previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x10; newByte = _geminiReport.byte2; break;
        case ST_RS1:   previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x20; newByte = _geminiReport.byte2; break;
        case ST_RS2:   previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x40; newByte = _geminiReport.byte2; break;
        
        // Byte 3
        case ST_PWR:   previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x01; newByte = _geminiReport.byte3; break;
        case ST_ST3:   previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x02; newByte = _geminiReport.byte3; break;
        case ST_ST4:   previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x04; newByte = _geminiReport.byte3; break;
        case ST_E:     previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x08; newByte = _geminiReport.byte3; break;
        case ST_U:     previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x10; newByte = _geminiReport.byte3; break;
        case ST_F:     previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x20; newByte = _geminiReport.byte3; break;
        case ST_R2:    previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x40; newByte = _geminiReport.byte3; break;
        
        // Byte 4
        case ST_P2:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x01; newByte = _geminiReport.byte4; break;
        case ST_B:     previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x02; newByte = _geminiReport.byte4; break;
        case ST_L:     previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x04; newByte = _geminiReport.byte4; break;
        case ST_G:     previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x08; newByte = _geminiReport.byte4; break;
        case ST_T2:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x10; newByte = _geminiReport.byte4; break;
        case ST_S:     previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x20; newByte = _geminiReport.byte4; break;
        case ST_D:     previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x40; newByte = _geminiReport.byte4; break;
        
        // Byte 5
        case ST_7:     previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x01; newByte = _geminiReport.byte5; break;
        case ST_8:     previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x02; newByte = _geminiReport.byte5; break;
        case ST_9:     previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x04; newByte = _geminiReport.byte5; break;
        case ST_10:    previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x08; newByte = _geminiReport.byte5; break;
        case ST_11:    previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x10; newByte = _geminiReport.byte5; break;
        case ST_12:    previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x20; newByte = _geminiReport.byte5; break;
        case ST_Z:     previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x40; newByte = _geminiReport.byte5; break;
        
        default:
            BLE_LOG_WARN(STENO_TAG, "Invalid stenotype key press attempt: %ld", stenoKey);
            return 0;
    }
    
    BLE_LOG_DEBUG(STENO_TAG, "Key press - Key: %ld, Byte changed: 0x%02X -> 0x%02X", stenoKey, previousByte, newByte);
    
    sendGeminiPRReport();
    return 1;
}

size_t BLESTENO::release(StenoKey stenoKey) {
    BLE_LOG_DEBUG(STENO_TAG, "Stenotype key release: %ld", stenoKey);
    
    uint8_t previousByte = 0;
    uint8_t newByte = 0;
    
    // Clear the appropriate bit
    switch (stenoKey) {
        // Byte 0
        case ST_FN:    previousByte = _geminiReport.byte0; _geminiReport.byte0 &= ~0x01; newByte = _geminiReport.byte0; break;
        case ST_1:     previousByte = _geminiReport.byte0; _geminiReport.byte0 &= ~0x02; newByte = _geminiReport.byte0; break;
        
        // Byte 1  
        case ST_S1:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x01; newByte = _geminiReport.byte1; break;
        case ST_S2:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x02; newByte = _geminiReport.byte1; break;
        case ST_T:     previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x04; newByte = _geminiReport.byte1; break;
        case ST_K:     previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x08; newByte = _geminiReport.byte1; break;
        case ST_P:     previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x10; newByte = _geminiReport.byte1; break;
        case ST_W:     previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x20; newByte = _geminiReport.byte1; break;
        case ST_H:     previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x40; newByte = _geminiReport.byte1; break;
        
        // Byte 2
        case ST_R:     previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x01; newByte = _geminiReport.byte2; break;
        case ST_A:     previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x02; newByte = _geminiReport.byte2; break;
        case ST_O:     previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x04; newByte = _geminiReport.byte2; break;
        case ST_ST1:   previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x08; newByte = _geminiReport.byte2; break;
        case ST_ST2:   previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x10; newByte = _geminiReport.byte2; break;
        case ST_RS1:   previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x20; newByte = _geminiReport.byte2; break;
        case ST_RS2:   previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x40; newByte = _geminiReport.byte2; break;
        
        // Byte 3
        case ST_PWR:   previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x01; newByte = _geminiReport.byte3; break;
        case ST_ST3:   previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x02; newByte = _geminiReport.byte3; break;
        case ST_ST4:   previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x04; newByte = _geminiReport.byte3; break;
        case ST_E:     previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x08; newByte = _geminiReport.byte3; break;
        case ST_U:     previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x10; newByte = _geminiReport.byte3; break;
        case ST_F:     previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x20; newByte = _geminiReport.byte3; break;
        case ST_R2:    previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x40; newByte = _geminiReport.byte3; break;
        
        // Byte 4
        case ST_P2:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x01; newByte = _geminiReport.byte4; break;
        case ST_B:     previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x02; newByte = _geminiReport.byte4; break;
        case ST_L:     previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x04; newByte = _geminiReport.byte4; break;
        case ST_G:     previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x08; newByte = _geminiReport.byte4; break;
        case ST_T2:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x10; newByte = _geminiReport.byte4; break;
        case ST_S:     previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x20; newByte = _geminiReport.byte4; break;
        case ST_D:     previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x40; newByte = _geminiReport.byte4; break;
        
        // Byte 5
        case ST_7:     previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x01; newByte = _geminiReport.byte5; break;
        case ST_8:     previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x02; newByte = _geminiReport.byte5; break;
        case ST_9:     previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x04; newByte = _geminiReport.byte5; break;
        case ST_10:    previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x08; newByte = _geminiReport.byte5; break;
        case ST_11:    previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x10; newByte = _geminiReport.byte5; break;
        case ST_12:    previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x20; newByte = _geminiReport.byte5; break;
        case ST_Z:     previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x40; newByte = _geminiReport.byte5; break;
        
        default:
            BLE_LOG_WARN(STENO_TAG, "Invalid stenotype key release attempt: %ld", stenoKey);
            return 0;
    }
    
    BLE_LOG_DEBUG(STENO_TAG, "Key release - Key: %ld, Byte changed: 0x%02X -> 0x%02X", stenoKey, previousByte, newByte);
    
    sendGeminiPRReport();
    return 1;
}

void BLESTENO::releaseAll() {
    BLE_LOG_DEBUG(STENO_TAG, "Releasing all stenotype keys - Current state: %02X %02X %02X %02X %02X %02X",
                 _geminiReport.byte0, _geminiReport.byte1, _geminiReport.byte2,
                 _geminiReport.byte3, _geminiReport.byte4, _geminiReport.byte5);
    
    memset(&_geminiReport, 0, sizeof(_geminiReport));
    sendGeminiPRReport();
    
    BLE_LOG_DEBUG(STENO_TAG, "All stenotype keys released");
}

void BLESTENO::geminiStroke(const StenoKey* keys, size_t count) {
    BLE_LOG_DEBUG(STENO_TAG, "Executing Gemini stroke with %zu keys", count);
    
    releaseAll();
    for (size_t i = 0; i < count; i++) {
        press(keys[i]);
        BLE_LOG_DEBUG(STENO_TAG, "Stroke key %zu: %ld", i, keys[i]);
    }
    sendGeminiPRReport();
    
    BLE_LOG_DEBUG(STENO_TAG, "Gemini stroke completed");
}

uint8_t BLESTENO::stenoCharToKey(char c) {
    uint8_t key = 0;
    
    switch (toupper(c)) {
        case 'Q': key = ST_S1;  break;
        case 'A': key = ST_S2;  break;
        case 'W': key = ST_T;   break;
        case 'S': key = ST_K;   break;
        case 'E': key = ST_P;   break;
        case 'D': key = ST_W;   break;
        case 'R': key = ST_H;   break;
        case 'F': key = ST_R;   break;
        case 'C': key = ST_A;   break;
        case 'V': key = ST_O;   break;
        case 'T': key = ST_ST1; break;
        case 'G': key = ST_ST2; break;
        case 'Y': key = ST_ST3; break;
        case 'H': key = ST_ST4; break;
        case ',': key = ST_E;   break;
        case 'M': key = ST_U;   break;
        case 'U': key = ST_F;   break;
        case 'J': key = ST_R2;  break;
        case 'I': key = ST_P2;  break;
        case 'K': key = ST_B;   break;
        case 'O': key = ST_L;   break;
        case 'L': key = ST_G;   break;
        case 'P': key = ST_T2;  break;
        case 'B': key = ST_S;   break;
        case 'X': key = ST_D;   break;
        case 'Z': key = ST_Z;   break;
        case '1': key = ST_1;   break;
        case '2': key = ST_7;   break;
        case '3': key = ST_8;   break;
        case '4': key = ST_9;   break;
        case '5': key = ST_10;  break;
        case '6': key = ST_11;  break;
        case '7': key = ST_12;  break;
        case '8': key = ST_PWR; break;
        case '9': key = ST_RS1; break;
        case '0': key = ST_RS2; break;
        case 'N': key = ST_FN;  break;
        default:  key = 0;      break;
    }
    
    BLE_LOG_DEBUG(STENO_TAG, "Character to key conversion - Char: '%c', Key: %u", c, key);
    return key;
}

void BLESTENO::sendGeminiPRReport() {
    BLE_LOG_DEBUG(STENO_TAG, "Preparing GeminiPR report: %02X %02X %02X %02X %02X %02X",
                 _geminiReport.byte0, _geminiReport.byte1, _geminiReport.byte2,
                 _geminiReport.byte3, _geminiReport.byte4, _geminiReport.byte5);
    
    // ONLY send over SPP as 6-byte raw data (for Plover compatibility)
    if (isSerialConnected()) {
        uint8_t serialData[6] = {
            _geminiReport.byte0,
            _geminiReport.byte1, 
            _geminiReport.byte2,
            _geminiReport.byte3,
            _geminiReport.byte4,
            _geminiReport.byte5
        };
        
        sendSerialData(serialData, 6);
        BLE_LOG_DEBUG(STENO_TAG, "GeminiPR SPP data sent: %02X %02X %02X %02X %02X %02X",
                     serialData[0], serialData[1], serialData[2],
                     serialData[3], serialData[4], serialData[5]);
    } else {
        BLE_LOG_WARN(STENO_TAG, "GeminiPR stroke ready but SPP not connected: %02X %02X %02X %02X %02X %02X",
                     _geminiReport.byte0, _geminiReport.byte1, _geminiReport.byte2,
                     _geminiReport.byte3, _geminiReport.byte4, _geminiReport.byte5);
    }
    
    delay(_delay_ms);
}

void BLESTENO::sendSerialData(const uint8_t* data, size_t length) {
    if (serialConnected && serialInput) {
        serialInput->setValue(data, length);
        
        if (serialInput->notify()) {
            BLE_LOG_DEBUG(STENO_TAG, "Serial data sent successfully - Length: %zu", length);
        } else {
            BLE_LOG_WARN(STENO_TAG, "Failed to send serial data notification - Length: %zu", length);
        }
    } else {
        BLE_LOG_DEBUG(STENO_TAG, "Cannot send serial data - %s%s", 
                     !serialConnected ? "not connected" : "", 
                     !serialInput ? "no input characteristic" : "");
    }
}

void BLESTENO::setSerialConnected(bool connected) {
    bool previousState = serialConnected;
    serialConnected = connected;
    
    BLE_LOG_INFO(STENO_TAG, "Serial connection state changed: %s -> %s", 
                 previousState ? "connected" : "disconnected",
                 connected ? "connected" : "disconnected");
}

bool BLESTENO::isSerialConnected() {
    bool connected = false;
    
    // If serial is neither on nor connected, then just return false
    if (!serialService || !serialInput) {
        connected = false;
    } else {
        // Fall back to the connection flag if the above all fails
        connected = serialConnected;
    }
    
    BLE_LOG_DEBUG(STENO_TAG, "Serial connection check: %s", connected ? "connected" : "disconnected");
    return connected;
}
