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

size_t BLESTENO::press(int32_t stenoKey) {
    BLE_LOG_DEBUG(STENO_TAG, "Stenotype key press: %ld", stenoKey);
    
    uint8_t previousByte = 0;
    uint8_t newByte = 0;
    
    // Set the appropriate bit in the correct byte position
    switch (stenoKey) {
        // Byte 0
        case GEMINI_FN:   previousByte = _geminiReport.byte0; _geminiReport.byte0 |= 0x01; newByte = _geminiReport.byte0; break;
        case GEMINI_NUM1: previousByte = _geminiReport.byte0; _geminiReport.byte0 |= 0x02; newByte = _geminiReport.byte0; break;
        
        // Byte 1  
        case GEMINI_S1:   previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x01; newByte = _geminiReport.byte1; break;
        case GEMINI_S2:   previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x02; newByte = _geminiReport.byte1; break;
        case GEMINI_T:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x04; newByte = _geminiReport.byte1; break;
        case GEMINI_K:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x08; newByte = _geminiReport.byte1; break;
        case GEMINI_P:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x10; newByte = _geminiReport.byte1; break;
        case GEMINI_W:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x20; newByte = _geminiReport.byte1; break;
        case GEMINI_H:    previousByte = _geminiReport.byte1; _geminiReport.byte1 |= 0x40; newByte = _geminiReport.byte1; break;
        
        // Byte 2
        case GEMINI_R:    previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x01; newByte = _geminiReport.byte2; break;
        case GEMINI_A:    previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x02; newByte = _geminiReport.byte2; break;
        case GEMINI_O:    previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x04; newByte = _geminiReport.byte2; break;
        case GEMINI_STAR1:previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x08; newByte = _geminiReport.byte2; break;
        case GEMINI_STAR2:previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x10; newByte = _geminiReport.byte2; break;
        case GEMINI_RES1: previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x20; newByte = _geminiReport.byte2; break;
        case GEMINI_RES2: previousByte = _geminiReport.byte2; _geminiReport.byte2 |= 0x40; newByte = _geminiReport.byte2; break;
        
        // Byte 3
        case GEMINI_PWR:  previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x01; newByte = _geminiReport.byte3; break;
        case GEMINI_STAR3:previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x02; newByte = _geminiReport.byte3; break;
        case GEMINI_STAR4:previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x04; newByte = _geminiReport.byte3; break;
        case GEMINI_E:    previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x08; newByte = _geminiReport.byte3; break;
        case GEMINI_U:    previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x10; newByte = _geminiReport.byte3; break;
        case GEMINI_F:    previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x20; newByte = _geminiReport.byte3; break;
        case GEMINI_R2:   previousByte = _geminiReport.byte3; _geminiReport.byte3 |= 0x40; newByte = _geminiReport.byte3; break;
        
        // Byte 4
        case GEMINI_P2:   previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x01; newByte = _geminiReport.byte4; break;
        case GEMINI_B:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x02; newByte = _geminiReport.byte4; break;
        case GEMINI_L:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x04; newByte = _geminiReport.byte4; break;
        case GEMINI_G:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x08; newByte = _geminiReport.byte4; break;
        case GEMINI_T2:   previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x10; newByte = _geminiReport.byte4; break;
        case GEMINI_S:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x20; newByte = _geminiReport.byte4; break;
        case GEMINI_D:    previousByte = _geminiReport.byte4; _geminiReport.byte4 |= 0x40; newByte = _geminiReport.byte4; break;
        
        // Byte 5
        case GEMINI_NUM7: previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x01; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM8: previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x02; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM9: previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x04; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM10:previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x08; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM11:previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x10; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM12:previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x20; newByte = _geminiReport.byte5; break;
        case GEMINI_Z:    previousByte = _geminiReport.byte5; _geminiReport.byte5 |= 0x40; newByte = _geminiReport.byte5; break;
        
        default:
            BLE_LOG_WARN(STENO_TAG, "Invalid stenotype key press attempt: %ld", stenoKey);
            return 0;
    }
    
    BLE_LOG_DEBUG(STENO_TAG, "Key press - Key: %ld, Byte changed: 0x%02X -> 0x%02X", stenoKey, previousByte, newByte);
    
    sendGeminiPRReport();
    return 1;
}

size_t BLESTENO::release(int32_t stenoKey) {
    BLE_LOG_DEBUG(STENO_TAG, "Stenotype key release: %ld", stenoKey);
    
    uint8_t previousByte = 0;
    uint8_t newByte = 0;
    
    // Clear the appropriate bit
    switch (stenoKey) {
        // Byte 0
        case GEMINI_FN:   previousByte = _geminiReport.byte0; _geminiReport.byte0 &= ~0x01; newByte = _geminiReport.byte0; break;
        case GEMINI_NUM1: previousByte = _geminiReport.byte0; _geminiReport.byte0 &= ~0x02; newByte = _geminiReport.byte0; break;
        
        // Byte 1  
        case GEMINI_S1:   previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x01; newByte = _geminiReport.byte1; break;
        case GEMINI_S2:   previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x02; newByte = _geminiReport.byte1; break;
        case GEMINI_T:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x04; newByte = _geminiReport.byte1; break;
        case GEMINI_K:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x08; newByte = _geminiReport.byte1; break;
        case GEMINI_P:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x10; newByte = _geminiReport.byte1; break;
        case GEMINI_W:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x20; newByte = _geminiReport.byte1; break;
        case GEMINI_H:    previousByte = _geminiReport.byte1; _geminiReport.byte1 &= ~0x40; newByte = _geminiReport.byte1; break;
        
        // Byte 2
        case GEMINI_R:    previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x01; newByte = _geminiReport.byte2; break;
        case GEMINI_A:    previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x02; newByte = _geminiReport.byte2; break;
        case GEMINI_O:    previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x04; newByte = _geminiReport.byte2; break;
        case GEMINI_STAR1:previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x08; newByte = _geminiReport.byte2; break;
        case GEMINI_STAR2:previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x10; newByte = _geminiReport.byte2; break;
        case GEMINI_RES1: previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x20; newByte = _geminiReport.byte2; break;
        case GEMINI_RES2: previousByte = _geminiReport.byte2; _geminiReport.byte2 &= ~0x40; newByte = _geminiReport.byte2; break;
        
        // Byte 3
        case GEMINI_PWR:  previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x01; newByte = _geminiReport.byte3; break;
        case GEMINI_STAR3:previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x02; newByte = _geminiReport.byte3; break;
        case GEMINI_STAR4:previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x04; newByte = _geminiReport.byte3; break;
        case GEMINI_E:    previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x08; newByte = _geminiReport.byte3; break;
        case GEMINI_U:    previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x10; newByte = _geminiReport.byte3; break;
        case GEMINI_F:    previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x20; newByte = _geminiReport.byte3; break;
        case GEMINI_R2:   previousByte = _geminiReport.byte3; _geminiReport.byte3 &= ~0x40; newByte = _geminiReport.byte3; break;
        
        // Byte 4
        case GEMINI_P2:   previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x01; newByte = _geminiReport.byte4; break;
        case GEMINI_B:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x02; newByte = _geminiReport.byte4; break;
        case GEMINI_L:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x04; newByte = _geminiReport.byte4; break;
        case GEMINI_G:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x08; newByte = _geminiReport.byte4; break;
        case GEMINI_T2:   previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x10; newByte = _geminiReport.byte4; break;
        case GEMINI_S:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x20; newByte = _geminiReport.byte4; break;
        case GEMINI_D:    previousByte = _geminiReport.byte4; _geminiReport.byte4 &= ~0x40; newByte = _geminiReport.byte4; break;
        
        // Byte 5
        case GEMINI_NUM7: previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x01; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM8: previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x02; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM9: previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x04; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM10:previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x08; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM11:previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x10; newByte = _geminiReport.byte5; break;
        case GEMINI_NUM12:previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x20; newByte = _geminiReport.byte5; break;
        case GEMINI_Z:    previousByte = _geminiReport.byte5; _geminiReport.byte5 &= ~0x40; newByte = _geminiReport.byte5; break;
        
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

void BLESTENO::geminiStroke(const int32_t* keys, size_t count) {
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
        case 'Q': key = GEMINI_S1; break;
        case 'A': key = GEMINI_S2; break;
        case 'W': key = GEMINI_T; break;
        case 'S': key = GEMINI_K; break;
        case 'E': key = GEMINI_P; break;
        case 'D': key = GEMINI_W; break;
        case 'R': key = GEMINI_H; break;
        case 'F': key = GEMINI_R; break;
        case 'C': key = GEMINI_A; break;
        case 'V': key = GEMINI_O; break;
        case 'T': key = GEMINI_STAR1; break;
        case 'G': key = GEMINI_STAR2; break;
        case 'Y': key = GEMINI_STAR3; break;
        case 'H': key = GEMINI_STAR4; break;
        case ',': key = GEMINI_E; break;
        case 'M': key = GEMINI_U; break;
        case 'U': key = GEMINI_F; break;
        case 'J': key = GEMINI_R2; break;
        case 'I': key = GEMINI_P2; break;
        case 'K': key = GEMINI_B; break;
        case 'O': key = GEMINI_L; break;
        case 'L': key = GEMINI_G; break;
        case 'P': key = GEMINI_T2; break;
        case 'B': key = GEMINI_S; break;
        case 'X': key = GEMINI_D; break;
        case 'Z': key = GEMINI_Z; break;
        case '1': key = GEMINI_NUM1; break;
        case '2': key = GEMINI_NUM7; break;
        case '3': key = GEMINI_NUM8; break;
        case '4': key = GEMINI_NUM9; break;
        case '5': key = GEMINI_NUM10; break;
        case '6': key = GEMINI_NUM11; break;
        case '7': key = GEMINI_NUM12; break;
        case '8': key = GEMINI_PWR; break;
        case '9': key = GEMINI_RES1; break;
        case '0': key = GEMINI_RES2; break;
        case 'N': key = GEMINI_FN; break;
        default: key = 0; break;
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
