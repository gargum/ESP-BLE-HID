#include "GeminiPR.h"
#include "NimBLEDevice.h"

static const char* STENO_TAG = "BLESTENO";

BLESTENO::BLESTENO() 
    : serialService(nullptr), serialInput(nullptr), serialOutput(nullptr), 
      serialOutputDescriptor(nullptr), serialConnected(false) {
    memset(&_geminiReport, 0, sizeof(_geminiReport));
}

BLESTENO::~BLESTENO() {
    // Clean up SPP resources
    if (serialOutputDescriptor) {
        delete serialOutputDescriptor;
        serialOutputDescriptor = nullptr;
    }
    // Note: serialService, serialInput, serialOutput are managed by BLE stack
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
    
    Serial.printf("[%s] SPP service started\n", STENO_TAG);
}

size_t BLESTENO::press(int32_t stenoKey) {
  // Set the appropriate bit in the correct byte position
  switch (stenoKey) {
    // Byte 0
    case GEMINI_FN:   _geminiReport.byte0 |= 0x01; break;
    case GEMINI_NUM1: _geminiReport.byte0 |= 0x02; break;
    
    // Byte 1  
    case GEMINI_S1:   _geminiReport.byte1 |= 0x01; break;
    case GEMINI_S2:   _geminiReport.byte1 |= 0x02; break;
    case GEMINI_T:    _geminiReport.byte1 |= 0x04; break;
    case GEMINI_K:    _geminiReport.byte1 |= 0x08; break;
    case GEMINI_P:    _geminiReport.byte1 |= 0x10; break;
    case GEMINI_W:    _geminiReport.byte1 |= 0x20; break;
    case GEMINI_H:    _geminiReport.byte1 |= 0x40; break;
    
    // Byte 2
    case GEMINI_R:    _geminiReport.byte2 |= 0x01; break;
    case GEMINI_A:    _geminiReport.byte2 |= 0x02; break;
    case GEMINI_O:    _geminiReport.byte2 |= 0x04; break;
    case GEMINI_STAR1:_geminiReport.byte2 |= 0x08; break;
    case GEMINI_STAR2:_geminiReport.byte2 |= 0x10; break;
    case GEMINI_RES1: _geminiReport.byte2 |= 0x20; break;
    case GEMINI_RES2: _geminiReport.byte2 |= 0x40; break;
    
    // Byte 3
    case GEMINI_PWR:  _geminiReport.byte3 |= 0x01; break;
    case GEMINI_STAR3:_geminiReport.byte3 |= 0x02; break;
    case GEMINI_STAR4:_geminiReport.byte3 |= 0x04; break;
    case GEMINI_E:    _geminiReport.byte3 |= 0x08; break;
    case GEMINI_U:    _geminiReport.byte3 |= 0x10; break;
    case GEMINI_F:    _geminiReport.byte3 |= 0x20; break;
    case GEMINI_R2:   _geminiReport.byte3 |= 0x40; break;
    
    // Byte 4
    case GEMINI_P2:   _geminiReport.byte4 |= 0x01; break;
    case GEMINI_B:    _geminiReport.byte4 |= 0x02; break;
    case GEMINI_L:    _geminiReport.byte4 |= 0x04; break;
    case GEMINI_G:    _geminiReport.byte4 |= 0x08; break;
    case GEMINI_T2:   _geminiReport.byte4 |= 0x10; break;
    case GEMINI_S:    _geminiReport.byte4 |= 0x20; break;
    case GEMINI_D:    _geminiReport.byte4 |= 0x40; break;
    
    // Byte 5
    case GEMINI_NUM7: _geminiReport.byte5 |= 0x01; break;
    case GEMINI_NUM8: _geminiReport.byte5 |= 0x02; break;
    case GEMINI_NUM9: _geminiReport.byte5 |= 0x04; break;
    case GEMINI_NUM10:_geminiReport.byte5 |= 0x08; break;
    case GEMINI_NUM11:_geminiReport.byte5 |= 0x10; break;
    case GEMINI_NUM12:_geminiReport.byte5 |= 0x20; break;
    case GEMINI_Z:    _geminiReport.byte5 |= 0x40; break;
  }
  
  sendGeminiPRReport();
  return 1;
}

size_t BLESTENO::release(int32_t stenoKey) {
  // Clear the appropriate bit
  switch (stenoKey) {
    // Byte 0
    case GEMINI_FN:   _geminiReport.byte0 &= ~0x01; break;
    case GEMINI_NUM1: _geminiReport.byte0 &= ~0x02; break;
    
    // Byte 1  
    case GEMINI_S1:   _geminiReport.byte1 &= ~0x01; break;
    case GEMINI_S2:   _geminiReport.byte1 &= ~0x02; break;
    case GEMINI_T:    _geminiReport.byte1 &= ~0x04; break;
    case GEMINI_K:    _geminiReport.byte1 &= ~0x08; break;
    case GEMINI_P:    _geminiReport.byte1 &= ~0x10; break;
    case GEMINI_W:    _geminiReport.byte1 &= ~0x20; break;
    case GEMINI_H:    _geminiReport.byte1 &= ~0x40; break;
    
    // Byte 2
    case GEMINI_R:    _geminiReport.byte2 &= ~0x01; break;
    case GEMINI_A:    _geminiReport.byte2 &= ~0x02; break;
    case GEMINI_O:    _geminiReport.byte2 &= ~0x04; break;
    case GEMINI_STAR1:_geminiReport.byte2 &= ~0x08; break;
    case GEMINI_STAR2:_geminiReport.byte2 &= ~0x10; break;
    case GEMINI_RES1: _geminiReport.byte2 &= ~0x20; break;
    case GEMINI_RES2: _geminiReport.byte2 &= ~0x40; break;
    
    // Byte 3
    case GEMINI_PWR:  _geminiReport.byte3 &= ~0x01; break;
    case GEMINI_STAR3:_geminiReport.byte3 &= ~0x02; break;
    case GEMINI_STAR4:_geminiReport.byte3 &= ~0x04; break;
    case GEMINI_E:    _geminiReport.byte3 &= ~0x08; break;
    case GEMINI_U:    _geminiReport.byte3 &= ~0x10; break;
    case GEMINI_F:    _geminiReport.byte3 &= ~0x20; break;
    case GEMINI_R2:   _geminiReport.byte3 &= ~0x40; break;
    
    // Byte 4
    case GEMINI_P2:   _geminiReport.byte4 &= ~0x01; break;
    case GEMINI_B:    _geminiReport.byte4 &= ~0x02; break;
    case GEMINI_L:    _geminiReport.byte4 &= ~0x04; break;
    case GEMINI_G:    _geminiReport.byte4 &= ~0x08; break;
    case GEMINI_T2:   _geminiReport.byte4 &= ~0x10; break;
    case GEMINI_S:    _geminiReport.byte4 &= ~0x20; break;
    case GEMINI_D:    _geminiReport.byte4 &= ~0x40; break;
    
    // Byte 5
    case GEMINI_NUM7: _geminiReport.byte5 &= ~0x01; break;
    case GEMINI_NUM8: _geminiReport.byte5 &= ~0x02; break;
    case GEMINI_NUM9: _geminiReport.byte5 &= ~0x04; break;
    case GEMINI_NUM10:_geminiReport.byte5 &= ~0x08; break;
    case GEMINI_NUM11:_geminiReport.byte5 &= ~0x10; break;
    case GEMINI_NUM12:_geminiReport.byte5 &= ~0x20; break;
    case GEMINI_Z:    _geminiReport.byte5 &= ~0x40; break;
  }
  
  sendGeminiPRReport();
  return 1;
}

void BLESTENO::releaseAll() {
    memset(&_geminiReport, 0, sizeof(_geminiReport));
    sendGeminiPRReport();
}

void BLESTENO::geminiStroke(const int32_t* keys, size_t count) {
  releaseAll();
  for (size_t i = 0; i < count; i++) {
    press(keys[i]);
  }
  sendGeminiPRReport();
}

uint8_t BLESTENO::stenoCharToKey(char c) {
  switch (toupper(c)) {
    case 'Q': return GEMINI_S1;
    case 'A': return GEMINI_S2;
    case 'W': return GEMINI_T;
    case 'S': return GEMINI_K;
    case 'E': return GEMINI_P;
    case 'D': return GEMINI_W;
    case 'R': return GEMINI_H;
    case 'F': return GEMINI_R;
    case 'C': return GEMINI_A;
    case 'V': return GEMINI_O;
    case 'T': return GEMINI_STAR1;
    case 'G': return GEMINI_STAR2;
    case 'Y': return GEMINI_STAR3;
    case 'H': return GEMINI_STAR4;
    case ',': return GEMINI_E;
    case 'M': return GEMINI_U;
    case 'U': return GEMINI_F;
    case 'J': return GEMINI_R2;
    case 'I': return GEMINI_P2;
    case 'K': return GEMINI_B;
    case 'O': return GEMINI_L;
    case 'L': return GEMINI_G;
    case 'P': return GEMINI_T2;
    case 'B': return GEMINI_S;
    case 'X': return GEMINI_D;
    case 'Z': return GEMINI_Z;
    case '1': return GEMINI_NUM1;
    case '2': return GEMINI_NUM7;
    case '3': return GEMINI_NUM8;
    case '4': return GEMINI_NUM9;
    case '5': return GEMINI_NUM10;
    case '6': return GEMINI_NUM11;
    case '7': return GEMINI_NUM12;
    case '8': return GEMINI_PWR;
    case '9': return GEMINI_RES1;
    case '0': return GEMINI_RES2;
    case 'N': return GEMINI_FN;
    default: return 0;
  }
}

void BLESTENO::sendGeminiPRReport() {
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
        Serial.printf("[%s] Sending GeminiPR SPP: %02X %02X %02X %02X %02X %02X\n", STENO_TAG,
                     serialData[0], serialData[1], serialData[2],
                     serialData[3], serialData[4], serialData[5]);
    } else {
        Serial.printf("[%s] GeminiPR stroke ready but SPP not connected: %02X %02X %02X %02X %02X %02X\n", STENO_TAG,
                     _geminiReport.byte0, _geminiReport.byte1, _geminiReport.byte2,
                     _geminiReport.byte3, _geminiReport.byte4, _geminiReport.byte5);
    }
    
    delay(_delay_ms);
}

void BLESTENO::sendSerialData(const uint8_t* data, size_t length) {
    if (serialConnected && serialInput) {
        serialInput->setValue(data, length);
        serialInput->notify();
    }
}

void BLESTENO::setSerialConnected(bool connected) {
    serialConnected = connected;
}

bool BLESTENO::isSerialConnected() {
    // If serial is neither on nor connected, then just return false
    if (!serialService || !serialInput) return false;
    
    // Fall back to the connection flag if the above all fails
    return serialConnected;
}
