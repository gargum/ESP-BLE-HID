/**
 * @file PS2Transport.cpp
 * @brief PS/2 transport implementation
 */

#include "PS2Transport.h"

// PS/2 command codes
namespace PS2Commands {
    constexpr uint8_t PS2_KEYBOARD_RESET = 0xFF;
    constexpr uint8_t PS2_KEYBOARD_DISABLE = 0xF5;
    constexpr uint8_t PS2_KEYBOARD_ENABLE = 0xF4;
    constexpr uint8_t PS2_KEYBOARD_ECHO = 0xEE;
    constexpr uint8_t PS2_KEYBOARD_SET_LEDS = 0xED;
    constexpr uint8_t PS2_KEYBOARD_SET_SCANCODE = 0xF0;
    
    constexpr uint8_t PS2_MOUSE_RESET = 0xFF;
    constexpr uint8_t PS2_MOUSE_DISABLE = 0xF5;
    constexpr uint8_t PS2_MOUSE_ENABLE = 0xF4;
    constexpr uint8_t PS2_MOUSE_ECHO = 0xEE;
    constexpr uint8_t PS2_MOUSE_SET_SAMPLE_RATE = 0xF3;
    constexpr uint8_t PS2_MOUSE_SET_RESOLUTION = 0xE8;
    constexpr uint8_t PS2_MOUSE_STATUS_REQUEST = 0xE9;
    constexpr uint8_t PS2_MOUSE_SET_SCALING = 0xE6;
}

// PS/2 responses
namespace PS2Responses {
    constexpr uint8_t SELF_TEST_PASSED = 0xAA;
    constexpr uint8_t ECHO_RESPONSE = 0xEE;
    constexpr uint8_t ACK = 0xFA;
    constexpr uint8_t SELF_TEST_FAILED = 0xFC;
    constexpr uint8_t RESEND = 0xFE;
    constexpr uint8_t ERROR = 0xFD;
}

// Scancode translation table
static const PS2Scancode HID_TO_PS2_SCANCODES[128] = {
    // 0x00-0x07: Reserved and errors
    {0x00, 0x00, false, 0}, // Reserved
    {0x00, 0x00, false, 0}, // Keyboard ErrorRollOver
    {0x00, 0x00, false, 0}, // Keyboard POSTFail
    {0x00, 0x00, false, 0}, // Keyboard ErrorUndefined
    
    // 0x04-0x1D: Alphabet keys (a-z)
    {0x1E, 0x9E, false, 0}, // Keyboard a and A
    {0x30, 0xB0, false, 0}, // Keyboard b and B
    {0x2E, 0xAE, false, 0}, // Keyboard c and C
    {0x20, 0xA0, false, 0}, // Keyboard d and D
    {0x12, 0x92, false, 0}, // Keyboard e and E
    {0x21, 0xA1, false, 0}, // Keyboard f and F
    {0x22, 0xA2, false, 0}, // Keyboard g and G
    {0x23, 0xA3, false, 0}, // Keyboard h and H
    {0x17, 0x97, false, 0}, // Keyboard i and I
    {0x24, 0xA4, false, 0}, // Keyboard j and J
    {0x25, 0xA5, false, 0}, // Keyboard k and K
    {0x26, 0xA6, false, 0}, // Keyboard l and L
    {0x32, 0xB2, false, 0}, // Keyboard m and M
    {0x31, 0xB1, false, 0}, // Keyboard n and N
    {0x18, 0x98, false, 0}, // Keyboard o and O
    {0x19, 0x99, false, 0}, // Keyboard p and P
    {0x10, 0x90, false, 0}, // Keyboard q and Q
    {0x13, 0x93, false, 0}, // Keyboard r and R
    {0x1F, 0x9F, false, 0}, // Keyboard s and S
    {0x14, 0x94, false, 0}, // Keyboard t and T
    {0x16, 0x96, false, 0}, // Keyboard u and U
    {0x2F, 0xAF, false, 0}, // Keyboard v and V
    {0x11, 0x91, false, 0}, // Keyboard w and W
    {0x2D, 0xAD, false, 0}, // Keyboard x and X
    {0x15, 0x95, false, 0}, // Keyboard y and Y
    {0x2C, 0xAC, false, 0}, // Keyboard z and Z
    
    // 0x1E-0x27: Number keys (1-0)
    {0x02, 0x82, false, 0}, // Keyboard 1 and !
    {0x03, 0x83, false, 0}, // Keyboard 2 and @
    {0x04, 0x84, false, 0}, // Keyboard 3 and #
    {0x05, 0x85, false, 0}, // Keyboard 4 and $
    {0x06, 0x86, false, 0}, // Keyboard 5 and %
    {0x07, 0x87, false, 0}, // Keyboard 6 and ^
    {0x08, 0x88, false, 0}, // Keyboard 7 and &
    {0x09, 0x89, false, 0}, // Keyboard 8 and *
    {0x0A, 0x8A, false, 0}, // Keyboard 9 and (
    {0x0B, 0x8B, false, 0}, // Keyboard 0 and )
    
    // 0x28-0x2F: Special characters
    {0x1C, 0x9C, false, 0}, // Keyboard Return (ENTER)
    {0x01, 0x81, false, 0}, // Keyboard ESCAPE
    {0x0E, 0x8E, false, 0}, // Keyboard DELETE (Backspace)
    {0x0F, 0x8F, false, 0}, // Keyboard Tab
    {0x39, 0xB9, false, 0}, // Keyboard Spacebar
    {0x0C, 0x8C, false, 0}, // Keyboard - and _
    {0x0D, 0x8D, false, 0}, // Keyboard = and +
    {0x1A, 0x9A, false, 0}, // Keyboard [ and {
    
    // 0x30-0x37: More special characters
    {0x1B, 0x9B, false, 0}, // Keyboard ] and }
    {0x2B, 0xAB, false, 0}, // Keyboard \ and |
    {0x00, 0x00, false, 0}, // Keyboard Non-US # and ~
    {0x27, 0xA7, false, 0}, // Keyboard ; and :
    {0x28, 0xA8, false, 0}, // Keyboard ' and "
    {0x29, 0xA9, false, 0}, // Keyboard Grave Accent and Tilde
    {0x33, 0xB3, false, 0}, // Keyboard , and <
    {0x34, 0xB4, false, 0}, // Keyboard . and >
    
    // 0x38-0x3F: Slash, Caps Lock, F1-F4
    {0x35, 0xB5, false, 0}, // Keyboard / and ?
    {0x3A, 0xBA, false, 0}, // Keyboard Caps Lock
    {0x3B, 0xBB, false, 0}, // Keyboard F1
    {0x3C, 0xBC, false, 0}, // Keyboard F2
    {0x3D, 0xBD, false, 0}, // Keyboard F3
    {0x3E, 0xBE, false, 0}, // Keyboard F4
    {0x3F, 0xBF, false, 0}, // Keyboard F5
    {0x40, 0xC0, false, 0}, // Keyboard F6
    
    // 0x40-0x47: F7-F12, PrintScreen, Scroll Lock
    {0x41, 0xC1, false, 0}, // Keyboard F7
    {0x42, 0xC2, false, 0}, // Keyboard F8
    {0x43, 0xC3, false, 0}, // Keyboard F9
    {0x44, 0xC4, false, 0}, // Keyboard F10
    {0x57, 0xD7, false, 0}, // Keyboard F11
    {0x58, 0xD8, false, 0}, // Keyboard F12
    {0x37, 0xB7, true,  0}, // Keyboard PrintScreen (extended)
    {0x46, 0xC6, false, 0}, // Keyboard Scroll Lock
    
    // 0x48-0x4F: Pause, Insert, Home, PageUp, Delete, End, PageDown, RightArrow
    {0x45, 0xC5, false, 0}, // Pause/Break (special - no break code)
    {0x52, 0xD2, true,  0}, // Keyboard Insert (extended)
    {0x47, 0xC7, true,  0}, // Keyboard Home (extended)
    {0x49, 0xC9, true,  0}, // Keyboard PageUp (extended)
    {0x53, 0xD3, true,  0}, // Keyboard Delete Forward (extended)
    {0x4F, 0xCF, true,  0}, // Keyboard End (extended)
    {0x51, 0xD1, true,  0}, // Keyboard PageDown (extended)
    {0x4D, 0xCD, true,  0}, // Keyboard RightArrow (extended)
    
    // 0x50-0x57: LeftArrow, DownArrow, UpArrow, NumLock, Keypad /
    {0x4B, 0xCB, true,  0}, // Keyboard LeftArrow (extended)
    {0x50, 0xD0, true,  0}, // Keyboard DownArrow (extended)
    {0x48, 0xC8, true,  0}, // Keyboard UpArrow (extended)
    {0x45, 0xC5, false, 0}, // Keypad Num Lock and Clear
    {0x35, 0xB5, true,  0}, // Keypad / (extended)
    {0x37, 0xB7, false, 0}, // Keypad *
    {0x4A, 0xCA, false, 0}, // Keypad -
    {0x4E, 0xCE, false, 0}, // Keypad +
    
    // 0x58-0x5F: Keypad Enter, Keypad 1-9
    {0x1C, 0x9C, true,  0}, // Keypad ENTER (extended)
    {0x4F, 0xCF, false, 0}, // Keypad 1 and End
    {0x50, 0xD0, false, 0}, // Keypad 2 and Down Arrow
    {0x51, 0xD1, false, 0}, // Keypad 3 and PageDn
    {0x4B, 0xCB, false, 0}, // Keypad 4 and Left Arrow
    {0x4C, 0xCC, false, 0}, // Keypad 5
    {0x4D, 0xCD, false, 0}, // Keypad 6 and Right Arrow
    {0x47, 0xC7, false, 0}, // Keypad 7 and Home
    
    // 0x60-0x67: Keypad 8-0, Keypad ., Non-US "\"
    {0x48, 0xC8, false, 0}, // Keypad 8 and Up Arrow
    {0x49, 0xC9, false, 0}, // Keypad 9 and PageUp
    {0x52, 0xD2, false, 0}, // Keypad 0 and Insert
    {0x53, 0xD3, false, 0}, // Keypad . and Delete
    {0x56, 0xD6, false, 0}, // Keyboard Non-US \ and |
    {0x00, 0x00, false, 0}, // Keyboard Application
    {0x00, 0x00, false, 0}, // Keyboard Power
    {0x00, 0x00, false, 0}, // Keypad =
    
    // 0x68-0x6F: F13-F24 (not typically in PS/2 so set to 0 so as to not break anything)
    {0x00, 0x00, false, 0}, // Keyboard F13
    {0x00, 0x00, false, 0}, // Keyboard F14
    {0x00, 0x00, false, 0}, // Keyboard F15
    {0x00, 0x00, false, 0}, // Keyboard F16
    {0x00, 0x00, false, 0}, // Keyboard F17
    {0x00, 0x00, false, 0}, // Keyboard F18
    {0x00, 0x00, false, 0}, // Keyboard F19
    {0x00, 0x00, false, 0}, // Keyboard F20
    
    // 0x70-0x77: F21-F24 and Execute/Help/Menu
    {0x00, 0x00, false, 0}, // Keyboard F21
    {0x00, 0x00, false, 0}, // Keyboard F22
    {0x00, 0x00, false, 0}, // Keyboard F23
    {0x00, 0x00, false, 0}, // Keyboard F24
    {0x00, 0x00, false, 0}, // Keyboard Execute
    {0x00, 0x00, false, 0}, // Keyboard Help
    {0x00, 0x00, false, 0}, // Keyboard Menu
    {0x00, 0x00, false, 0}, // Keyboard Select
    
    // 0x78-0x7F: Media and System Control Keys
    {0x00, 0x00, false, 0}, // Keyboard Stop
    {0x00, 0x00, false, 0}, // Keyboard Again
    {0x00, 0x00, false, 0}, // Keyboard Undo
    {0x00, 0x00, false, 0}, // Keyboard Cut
    {0x00, 0x00, false, 0}, // Keyboard Copy
    {0x00, 0x00, false, 0}, // Keyboard Paste
    {0x00, 0x00, false, 0}, // Keyboard Find
    {0x00, 0x00, false, 0}  // Keyboard Mute
};

// Special modifier key mappings
static const PS2Scancode MODIFIER_KEYS[] = {
    {0x14, 0x94, false, MOD_LEFT_CTRL},    // Left Control
    {0x12, 0x92, false, MOD_LEFT_SHIFT},   // Left Shift
    {0x11, 0x91, false, MOD_LEFT_ALT},     // Left Alt
    {0x1F, 0x9F, true,  MOD_LEFT_GUI},     // Left GUI (Windows)
    {0x14, 0x94, true,  MOD_RIGHT_CTRL},   // Right Control
    {0x59, 0xD9, false, MOD_RIGHT_SHIFT},  // Right Shift
    {0x11, 0x91, true,  MOD_RIGHT_ALT},    // Right Alt (AltGr)
    {0x27, 0xA7, true,  MOD_RIGHT_GUI},    // Right GUI
};

// Static members for ISR
static volatile PS2Transport* activePS2Instance = nullptr;
static volatile uint8_t ps2RxBuffer[16];
static volatile uint8_t ps2RxIndex = 0;
static volatile bool ps2Receiving = false;
static volatile uint8_t ps2CurrentBit = 0;
static volatile uint8_t ps2CurrentByte = 0;
static volatile uint32_t ps2LastClockTime = 0;

PS2Transport::PS2Transport(DeviceType type, int clkPin, int dataPin)
    : callbacks(nullptr)
    , deviceName("PS2 Device")
    , deviceManufacturer("SquidHID")
    , initialized(false)
    , connected(false)
    , deviceType(type)
    , clockPin(clkPin)
    , dataPin(dataPin)
    , inhibitCommunication(false)
    , lastCommand(0)
    , responseIndex(0)
    , keyboardLEDs(0)
    , keyboardEnabled(false)
    , mouseSampleRate(100)
    , mouseResolution(4)
    , mouseScaling(false)
    , mouseEnabled(false)
    , vid(0x046D)
    , pid(0xC52B)
    , version(0x0310)
    , batteryLevel(100)
    , appearance(0x03C4) // HID keyboard appearance
    , reportMap(nullptr)
    , reportMapLength(0)
{
    memset(keyboardReport, 0, sizeof(keyboardReport));
    memset(mouseReport, 0, sizeof(mouseReport));
}

PS2Transport::~PS2Transport() {
    end();
}

bool PS2Transport::begin() {
    if (initialized) {
        end();
    }
    
    // Initialize GPIO pins
    pinMode(clockPin, INPUT_PULLUP);
    pinMode(dataPin, INPUT_PULLUP);
    
    // Attach interrupts
    activePS2Instance = this;
    attachInterrupt(digitalPinToInterrupt(clockPin), ps2ClockISR, FALLING);
    
    // Reset devices
    ps2Reset();
    
    if (deviceType == DeviceType::PS2_KEYBOARD || deviceType == DeviceType::PS2_COMBO) {
        ps2KeyboardInit();
    }
    
    if (deviceType == DeviceType::PS2_MOUSE || deviceType == DeviceType::PS2_COMBO) {
        ps2MouseInit();
    }
    
    initialized = true;
    connected = true; // PS/2 is always connected once initialized
    
    SQUID_LOG_INFO(PS2_TAG, "PS/2 transport initialized - Type: %d, CLK: %d, DATA: %d", 
                  static_cast<int>(deviceType), clockPin, dataPin);
    
    if (callbacks) {
        callbacks->onConnect();
    }
    
    return true;
}

void PS2Transport::end() {
    if (!initialized) return;
    
    // Detach interrupts
    detachInterrupt(digitalPinToInterrupt(clockPin));
    activePS2Instance = nullptr;
    
    // Set pins to input
    pinMode(clockPin, INPUT);
    pinMode(dataPin, INPUT);
    
    initialized = false;
    connected = false;
    
    if (callbacks) {
        callbacks->onDisconnect();
    }
    
    SQUID_LOG_INFO(PS2_TAG, "PS/2 transport deinitialized");
}

void PS2Transport::update() {
    // Process any received PS/2 commands
    if (ps2RxIndex > 0) {
        uint8_t command = ps2Read();
        processPS2Command(command);
    }
}

bool PS2Transport::isConnected() {
    return connected && initialized;
}

bool PS2Transport::connect() {
    // For PS/2, connection is always "active" once initialized
    if (!initialized) {
        return begin();
    }
    return true;
}

void PS2Transport::disconnect() {
    // PS/2 can't really disconnect as far as I can tell, so I'm just gonna disable
    if (connected && callbacks) {
        callbacks->onDisconnect();
    }
    connected = false;
}

bool PS2Transport::sendData(const uint8_t* data, size_t length) {
    // Generic data send - route to appropriate handler based on content
    if (length >= 2 && data[0] == 0x01) { // Keyboard report ID
        return sendReport(0x01, data + 1, length - 1);
    } else if (length >= 2 && data[0] == 0x04) { // Mouse report ID
        return sendReport(0x04, data + 1, length - 1);
    }
    
    SQUID_LOG_WARN(PS2_TAG, "Unsupported generic data format");
    return false;
}

bool PS2Transport::sendReport(uint8_t reportId, const uint8_t* data, size_t length) {
    if (!isConnected()) {
        SQUID_LOG_WARN(PS2_TAG, "Cannot send HID report - not connected");
        return false;
    }
    
    switch (reportId) {
        case 0x01: // Keyboard report
            if (deviceType == DeviceType::PS2_KEYBOARD || deviceType == DeviceType::PS2_COMBO) {
                sendKeyboardReportPS2(data);
                return true;
            }
            break;
            
        case 0x04: // Mouse report  
            if (deviceType == DeviceType::PS2_MOUSE || deviceType == DeviceType::PS2_COMBO) {
                sendMouseReportPS2(data);
                return true;
            }
            break;
            
        default:
            SQUID_LOG_WARN(PS2_TAG, "Unsupported HID report ID: 0x%02X", reportId);
            break;
    }
    
    return false;
}

void PS2Transport::sendKeyboardReportPS2(const uint8_t* hidReport) {
    if (!keyboardEnabled) return;
    
    static uint8_t lastModifiers = 0;
    static uint8_t lastKeys[6] = {0};
    
    uint8_t currentModifiers = hidReport[0];
    const uint8_t* currentKeys = &hidReport[2]; // Skip report ID and reserved byte
    
    // Handle modifier key changes
    for (int i = 0; i < 8; i++) {
        bool wasPressed = lastModifiers & (1 << i);
        bool isPressed = currentModifiers & (1 << i);
        
        if (wasPressed != isPressed) {
            const PS2Scancode& scancode = MODIFIER_KEYS[i];
            if (scancode.isExtended) {
                ps2Write(0xE0);
            }
            if (!isPressed) {
                ps2Write(0xF0); // Break code for release
            }
            ps2Write(scancode.makeCode);
            delayMicroseconds(200); // Small delay between key events
        }
    }
    
    // Handle regular key releases (keys that were pressed but now aren't)
    for (int i = 0; i < 6; i++) {
        if (lastKeys[i] != 0 && lastKeys[i] < 128) {
            bool stillPressed = false;
            for (int j = 0; j < 6; j++) {
                if (currentKeys[j] == lastKeys[i]) {
                    stillPressed = true;
                    break;
                }
            }
            
            if (!stillPressed) {
                const PS2Scancode& scancode = HID_TO_PS2_SCANCODES[lastKeys[i]];
                if (scancode.makeCode != 0) {
                    if (scancode.isExtended) {
                        ps2Write(0xE0);
                    }
                    ps2Write(0xF0); // Break code
                    ps2Write(scancode.makeCode);
                    delayMicroseconds(200);
                }
            }
        }
    }
    
    // Handle regular key presses (new keys that weren't pressed before)
    for (int i = 0; i < 6; i++) {
        if (currentKeys[i] != 0 && currentKeys[i] < 128) {
            bool wasPressed = false;
            for (int j = 0; j < 6; j++) {
                if (lastKeys[j] == currentKeys[i]) {
                    wasPressed = true;
                    break;
                }
            }
            
            if (!wasPressed) {
                const PS2Scancode& scancode = HID_TO_PS2_SCANCODES[currentKeys[i]];
                if (scancode.makeCode != 0) {
                    if (scancode.isExtended) {
                        ps2Write(0xE0);
                    }
                    ps2Write(scancode.makeCode);
                    delayMicroseconds(200);
                }
            }
        }
    }
    
    // Update last state
    lastModifiers = currentModifiers;
    memcpy(lastKeys, currentKeys, 6);
}

void PS2Transport::sendMouseReportPS2(const uint8_t* hidReport) {
    if (!mouseEnabled) return;
    
    uint8_t buttons = hidReport[0];
    int8_t x = hidReport[1];
    int8_t y = hidReport[2];
    int8_t wheel = hidReport[3];
    
    // PS/2 mouse packet format: [YOVF XOVF YS XS 1 M R L] [X] [Y] [Z]
    uint8_t packet[4] = {0};
    
    // Button bits (Bit 0: Left, Bit 1: Right, Bit 2: Middle)
    packet[0] = 0x08; // Always 1 for first byte
    if (buttons & 0x01) packet[0] |= 0x01; // Left button
    if (buttons & 0x02) packet[0] |= 0x02; // Right button  
    if (buttons & 0x04) packet[0] |= 0x04; // Middle button
    
    // Handle overflow and sign bits
    if (x < -128) x = -128;
    if (x > 127) x = 127;
    if (y < -128) y = -128;
    if (y > 127) y = 127;
    
    if (x < 0) {
        packet[0] |= 0x10; // X sign bit
        x = 256 + x; // Convert to two's complement
    }
    
    if (y < 0) {
        packet[0] |= 0x20; // Y sign bit  
        y = 256 + y; // Convert to two's complement
    }
    
    if (x > 255) packet[0] |= 0x40; // X overflow
    if (y > 255) packet[0] |= 0x80; // Y overflow
    
    packet[1] = x & 0xFF;
    packet[2] = y & 0xFF;
    packet[3] = (wheel & 0x0F) | ((wheel < 0) ? 0xF0 : 0x00); // 4-bit signed wheel
    
    // Send mouse data
    for (int i = 0; i < 4; i++) {
        ps2Write(packet[i]);
    }
}

void PS2Transport::ps2Write(uint8_t data) {
    // Very basic error handling
    if (!initialized) return;
    
    // Implement PS/2 write protocol
    inhibitCommunication = true;
    
    // Wait for clock to be high, timeout after a while if unsuccessful
    uint32_t timeout = micros() + 10000; // 10ms timeout
    while (digitalRead(clockPin) == LOW) {
        if (micros() > timeout) {
            SQUID_LOG_WARN(PS2_TAG, "PS/2 clock stuck low during write");
            inhibitCommunication = false;
            return;
        }
        delayMicroseconds(10);
    }
    
    // Set data pin as output
    pinMode(dataPin, OUTPUT);
    digitalWrite(dataPin, LOW);
    delayMicroseconds(10);
    
    // Pull clock low to start transmission
    pinMode(clockPin, OUTPUT);
    digitalWrite(clockPin, LOW);
    delayMicroseconds(100);
    
    // Release clock
    pinMode(clockPin, INPUT_PULLUP);
    
    // Send data bits with parity
    uint8_t parity = 1; // Odd parity
    for (int i = 0; i < 8; i++) {
        bool bit = (data >> i) & 0x01;
        digitalWrite(dataPin, bit ? HIGH : LOW);
        parity ^= bit;
        delayMicroseconds(50);
        
        // Wait for clock pulse
        while (digitalRead(clockPin) == HIGH);
        while (digitalRead(clockPin) == LOW);
    }
    
    // Send parity bit
    digitalWrite(dataPin, parity ? HIGH : LOW);
    delayMicroseconds(50);
    while (digitalRead(clockPin) == HIGH);
    while (digitalRead(clockPin) == LOW);
    
    // Send stop bit
    digitalWrite(dataPin, HIGH);
    delayMicroseconds(50);
    while (digitalRead(clockPin) == HIGH);
    while (digitalRead(clockPin) == LOW);
    
    // Wait for device to release data line
    pinMode(dataPin, INPUT_PULLUP);
    inhibitCommunication = false;
    
    // Wait for ACK, notify if nothing comes in
    ps2WaitForAck();
    if (!ps2WaitForAck()) {
        SQUID_LOG_WARN(PS2_TAG, "No ACK received for command 0x%02X", data);
    }
}

uint8_t PS2Transport::ps2Read() {
    // This would be called from ISR - simplified for example
    if (ps2RxIndex > 0) {
        uint8_t data = ps2RxBuffer[0];
        // Shift buffer
        for (int i = 1; i < ps2RxIndex; i++) {
            ps2RxBuffer[i-1] = ps2RxBuffer[i];
        }
        ps2RxIndex--;
        return data;
    }
    return 0;
}

bool PS2Transport::ps2WaitForAck() {
    uint32_t timeout = millis() + 1000;
    while (millis() < timeout) {
        if (ps2RxIndex > 0 && ps2RxBuffer[0] == PS2Responses::ACK) {
            ps2RxIndex = 0; // Consume ACK
            return true;
        }
        delayMicroseconds(100);
    }
    return false;
}

void PS2Transport::ps2Reset() {
    SQUID_LOG_DEBUG(PS2_TAG, "Performing PS/2 reset");
    
    // Reset keyboard
    if (deviceType == DeviceType::PS2_KEYBOARD || deviceType == DeviceType::PS2_COMBO) {
        ps2Write(PS2Commands::PS2_KEYBOARD_RESET);
        if (ps2WaitForAck()) {
            // Wait for self-test passed
            uint32_t timeout = millis() + 2000;
            while (millis() < timeout) {
                if (ps2RxIndex > 0 && ps2RxBuffer[0] == PS2Responses::SELF_TEST_PASSED) {
                    ps2RxIndex = 0;
                    keyboardEnabled = true;
                    SQUID_LOG_INFO(PS2_TAG, "Keyboard self-test passed");
                    break;
                }
                delayMicroseconds(100);
            }
        }
    }
    
    // Reset mouse
    if (deviceType == DeviceType::PS2_MOUSE || deviceType == DeviceType::PS2_COMBO) {
        ps2Write(PS2Commands::PS2_MOUSE_RESET);
        if (ps2WaitForAck()) {
            // Mouse reset sequence
            uint32_t timeout = millis() + 2000;
            while (millis() < timeout) {
                if (ps2RxIndex >= 3) {
                    if (ps2RxBuffer[0] == 0xAA && ps2RxBuffer[1] == 0x00) {
                        // Mouse reset successful
                        ps2RxIndex = 0;
                        mouseEnabled = true;
                        SQUID_LOG_INFO(PS2_TAG, "Mouse self-test passed");
                        break;
                    }
                }
                delayMicroseconds(100);
            }
        }
    }
}

void PS2Transport::ps2KeyboardInit() {
    if (!keyboardEnabled) {
        SQUID_LOG_WARN(PS2_TAG, "Keyboard not enabled, skipping initialization");
        return;
    }
    
        // Enable keyboard, retry a few times if unsuccessful then stop gracefully
    for (int attempt = 0; attempt < 3; attempt++) {
        ps2Write(PS2Commands::PS2_KEYBOARD_ENABLE);
        if (ps2WaitForAck()) {
            break;
        }
        delay(100);
    }
    
    // Set scancode set 2 (most common)
    ps2Write(PS2Commands::PS2_KEYBOARD_SET_SCANCODE);
    ps2WaitForAck();
    ps2Write(0x02); // Scancode set 2
    ps2WaitForAck();
    
    SQUID_LOG_DEBUG(PS2_TAG, "Keyboard initialized with scancode set 2");
}

void PS2Transport::ps2MouseInit() {
    if (!mouseEnabled) return;
    
    // Enable mouse
    ps2Write(PS2Commands::PS2_MOUSE_ENABLE);
    ps2WaitForAck();
    
    // Set sample rate
    ps2Write(PS2Commands::PS2_MOUSE_SET_SAMPLE_RATE);
    ps2WaitForAck();
    ps2Write(mouseSampleRate);
    ps2WaitForAck();
    
    // Set resolution
    ps2Write(PS2Commands::PS2_MOUSE_SET_RESOLUTION);
    ps2WaitForAck();
    ps2Write(mouseResolution);
    ps2WaitForAck();
    
    SQUID_LOG_DEBUG(PS2_TAG, "Mouse initialized - Rate: %d, Resolution: %d", 
                   mouseSampleRate, mouseResolution);
}

// ISR for PS/2 clock
void PS2Transport::ps2ClockISR() {
    if (!activePS2Instance || activePS2Instance->inhibitCommunication) return;
    
    uint32_t currentTime = micros();
    if (currentTime - ps2LastClockTime < 50) return; // Debounce
    
    ps2LastClockTime = currentTime;
    
    // Read data bit
    bool dataBit = digitalRead(activePS2Instance->dataPin);
    
    if (ps2CurrentBit == 0) {
        // Start bit should be 0
        if (!dataBit) {
            ps2Receiving = true;
            ps2CurrentByte = 0;
            ps2CurrentBit = 1;
        }
    } else if (ps2CurrentBit <= 8) {
        // Data bits
        ps2CurrentByte |= (dataBit << (ps2CurrentBit - 1));
        ps2CurrentBit++;
    } else if (ps2CurrentBit == 9) {
        // Parity bit (we ignore for now)
        ps2CurrentBit++;
    } else if (ps2CurrentBit == 10) {
        // Stop bit should be 1
        if (dataBit) {
            // Valid byte received
            if (ps2RxIndex < 16) {
                ps2RxBuffer[ps2RxIndex++] = ps2CurrentByte;
            }
        }
        ps2Receiving = false;
        ps2CurrentBit = 0;
    }
}

void PS2Transport::setDeviceInfo(const char* name, const char* manufacturer, 
                                uint16_t vid, uint16_t pid, uint16_t version) {
    this->deviceName = name;
    this->deviceManufacturer = manufacturer;
    this->vid = vid;
    this->pid = pid;
    this->version = version;
}

void PS2Transport::setBatteryLevel(uint8_t level) {
    this->batteryLevel = level;
    // PS/2 is wired, so I'm just gonna say battery level is always 100% for now until I figure out how to go about implementing this
}

void PS2Transport::setAppearance(uint16_t appearance) {
    this->appearance = appearance;
}

void PS2Transport::setCallbacks(TransportCallbacks* callbacks) {
    this->callbacks = callbacks;
}

void PS2Transport::setReportMap(const uint8_t* descriptor, size_t length) {
    this->reportMap = descriptor;
    this->reportMapLength = length;
    SQUID_LOG_DEBUG(PS2_TAG, "HID report map set (length: %zu) - not used by PS/2", length);
}

// PS/2 specific methods
void PS2Transport::setPins(int clockPin, int dataPin) {
    if (initialized) {
        SQUID_LOG_WARN(PS2_TAG, "Cannot change pins after initialization");
        return;
    }
    this->clockPin = clockPin;
    this->dataPin = dataPin;
}

void PS2Transport::setDeviceType(DeviceType type) {
    if (initialized) {
        SQUID_LOG_WARN(PS2_TAG, "Cannot change device type after initialization");
        return;
    }
    deviceType = type;
}

void PS2Transport::handlePS2Communication() {
    // Process any received PS/2 commands
    if (ps2RxIndex > 0) {
        uint8_t command = ps2Read();
        processPS2Command(command);
    }
}

void PS2Transport::processPS2Command(uint8_t command) {
    SQUID_LOG_DEBUG(PS2_TAG, "Processing PS/2 command: 0x%02X", command);
    
    switch (command) {
        case PS2Commands::PS2_KEYBOARD_SET_LEDS:
            // Host wants to set LEDs
            if (ps2WaitForAck()) {
                uint8_t ledData = ps2Read();
                keyboardLEDs = ledData;
                ps2Write(PS2Responses::ACK);
                
                if (callbacks) {
                    // Convert to HID-style report (LED status)
                    uint8_t report[1] = {ledData};
                    callbacks->onDataReceived(report, 1);
                }
            }
            break;
            
        case PS2Commands::PS2_KEYBOARD_ECHO:
            ps2Write(PS2Responses::ECHO_RESPONSE);
            break;
            
        case PS2Commands::PS2_KEYBOARD_ENABLE:
            mouseEnabled = true;
            keyboardEnabled = true;
            ps2Write(PS2Responses::ACK);
            break;
            
        case PS2Commands::PS2_KEYBOARD_DISABLE:
            keyboardEnabled = false;
            mouseEnabled = false;
            ps2Write(PS2Responses::ACK);
            break;
            
        default:
            // Unknown command - send ACK anyway because why not
            ps2Write(PS2Responses::ACK);
            SQUID_LOG_DEBUG(PS2_TAG, "Unknown PS/2 command: 0x%02X", command);
            break;
    }
}

void PS2Transport::setLEDs(bool scrollLock, bool numLock, bool capsLock) {
    if (!keyboardEnabled) return;
    
    uint8_t ledState = 0;
    if (scrollLock) ledState |= 0x01;
    if (numLock)    ledState |= 0x02;
    if (capsLock)   ledState |= 0x04;
    
    ps2Write(PS2Commands::PS2_KEYBOARD_SET_LEDS);
    if (ps2WaitForAck()) {
        ps2Write(ledState);
        ps2WaitForAck();
        keyboardLEDs = ledState;
    }
}

void PS2Transport::setMouseSampleRate(uint8_t rate) {
    if (!mouseEnabled) return;
    
    mouseSampleRate = rate;
    if (initialized) {
        ps2Write(PS2Commands::PS2_MOUSE_SET_SAMPLE_RATE);
        ps2WaitForAck();
        ps2Write(rate);
        ps2WaitForAck();
    }
}

void PS2Transport::setMouseResolution(uint8_t resolution) {
    if (!mouseEnabled) return;
    
    mouseResolution = resolution;
    if (initialized) {
        ps2Write(PS2Commands::PS2_MOUSE_SET_RESOLUTION);
        ps2WaitForAck();
        ps2Write(resolution);
        ps2WaitForAck();
    }
}
