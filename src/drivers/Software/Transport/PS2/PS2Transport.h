/**
 * @file PS2Transport.h
 * @brief PS/2 transport implementation
 */

#ifndef PS2TRANSPORT_H
#define PS2TRANSPORT_H

#include "../Transport.h"

struct PS2Scancode {
    uint8_t makeCode;        // Make code (key press)
    uint8_t breakCode;       // Break code (key release)  
    bool isExtended;         // Requires E0 prefix
    uint8_t modifierMask;    // Modifier bitmask for special handling
};

enum PS2ModifierMasks {
    MOD_LEFT_CTRL   = 0x01,
    MOD_LEFT_SHIFT  = 0x02,
    MOD_LEFT_ALT    = 0x04,
    MOD_LEFT_GUI    = 0x08,
    MOD_RIGHT_CTRL  = 0x10,
    MOD_RIGHT_SHIFT = 0x20,
    MOD_RIGHT_ALT   = 0x40,
    MOD_RIGHT_GUI   = 0x80
};

class PS2Transport : public Transport {
public:
    enum class DeviceType {
        PS2_KEYBOARD,
        PS2_MOUSE,
        PS2_COMBO  // Keyboard + Mouse
    };

private:
    TransportCallbacks* callbacks;
    std::string deviceName;
    std::string deviceManufacturer;
    bool initialized;
    bool connected;
    DeviceType deviceType;
    
    // PS/2 pins
    int clockPin;
    int dataPin;
    
    // PS/2 protocol state
    bool inhibitCommunication;
    uint8_t lastCommand;
    uint8_t responseBuffer[16];
    uint8_t responseIndex;
    
    // Keyboard state
    uint8_t keyboardLEDs;
    bool keyboardEnabled;
    
    // Mouse state  
    uint8_t mouseSampleRate;
    uint8_t mouseResolution;
    bool mouseScaling;
    bool mouseEnabled;
    
    // HID report buffers
    uint8_t keyboardReport[8];
    uint8_t mouseReport[4];
    
    // Device info
    uint16_t vid;
    uint16_t pid;
    uint16_t version;
    uint8_t batteryLevel;
    uint16_t appearance;
    
    // HID descriptor
    const uint8_t* reportMap;
    size_t reportMapLength;
    
    // PS/2 protocol methods
    void ps2Write(uint8_t data);
    uint8_t ps2Read();
    bool ps2WaitForAck();
    void ps2Reset();
    void ps2KeyboardInit();
    void ps2MouseInit();
    void processPS2Command(uint8_t command);
    
    // HID to PS/2 conversion
    void sendKeyboardReportPS2(const uint8_t* hidReport);
    void sendMouseReportPS2(const uint8_t* hidReport);
    
    // Interrupt service routines
    static void ps2ClockISR();
    static void ps2DataISR();
    
    void sendExtendedKey(uint8_t mainCode, bool isPress) {
        ps2Write(0xE0);
        if (!isPress) {
            ps2Write(0xF0); // Break code
        }
        ps2Write(mainCode);
    }
    
    bool isModifierKey(uint8_t hidCode) {
        return (hidCode >= 0xE0 && hidCode <= 0xE7);
    }
    
public:
    PS2Transport(DeviceType type = DeviceType::PS2_KEYBOARD, int clkPin = 3, int dataPin = 4);
    ~PS2Transport();
    
    // Transport interface implementation
    bool begin() override;
    void end() override;
    void update() override;
    
    bool isConnected() override;
    bool connect() override;
    void disconnect() override;
    
    bool sendData(const uint8_t* data, size_t length) override;
    bool sendReport(uint8_t reportId, const uint8_t* data, size_t length) override;
    
    void setDeviceInfo(const char* name, const char* manufacturer, uint16_t vid, uint16_t pid, uint16_t version) override;
    void setBatteryLevel(uint8_t level) override;
    void setAppearance(uint16_t appearance) override;
    
    void setCallbacks(TransportCallbacks* callbacks) override;
    
    void setReportMap(const uint8_t* descriptor, size_t length) override;
    
    bool supportsHID() override { return true; }
    
    // PS/2 specific methods
    void setPins(int clockPin, int dataPin);
    void setDeviceType(DeviceType type);
    void handlePS2Communication();
    
    // LED control for keyboard
    void setLEDs(bool scrollLock, bool numLock, bool capsLock);
    
    // Mouse configuration
    void setMouseSampleRate(uint8_t rate);
    void setMouseResolution(uint8_t resolution);
};

#endif
