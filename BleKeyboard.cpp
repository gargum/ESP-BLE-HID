#include "BleKeyboard.h"

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEUUID.h>
#include "HIDTypes.h"
static const char* LOG_TAG = "BleKeyboard";
bool getInitialized = false;

// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02
#define NKRO_KEYBOARD_ID 0x03
#define MOUSE_ID 0x04
#define DIGITIZER_ID 0x05
#define GAMEPAD_ID 0x06

void pollConnection(void * arg);

// Global instance tracking
static BleKeyboard* _activeBleKeyboardInstance = nullptr;

// Automatic update/polling function
void _bleKeyboardAutoUpdate() {
  static uint32_t lastUpdateTime = 0;
  uint32_t currentTime = millis();
  
  // Updating every 10ms currently
  if (currentTime - lastUpdateTime >= 10) {
    lastUpdateTime = currentTime;
    
    if (_activeBleKeyboardInstance) {
      _activeBleKeyboardInstance->_update();
    }
  }
}

// We're hooking into Arduino IDE's main loop here
__attribute__((weak)) void loop() {
  _bleKeyboardAutoUpdate();
  delay(1); // Small delay was added to prevent overwhelming the CPU
}

// This "HID Report Descriptor" tells your computer or phone what the ESP32 you've just connected is supposed to do
static const uint8_t _hidReportDescriptor[] = {
  // NKRO Report Descriptor (6KRO is emulated)
  USAGE_PAGE(1),      0x01,             // USAGE_PAGE (Generic Desktop)
  USAGE(1),           0x06,             // USAGE (Keyboard)
  COLLECTION(1),      0x01,             // COLLECTION (Application)
  REPORT_ID(1),       NKRO_KEYBOARD_ID, // REPORT_ID (3)
  USAGE_PAGE(1),      0x07,             // USAGE_PAGE (Key Codes)
  USAGE_MINIMUM(1),   0xE0,             // USAGE_MINIMUM (Keyboard LeftControl)
  USAGE_MAXIMUM(1),   0xE7,             // USAGE_MAXIMUM (Keyboard Right GUI)
  LOGICAL_MINIMUM(1), 0x00,             // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,             // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,             // REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,             // REPORT_COUNT (8)
  HIDINPUT(1),        0x02,             // INPUT (Data, Variable, Absolute)
  REPORT_COUNT(1),    0x01,             // REPORT_COUNT (1)
  REPORT_SIZE(1),     0x08,             // REPORT_SIZE (8)
  HIDINPUT(1),        0x01,             // INPUT (Constant)
  REPORT_COUNT(1),    0x05,             // REPORT_COUNT (5)
  REPORT_SIZE(1),     0x01,             // REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,             // USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,             // USAGE_MINIMUM (Num Lock)
  USAGE_MAXIMUM(1),   0x05,             // USAGE_MAXIMUM (Kana)
  HIDOUTPUT(1),       0x02,             // OUTPUT (Data, Variable, Absolute)
  REPORT_COUNT(1),    0x01,             // REPORT_COUNT (1)
  REPORT_SIZE(1),     0x03,             // REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,             // OUTPUT (Constant)
  USAGE_PAGE(1),      0x07,             // USAGE_PAGE (Key Codes)
  USAGE_MINIMUM(1),   0x00,             // USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x6F,             // USAGE_MAXIMUM (111) - Maximum key index
  LOGICAL_MINIMUM(1), 0x00,             // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,             // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,             // REPORT_SIZE (1)
  REPORT_COUNT(1),    0xFC,             // REPORT_COUNT (252) - 252 keys
  HIDINPUT(1),        0x02,             // INPUT (Data, Variable, Absolute)
  END_COLLECTION(0),                      // END_COLLECTION
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,             // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,             // USAGE (Consumer Control)
  COLLECTION(1),      0x01,             // COLLECTION (Application)
  REPORT_ID(1),       MEDIA_KEYS_ID,    // REPORT_ID (2)
  USAGE_PAGE(1),      0x0C,             // USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1), 0x00,             // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(2), 0xFF, 0x03,       // LOGICAL_MAXIMUM (0x03FF) - Maximum 16-bit usage code
  REPORT_SIZE(1),     0x01,             // REPORT_SIZE (1) - One bit, one code
  REPORT_COUNT(1),    0x1C,             // REPORT_COUNT (1C) - 28 codes, so 28 bits 
  // System Controls
  USAGE(2),           0x30, 0x01,       // USAGE (System Power - 0x0130)        [bit 1]
  USAGE(2),           0x34, 0x01,       // USAGE (System Sleep - 0x0134)        [bit 2]
  USAGE(2),           0x35, 0x01,       // USAGE (System Wake - 0x0135)         [bit 3] 
  // Transport Controls
  USAGE(1),           0xB5,             // USAGE (Scan Next Track - 0x00B5)     [bit 4]
  USAGE(1),           0xB6,             // USAGE (Scan Previous Track - 0x00B6) [bit 5]
  USAGE(1),           0xB7,             // USAGE (Stop - 0x00B7)                [bit 6]
  USAGE(1),           0xCD,             // USAGE (Play/Pause - 0x00CD)          [bit 7]
  USAGE(1),           0xB3,             // USAGE (Fast Forward - 0x00B3)        [bit 8]
  USAGE(1),           0xB4,             // USAGE (Rewind - 0x00B4)              [bit 9]
  USAGE(1),           0xB8,             // USAGE (Eject - 0x00B8)               [bit 10]
  // Volume Controls
  USAGE(1),           0xE2,             // USAGE (Mute - 0x00E2)                [bit 11]
  USAGE(1),           0xE9,             // USAGE (Volume Increment - 0x00E9)    [bit 12]
  USAGE(1),           0xEA,             // USAGE (Volume Decrement - 0x00EA)    [bit 13] 
  // Display Controls
  USAGE(1),           0x6F,             // USAGE (Brightness Up - 0x006F)       [bit 14]
  USAGE(1),           0x70,             // USAGE (Brightness Down - 0x0070)     [bit 15] 
  // Application Launch
  USAGE(2),           0x94, 0x01,       // USAGE (My Computer - 0x0194)         [bit 16]
  USAGE(2),           0x92, 0x01,       // USAGE (Calculator - 0x0192)          [bit 17]
  USAGE(2),           0x8A, 0x01,       // USAGE (Mail - 0x018A)                [bit 18]
  USAGE(2),           0x83, 0x01,       // USAGE (Media Selection - 0x0183)     [bit 19]
  USAGE(2),           0x86, 0x01,       // USAGE (Control Panel - 0x0186)       [bit 20]
  USAGE(2),           0x87, 0x01,       // USAGE (Launchpad - 0x0187)           [bit 21]
  // Browser Controls
  USAGE(2),           0x23, 0x02,       // USAGE (WWW Home - 0x0223)            [bit 22]
  USAGE(2),           0x2A, 0x02,       // USAGE (WWW favorites - 0x022A)       [bit 23]
  USAGE(2),           0x21, 0x02,       // USAGE (WWW search - 0x0221)          [bit 24]
  USAGE(2),           0x26, 0x02,       // USAGE (WWW stop - 0x0226)            [bit 25]
  USAGE(2),           0x24, 0x02,       // USAGE (WWW back - 0x0224)            [bit 26]
  USAGE(2),           0x25, 0x02,       // USAGE (WWW forward - 0x0225)         [bit 27]
  USAGE(2),           0x27, 0x02,       // USAGE (WWW refresh - 0x0227)         [bit 28]
  HIDINPUT(1),        0x02,             // INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                      // END_COLLECTION
  // ------------------------------------------------- Relative Pointer - Needed for Pointers on Android/iOS
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x02,            // USAGE (Mouse)
  COLLECTION(1),       0x01,            // COLLECTION (Application)
  REPORT_ID(1),        MOUSE_ID,        // REPORT_ID (4) - Mouse report
  USAGE(1),            0x01,            // USAGE (Pointer)
  COLLECTION(1),       0x00,            // COLLECTION (Physical)
  // Buttons (Left, Right, Middle, Back, Forward)
  USAGE_PAGE(1),       0x09,            // USAGE_PAGE (Button)
  USAGE_MINIMUM(1),    0x01,            // USAGE_MINIMUM (Button 1)
  USAGE_MAXIMUM(1),    0x05,            // USAGE_MAXIMUM (Button 5)
  LOGICAL_MINIMUM(1),  0x00,            // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01,            // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01,            // REPORT_SIZE (1)
  REPORT_COUNT(1),     0x05,            // REPORT_COUNT (5)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Padding
  REPORT_SIZE(1),      0x03,            // REPORT_SIZE (3)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x03,            // INPUT (Constant, Variable, Absolute)
  // X/Y position, Wheel
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x30,            // USAGE (X)
  USAGE(1),            0x31,            // USAGE (Y)
  USAGE(1),            0x38,            // USAGE (Wheel)
  LOGICAL_MINIMUM(1),  0x81,            // LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f,            // LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08,            // REPORT_SIZE (8)
  REPORT_COUNT(1),     0x03,            // REPORT_COUNT (3)
  HIDINPUT(1),         0x06,            // INPUT (Data, Variable, Relative)
  // Horizontal wheel
  USAGE_PAGE(1),       0x0c,            // USAGE PAGE (Consumer Devices)
  USAGE(2),      0x38, 0x02,            // USAGE (AC Pan)
  LOGICAL_MINIMUM(1),  0x81,            // LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f,            // LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08,            // REPORT_SIZE (8)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x06,            // INPUT (Data, Var, Rel)
  END_COLLECTION(0),                      // END_COLLECTION (Physical)
  END_COLLECTION(0),                      // END_COLLECTION (Application)
  // ------------------------------------------------- Absolute Pointer - Only works on desktop
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x02,            // USAGE (Mouse)
  COLLECTION(1),       0x01,            // COLLECTION (Application)
  REPORT_ID(1),        DIGITIZER_ID,    // REPORT_ID (5) - Absolute mouse report
  // Buttons (Left, Right, Middle, Back, Forward)
  USAGE_PAGE(1),       0x09,            // USAGE_PAGE (Button)
  USAGE_MINIMUM(1),    0x01,            // USAGE_MINIMUM (Button 1)
  USAGE_MAXIMUM(1),    0x05,            // USAGE_MAXIMUM (Button 5)
  LOGICAL_MINIMUM(1),  0x00,            // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01,            // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01,            // REPORT_SIZE (1)
  REPORT_COUNT(1),     0x05,            // REPORT_COUNT (5)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Padding
  REPORT_SIZE(1),      0x03,            // REPORT_SIZE (3)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x03,            // INPUT (Constant, Variable, Absolute)
  // Absolute X/Y position
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x30,            // USAGE (X)
  USAGE(1),            0x31,            // USAGE (Y)
  LOGICAL_MINIMUM(2),  0x00, 0x00,      // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(2),  0xFF, 0x7F,      // LOGICAL_MAXIMUM (32767)
  REPORT_SIZE(1),      0x10,            // REPORT_SIZE (16)
  REPORT_COUNT(1),     0x02,            // REPORT_COUNT (2)
  UNIT(1),             0x13,            // UNIT (Inch, EngLinear)
  UNIT_EXPONENT(1),    0x0D,            // UNIT_EXPONENT (-3)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Pressure
  USAGE_PAGE(1),       0xFF, 0x00,      // USAGE_PAGE (Vendor Defined 0xFF00)
  USAGE(1),            0x01,            // USAGE (Vendor Usage 1 - Pressure)
  LOGICAL_MINIMUM(2),  0x00, 0x00,      // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(2),  0xFF, 0x03,      // LOGICAL_MAXIMUM (1023)
  REPORT_SIZE(1),      0x10,            // REPORT_SIZE (16)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Tip switch
  USAGE_PAGE(1),       0xFF, 0x00,      // USAGE_PAGE (Vendor Defined 0xFF00)  
  USAGE(1),            0x02,            // USAGE (Vendor Usage 2 - Tip Switch)
  LOGICAL_MINIMUM(1),  0x00,            // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01,            // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01,            // REPORT_SIZE (1)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Padding for remaining 7 bits
  REPORT_SIZE(1),      0x07,            // REPORT_SIZE (7)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x03,            // INPUT (Constant, Variable, Absolute)
  // Wheel (relative) - kept for compatibility
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x38,            // USAGE (Wheel)
  LOGICAL_MINIMUM(1),  0x81,            // LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f,            // LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08,            // REPORT_SIZE (8)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x06,            // INPUT (Data, Variable, Relative)
  // Horizontal wheel (relative) - kept for compatibility
  USAGE_PAGE(1),       0x0c,            // USAGE PAGE (Consumer Devices)
  USAGE(2),      0x38, 0x02,            // USAGE (AC Pan)
  LOGICAL_MINIMUM(1),  0x81,            // LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f,            // LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08,            // REPORT_SIZE (8)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x06,            // INPUT (Data, Var, Rel)
  END_COLLECTION(0),                      // END_COLLECTION (Application)
  // ------------------------------------------------- Gamepad
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x05,            // USAGE (Game Pad)
  COLLECTION(1),       0x01,            // COLLECTION (Application)
  REPORT_ID(1),        GAMEPAD_ID,      // REPORT_ID (6) - Gamepad report
  // 64 buttons in two 32-bit fields
  USAGE_PAGE(1),       0x09,            // USAGE_PAGE (Button)
  LOGICAL_MINIMUM(1),  0x00,            // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01,            // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01,            // REPORT_SIZE (1)
  REPORT_COUNT(1),     0x40,            // REPORT_COUNT (64) - All 64 buttons
  USAGE_MINIMUM(1),    0x01,            // USAGE_MINIMUM (Button 1)
  USAGE_MAXIMUM(1),    0x40,            // USAGE_MAXIMUM (Button 64)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Left analogue stick (X/Y) - Fully working
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x01,            // USAGE (Pointer) - Important context
  COLLECTION(1),       0x00,            // COLLECTION (Physical)
  USAGE(1),            0x30,            // USAGE (X)
  USAGE(1),            0x31,            // USAGE (Y)
  LOGICAL_MINIMUM(2),  0x01, 0x80,      // LOGICAL_MINIMUM (-32767)
  LOGICAL_MAXIMUM(2),  0xFF, 0x7F,      // LOGICAL_MAXIMUM (32767)
  REPORT_SIZE(1),      0x10,            // REPORT_SIZE (16)
  REPORT_COUNT(1),     0x02,            // REPORT_COUNT (2)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  END_COLLECTION(0),                      // END_COLLECTION (Physical)
  // Right analogue stick (X/Y) - Fully working
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x01,            // USAGE (Pointer)
  COLLECTION(1),       0x00,            // COLLECTION (Physical)
  USAGE(1),            0x33,            // USAGE (Rx)
  USAGE(1),            0x34,            // USAGE (Ry)
  LOGICAL_MINIMUM(2),  0x01, 0x80,      // LOGICAL_MINIMUM (-32767) 
  LOGICAL_MAXIMUM(2),  0xFF, 0x7F,      // LOGICAL_MAXIMUM (32767)
  REPORT_SIZE(1),      0x10,            // REPORT_SIZE (16)
  REPORT_COUNT(1),     0x02,            // REPORT_COUNT (2)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  END_COLLECTION(0),                      // END_COLLECTION (Physical)
  // Analogue triggers - Fully working
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x32,            // USAGE (Z) - Left trigger
  USAGE(1),            0x35,            // USAGE (Rz) - Right trigger
  LOGICAL_MINIMUM(2),  0x00, 0x00,      // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(2),  0xFF, 0x7F,      // LOGICAL_MAXIMUM (32767)
  REPORT_SIZE(1),      0x10,            // REPORT_SIZE (16)
  REPORT_COUNT(1),     0x02,            // REPORT_COUNT (2)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Hat switch - Fully working
  USAGE_PAGE(1),       0x01,            // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x39,            // USAGE (Hat switch)
  LOGICAL_MINIMUM(1),  0x00,            // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x07,            // LOGICAL_MAXIMUM (7) - 8 positions
  PHYSICAL_MINIMUM(1), 0x00,            // PHYSICAL_MINIMUM (0)
  PHYSICAL_MAXIMUM(2), 0x3B, 0x01,      // PHYSICAL_MAXIMUM (315 degrees)
  UNIT(1),             0x14,            // UNIT (English Rotation, Degrees)
  REPORT_SIZE(1),      0x04,            // REPORT_SIZE (4) - 4 bits for 8 values
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x02,            // INPUT (Data, Variable, Absolute)
  // Padding for the remaining 4 bits
  REPORT_SIZE(1),      0x04,            // REPORT_SIZE (4)
  REPORT_COUNT(1),     0x01,            // REPORT_COUNT (1)
  HIDINPUT(1),         0x03,            // INPUT (Constant, Variable, Absolute)
  
  // Haptics - left and right motor vibration
  USAGE_PAGE(1),       0x0F,            // USAGE_PAGE (Physical Interface Device)
  LOGICAL_MINIMUM(1),  0x00,            // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0xFF,            // LOGICAL_MAXIMUM (255)
  REPORT_SIZE(1),      0x08,            // REPORT_SIZE (8)
  REPORT_COUNT(1),     0x02,            // REPORT_COUNT (2) - Left and right motor
  USAGE(1),            0x97,            // USAGE (Magnitude) - Left motor
  USAGE(1),            0x97,            // USAGE (Magnitude) - Right motor  
  HIDOUTPUT(1),        0x02,            // OUTPUT (Data, Variable, Absolute)
  
  END_COLLECTION(0),                      // END_COLLECTION (Application)
};
// This is a "constructor". It takes that class from the BleKeyboard.h file, and turns it into "objects" that can actually be used.
BleKeyboard::BleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) 
    , _mediaKeyBitmask(0) 
    , _useNKRO(true)
    , _mouseButtons(0)
    , _useAbsolute(false)
    , _onVibrateCallback(nullptr) 
    , lastPollTime(0) 
{
  // Initialize reports
  memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
  memset(&_mouseReport, 0, sizeof(_mouseReport));
  memset(&_absoluteReport, 0, sizeof(_absoluteReport));
  memset(&_gamepadReport, 0, sizeof(_gamepadReport));
  _gamepadReport.hat = HAT_CENTER; // Initialize hat to center position 
  _activeBleKeyboardInstance = this;
}
// This is a "destructor". It takes objects the contructor made, and destroys them whenever you tell it to. 
BleKeyboard::~BleKeyboard() { 
  // Unregister this instance
  if (_activeBleKeyboardInstance == this) {
    _activeBleKeyboardInstance = nullptr;
  }
}

void BleKeyboard::begin(void) {
  
    // Initialise BLE stack only once
    if (getInitialized) {
        Serial.printf("[%s] BLE already initialized, cleaning up first...\n", LOG_TAG);
        end();
        delay(100);
    } else { NimBLEDevice::init(deviceName.c_str()); }
    
    // Configure security if enabled
    if (isSecurityEnabled()) {
        // Enhanced security configuration for bonding
        NimBLEDevice::setSecurityAuth(true, true, true); // Bonding, MITM, Secure Connections
        NimBLEDevice::setSecurityPasskey(passkey);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
        
        // Configure bonding parameters
        if (bonding_enabled) {
            NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
            NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        }
        
        Serial.printf("[%s] Security configured with PIN: %06lu, Bonding: %s\n", 
                 LOG_TAG, passkey, bonding_enabled ? "enabled" : "disabled");
    } else {
        // For "Just Works" pairing, still enable bonding if requested
        if (bonding_enabled) {
            NimBLEDevice::setSecurityAuth(true, false, false); // Bonding only, no MITM
            NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC);
            NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC);
            Serial.printf("[%s] Just Works pairing with bonding enabled\n", LOG_TAG);
        } else {
            NimBLEDevice::setSecurityAuth(false, false, false);
            Serial.printf("[%s] Running without security (Just Works)\n", LOG_TAG);
        }
    }
    
    // Create server & install callbacks
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(this);

    // Create HID device object
    hid = new NimBLEHIDDevice(pServer);

    // Obtain report-characteristic pointers
    outputKeyboard = hid->getOutputReport(KEYBOARD_ID);
    inputMediaKeys = hid->getInputReport(MEDIA_KEYS_ID);
    inputNKRO      = hid->getInputReport(NKRO_KEYBOARD_ID);
    outputKeyboard->setCallbacks(this);

    // Manufacturer / PnP / HID-info
    hid->setManufacturer(std::string(deviceManufacturer.c_str()));
    hid->setHidInfo(0x00, 0x01);

    // Mouse / Digitizer / Gamepad reports
    inputMouse    = hid->getInputReport(0x04);
    inputAbsolute = hid->getInputReport(0x05);
    inputGamepad  = hid->getInputReport(0x06);
    if (inputGamepad) {
    inputGamepad->setCallbacks(this);
    }

    // Publish HID report map and start services
    hid->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->startServices();

    // Advertising setup
    advertising = pServer->getAdvertising();
    BLEAdvertisementData adv, scan;
    
    scan.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    scan.setName(deviceName.c_str());
    scan.setShortName(deviceName.substr(0, 8).c_str());
    scan.setAppearance(this->appearance);
    scan.addServiceUUID(hid->getHidService()->getUUID());
    scan.setManufacturerData((deviceManufacturer).c_str());

    adv.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    adv.setName(deviceName.c_str());
    adv.setShortName(deviceName.substr(0, 8).c_str());
    adv.setAppearance(this->appearance);
    adv.setCompleteServices(hid->getHidService()->getUUID());
    adv.addServiceUUID(hid->getHidService()->getUUID());
    adv.setManufacturerData((deviceManufacturer).c_str());

    advertising->setMinInterval(32);    // 20ms in 0.625ms units
    advertising->setMaxInterval(160);   // 100ms in 0.625ms units  
    advertising->setAdvertisementData(adv);
    advertising->setScanResponseData(scan);

    // Start advertising & finish
    onStarted(pServer);
    advertising->start();
    hid->setBatteryLevel(batteryLevel);
    
    lastPollTime = millis();

    Serial.printf("[%s] Advertising started!\n", LOG_TAG);
    Serial.printf("[%s] Device name: %s\n", LOG_TAG, deviceName.c_str());
    Serial.printf("[%s] Service UUID: %s\n", LOG_TAG, hid->getHidService()->getUUID().toString().c_str());
    Serial.printf("[%s] Using %s mode by default\n", LOG_TAG, _useNKRO ? "NKRO" : "6KRO");
    
    getInitialized = true;
}

void BleKeyboard::end(void) {
  if (hid != 0) {
    delete hid;
    hid = 0;
  }
  BLEDevice::deinit(true);
  getInitialized = false;
  Serial.printf("[%s] BLE Keyboard stopped\n", LOG_TAG);
}

void BleKeyboard::_update() {
  uint32_t currentTime = millis();
  
  // Handle millis() rollover
  if (currentTime < lastPollTime) {
    lastPollTime = currentTime;
    return;
  }
  
  if (currentTime - lastPollTime >= POLL_INTERVAL) {
    lastPollTime = currentTime;
    pollConnection(this);
  }
}

// Security callback - displays PIN to user
void BleKeyboard::securityCallback(uint32_t passkey) {
  Serial.printf("[%s] Pairing PIN: %06lu\n", LOG_TAG, passkey);
  // You could add display output here if you have an LCD/OLED
}

void BleKeyboard::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    Serial.printf("[%s] Authentication complete - encrypted: %s, authenticated: %s\n", LOG_TAG,
             desc->sec_state.encrypted ? "yes" : "no",
             desc->sec_state.authenticated ? "yes" : "no");
}

void BleKeyboard::setPIN(const char* pin) {
  if (pin == nullptr) {
    disableSecurity();
    return;
  }
  
  if (strlen(pin) == 6) {
    this->passkey = atoi(pin);
    Serial.printf("[%s] Security enabled with PIN: %s\n", LOG_TAG, pin);
  } else {
    Serial.printf("[%s] PIN must be 6 digits, security disabled\n", LOG_TAG);
    this->passkey = 0;
  }
}

void BleKeyboard::setPIN(uint32_t pin) {
  if (pin >= 1 && pin <= 999999) {  // 0 means no security
    this->passkey = pin;
    Serial.printf("[%s] Security enabled with PIN: %06lu\n", LOG_TAG, pin);
  } else {
    Serial.printf("[%s] PIN must be between 000001 and 999999, security disabled\n", LOG_TAG);
    this->passkey = 0;
  }
}

void BleKeyboard::disableSecurity(bool enable) {
  if (!enable) {
    this->passkey = 0;
    Serial.printf("[%s] Security disabled\n", LOG_TAG);
  } else {
    Serial.printf("[%s] Security remains enabled (call setPIN to enable)\n", LOG_TAG);
  }
}

bool BleKeyboard::isSecurityEnabled() const {
  return passkey != 0;
}

// Update the security callbacks to check passkey directly
uint32_t BleKeyboard::onPassKeyRequest() {
  Serial.printf("[%s] PassKeyRequest received\n", LOG_TAG);
  if (isSecurityEnabled()) {
    securityCallback(passkey);
    return passkey;
  }
  return 0; // No PIN = Just Works
}

bool BleKeyboard::onSecurityRequest() {
  Serial.printf("[%s] Security request received\n", LOG_TAG);
  return isSecurityEnabled(); // Only require auth if we have a PIN
}

void BleKeyboard::setAppearance(uint16_t newAppearance) {
  this->appearance = newAppearance;
  Serial.printf("[%s] Appearance set to: 0x%04X\n", LOG_TAG, newAppearance);
}

bool BleKeyboard::isConnected(void) {
    // Always check the actual BLE state - relying on cached flags kept breaking for some reason
    if (NimBLEDevice::getServer()) {
        int connectedClients = NimBLEDevice::getServer()->getConnectedCount();
        
        // Debug logging (every 10 seconds)
        static uint64_t lastLogTime = 0;
        uint64_t currentTime = esp_timer_get_time();
        
        if (currentTime - lastLogTime > 10000000) { // This is just 10 seconds in microseconds
            Serial.printf("[%s] BLE Status - Connected clients: %d, Advertising: %s\n", LOG_TAG,
                    connectedClients,
                    advertising ? (advertising->isAdvertising() ? "Yes" : "No") : "Null");
            lastLogTime = currentTime;
        }
        
        return (connectedClients > 0);
    }
    
    static uint64_t lastLogTime = 0;
    uint64_t currentTime = esp_timer_get_time();
    
    if (currentTime - lastLogTime > 10000000) {
        Serial.printf("[%s] BLE Status: No server instance available\n", LOG_TAG);
        lastLogTime = currentTime;
    }
    
    return false;
}

void BleKeyboard::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}

//must be called before begin in order to set the name
void BleKeyboard::setName(std::string deviceName) {
  this->deviceName = deviceName;
}

//must be called before begin in order to set the manufacturer
void BleKeyboard::setManufacturer(std::string deviceManufacturer) {
  this->deviceManufacturer = deviceManufacturer;
}

// Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
void BleKeyboard::setDelay(uint32_t ms) {
  delay(_delay_ms);
}

void BleKeyboard::set_vendor_id(uint16_t vid) { 
	this->vid = vid; 
}

void BleKeyboard::set_product_id(uint16_t pid) { 
	this->pid = pid; 
}

void BleKeyboard::set_version(uint16_t version) { 
	this->version = version; 
}

void BleKeyboard::sendReport() {
  if (this->isConnected()) {
    // Send the current media key bitmask
    this->inputMediaKeys->setValue((uint8_t*)&_mediaKeyBitmask, sizeof(uint32_t));
    this->inputMediaKeys->notify();
    delay(_delay_ms);
  }	
}

void BleKeyboard::sendNKROReport() {
  if (this->isConnected() && inputNKRO) {
    inputNKRO->setValue((uint8_t*)&_keyReportNKRO, sizeof(KeyReportNKRO));
    inputNKRO->notify();
    delay(_delay_ms);
  }
}

void BleKeyboard::updateNKROBitmask(uint8_t k, bool pressed)
{  
  if (k < NKRO_KEY_COUNT) {
    uint8_t bitmaskIndex = k / 8;
    uint8_t bitOffset = k % 8;
    
    if (pressed) {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] |= (1 << bitOffset);
    } else {
      _keyReportNKRO.keys_bitmask[bitmaskIndex] &= ~(1 << bitOffset);
    }
  }
}

// NKRO/6KRO mode switching functions
void BleKeyboard::useNKRO(bool state) {
  _useNKRO = state; // state = enabled, therefore _useNKRO = true/enabled
  Serial.printf("[%s] Switched to %s mode\n", LOG_TAG, _useNKRO ? "NKRO" : "6KRO");
}

void BleKeyboard::use6KRO(bool state) {
  _useNKRO = !state; // state = enabled, therefore _useNKRO = not true/enabled = false
  Serial.printf("[%s] Switched to %s mode\n", LOG_TAG, _useNKRO ? "NKRO" : "6KRO");
}

bool BleKeyboard::isNKROEnabled() {
  return _useNKRO;
}

// The ASCII map exists to facilitate the keyboard's write function
extern
const uint8_t _asciimap[128] PROGMEM;

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,	          // BS	Backspace
	0x2b,		  // TAB	Tab
	0x28,		  // LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US
	0x2c,		  //  ' '
	0x1e|SHIFT,	  // !
	0x34|SHIFT,	  // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    // {
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
};


uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BleKeyboard::press(uint8_t k) {
  // This function ONLY handles regular keycodes (uint8_t)
  if (k >= 136) { 
    k = k - 136;
  }
  
  if (k != 0) {
    // Check if we're already at 6 non-modifier keys
    if (!_useNKRO && countPressedKeys() >= 6) {
      setWriteError();                
      return 0;
    }
    
    // Update the bitmask - ONLY for regular keys
    updateNKROBitmask(k, true);
  }
  
  sendNKROReport();
  return 1;
}

size_t BleKeyboard::press(int16_t modifier) {
  // This function ONLY handles modifier keys (0x0100-0x8000)
  
  // Convert internal modifier code to HID modifier code
  uint8_t hidModifier = 0;
  if (modifier >= 0x0100 && modifier <= 0x8000 && ((modifier & (modifier - 1)) == 0)) {
    hidModifier = modifier >> 8;
  } else {
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers |= hidModifier;
  sendNKROReport();
  return 1;
}

size_t BleKeyboard::press(uint16_t mediaKey)
{
    addMediaKey(mediaKey);
    return 1;
}


void BleKeyboard::press(int8_t button) {
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        _gamepadReport.buttons[field] |= (1UL << bit);
    }
    else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatPress[directionIndex][currentHat];
    }
    sendGamepadReport();
}

void BleKeyboard::press(char b) {
  if (_useAbsolute) {
    // In absolute mode without coordinates, use current position
    press(_absoluteReport.x, _absoluteReport.y, b);
  } else {
    // Relative mode
    _mouseButtons |= b;
    move(0, 0, 0, 0);
  }
}

void BleKeyboard::press(uint16_t x, uint16_t y, char b) {
  // Auto-switch to absolute mode if coordinates are provided
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  _absoluteReport.buttons |= b;
  moveTo(x, y);
}

// This just sends a keyup event/unpresses a given key
size_t BleKeyboard::release(uint8_t k) {
  // This function ONLY handles regular keycodes
  if (k >= 136) {
    k = k - 136;
  }
  
  if (k != 0) {
    updateNKROBitmask(k, false);
  }
  
  sendNKROReport();
  return 1;
}

size_t BleKeyboard::release(int16_t modifier) {
  // This function ONLY handles modifier keys
  
  // Convert internal modifier code to HID modifier code
  uint8_t hidModifier = 0;
  if (modifier >= 0x0100 && modifier <= 0x8000 && ((modifier & (modifier - 1)) == 0)) {
    hidModifier = modifier >> 8;
  } else {
    return 0; // Invalid modifier
  }
  
  _keyReportNKRO.modifiers &= ~hidModifier;
  sendNKROReport();
  return 1;
}

size_t BleKeyboard::release(uint16_t mediaKey)
{
    removeMediaKey(mediaKey);
    return 1;
}

void BleKeyboard::release(int8_t button) {
    if (button >= 1 && button <= 64) {
        uint8_t field = (button - 1) / 32;
        uint8_t bit = (button - 1) % 32;
        _gamepadReport.buttons[field] &= ~(1UL << bit);
    }
    else if (button >= 65 && button <= 68) {
        uint8_t currentHat = _gamepadReport.hat;
        uint8_t directionIndex = button - 65; // Convert 65-68 to 0-3
        
        _gamepadReport.hat = hatRelease[directionIndex][currentHat];
    }
    sendGamepadReport();
}

void BleKeyboard::release(char b) {
  if (_useAbsolute) {
    // In absolute mode without coordinates, use current position
    release(_absoluteReport.x, _absoluteReport.y, b);
  } else {
    // Relative mode
    _mouseButtons &= ~b;
    move(0, 0, 0, 0);
  }
}

void BleKeyboard::release(uint16_t x, uint16_t y, char b) {
  // Auto-switches to absolute mode if coordinates are provided
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  _absoluteReport.buttons &= ~b;
  moveTo(x, y);
}

void BleKeyboard::releaseAll(void)
{
    // Release keyboard
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
    sendNKROReport();
    _mediaKeyBitmask = 0;
    sendReport();
}

void BleKeyboard::gamepadReleaseAll(void)
{
    // Release gamepad
    _gamepadReport.buttons[0] = 0;
    _gamepadReport.buttons[1] = 0;
    _gamepadReport.hat = HAT_CENTER;
    sendGamepadReport();
}

size_t BleKeyboard::write(uint8_t c) {
  uint8_t p = press(c);  // Keydown
  release(c);            // Keyup
  return p;
}

size_t BleKeyboard::write(int16_t modifier) {
  uint16_t p = press(modifier);  // Modifier down
  release(modifier);             // Modifier up
  return p;
}

size_t BleKeyboard::write(uint16_t mediaKey)
{
	uint16_t p = press(mediaKey);  // Keydown
	release(mediaKey);            // Keyup
	return p;
}

size_t BleKeyboard::write(const uint8_t *buffer, size_t size) {
	size_t n = 0;
	while (size--) {
		if (*buffer != '\r') {
			if (write(*buffer)) {
			  n++;
			} else {
			  break;
			}
		}
		buffer++;
	}
	return n;
}

void BleKeyboard::setModifiers(uint8_t modifiers) {
    _keyReportNKRO.modifiers = modifiers;
    sendNKROReport();
}

uint8_t BleKeyboard::getModifiers() {
    return _keyReportNKRO.modifiers;
}

void BleKeyboard::setMediaKeyBitmask(uint32_t bitmask) {
    _mediaKeyBitmask = bitmask;
    sendReport(); // Send the updated bitmask
}

uint32_t BleKeyboard::getMediaKeyBitmask() {
    return _mediaKeyBitmask;
}

uint32_t BleKeyboard::mediaKeyToBitmask(uint16_t usageCode) {
    switch (usageCode) {
      case 0x0130: return (1UL << 0);   // System Power
      case 0x0134: return (1UL << 1);   // System Sleep  
      case 0x0135: return (1UL << 2);   // System Wake
      case 0x00B5: return (1UL << 3);   // Next Track
      case 0x00B6: return (1UL << 4);   // Previous Track
      case 0x00B7: return (1UL << 5);   // Stop
      case 0x00CD: return (1UL << 6);   // Play/Pause
      case 0x00B3: return (1UL << 7);   // Fast Forward
      case 0x00B4: return (1UL << 8);   // Rewind
      case 0x00B8: return (1UL << 9);   // Eject
      case 0x00E2: return (1UL << 10);  // Mute
      case 0x00E9: return (1UL << 11);  // Volume Up
      case 0x00EA: return (1UL << 12);  // Volume Down
      case 0x006F: return (1UL << 13);  // Brightness Up
      case 0x0070: return (1UL << 14);  // Brightness Down
      case 0x0194: return (1UL << 15);  // My Computer
      case 0x0192: return (1UL << 16);  // Calculator
      case 0x018A: return (1UL << 17);  // Mail
      case 0x0183: return (1UL << 18);  // Media Selection
      case 0x0186: return (1UL << 19);  // Control Panel
      case 0x0187: return (1UL << 20);  // Launchpad
      case 0x0223: return (1UL << 21);  // WWW Home
      case 0x022A: return (1UL << 22);  // WWW Favorites
      case 0x0221: return (1UL << 23);  // WWW Search
      case 0x0226: return (1UL << 24);  // WWW Stop
      case 0x0224: return (1UL << 25);  // WWW Back
      case 0x0225: return (1UL << 26);  // WWW Forward
      case 0x0227: return (1UL << 27);  // WWW Refresh
      default: return 0;
    }
}

void BleKeyboard::addMediaKey(uint16_t mediaKey) {
    uint32_t keyBitmask = mediaKeyToBitmask(mediaKey);
    _mediaKeyBitmask |= keyBitmask;
    sendReport();
}

void BleKeyboard::removeMediaKey(uint16_t mediaKey) {
    uint32_t keyBitmask = mediaKeyToBitmask(mediaKey);
    _mediaKeyBitmask &= ~keyBitmask;
    sendReport();
}

uint8_t BleKeyboard::countPressedKeys() {
  uint8_t count = 0;
  // Only count non-modifier keys in the main key area
  // This ensures modifiers don't count toward the 6-key limit
  for (int i = 0; i < (NKRO_KEY_COUNT / 8); i++) {
    uint8_t byte = _keyReportNKRO.keys_bitmask[i];
    count += __builtin_popcount(byte);
  }
  return count;
}

void BleKeyboard::click(char b) {
  if (_useAbsolute) {
    // In absolute mode without coordinates, use current position
    click(_absoluteReport.x, _absoluteReport.y, b);
  } else {
    // Relative mode
    _mouseButtons = b;
    move(0, 0, 0, 0);
    _mouseButtons = 0;
    move(0, 0, 0, 0);
  }
}

void BleKeyboard::click(uint16_t x, uint16_t y, char b) {
  // Auto-switch to absolute mode if coordinates are provided
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  press(x, y, b);
  release(x, y, b);
}

void BleKeyboard::move(signed char x, signed char y, signed char wheel, signed char hWheel) {
  // Auto-switch to relative mode if moving with relative coordinates
  if (_useAbsolute) {
    useAbsolute(false);
  }
  
  if (this->isConnected() && inputMouse) {
    _mouseReport.buttons = _mouseButtons;
    _mouseReport.x = x;
    _mouseReport.y = y;
    _mouseReport.wheel = wheel;
    _mouseReport.hWheel = hWheel;
    
    inputMouse->setValue((uint8_t*)&_mouseReport, sizeof(_mouseReport));
    inputMouse->notify();
    delay(_delay_ms);
  }
}

void BleKeyboard::moveTo(uint16_t x, uint16_t y, signed char wheel, signed char hWheel) {
  // Auto-switch to absolute mode if moving to absolute coordinates
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  _absoluteReport.x = x;
  _absoluteReport.y = y;
  _absoluteReport.wheel = wheel;
  _absoluteReport.hWheel = hWheel;
  
  // Set tip switch based on pressure for better compatibility
  if (_absoluteReport.pressure > 0) {
    _absoluteReport.tipSwitch = 1;
  } else {
    _absoluteReport.tipSwitch = 0;
  }
  
  if (this->isConnected() && inputAbsolute) {
    inputAbsolute->setValue((uint8_t*)&_absoluteReport, sizeof(_absoluteReport));
    inputAbsolute->notify();
    delay(_delay_ms);
  }
}

bool BleKeyboard::mouseIsPressed(char b) {
  if (_useAbsolute) {
    return (_absoluteReport.buttons & b) != 0;
  } else {
    return (_mouseButtons & b) != 0;
  }
}

void BleKeyboard::mouseReleaseAll() {
  if (_useAbsolute) {
    _absoluteReport.buttons = 0;
    moveTo(_absoluteReport.x, _absoluteReport.y);
  } else {
    _mouseButtons = 0;
    move(0, 0, 0, 0);
  }
}

void BleKeyboard::useAbsolute(bool enable) {
  _useAbsolute = enable;
  Serial.printf("[%s] Switched to %s pointer mode\n", LOG_TAG, _useAbsolute ? "absolute" : "relative");
}

void BleKeyboard::useRelative(bool enable) {
  _useAbsolute = !enable;
  Serial.printf("[%s] Switched to %s pointer mode\n", LOG_TAG, _useAbsolute ? "absolute" : "relative");
}

void BleKeyboard::setAbsoluteRange(uint16_t minVal, uint16_t maxVal) {
  // This is just to scale your coordinates - the actual range is fixed to 32767 on both axes
  Serial.printf("[%s] Absolute pointer range set to %d-%d\n", LOG_TAG, minVal, maxVal);
}

bool BleKeyboard::isAbsoluteEnabled() {
  return _useAbsolute;
}

void BleKeyboard::setPressure(uint16_t pressure) {
  _absoluteReport.pressure = pressure;
}

void BleKeyboard::setTipSwitch(bool state) {
  _absoluteReport.tipSwitch = state ? 1 : 0;
}

void BleKeyboard::moveToWithPressure(uint16_t x, uint16_t y, uint16_t pressure, bool touching) {
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  _absoluteReport.x = x;
  _absoluteReport.y = y;
  _absoluteReport.pressure = pressure;
  _absoluteReport.tipSwitch = touching ? 1 : 0;
  
  if (this->isConnected() && inputAbsolute) {
    inputAbsolute->setValue((uint8_t*)&_absoluteReport, sizeof(_absoluteReport));
    inputAbsolute->notify();
    delay(_delay_ms);
  }
}

void BleKeyboard::clickWithPressure(uint16_t x, uint16_t y, uint16_t pressure, uint8_t button) {
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  // Press with pressure
  _absoluteReport.buttons |= button;
  _absoluteReport.x = x;
  _absoluteReport.y = y;
  _absoluteReport.pressure = pressure;
  _absoluteReport.tipSwitch = 1;
  // inRange removed from structure
  
  if (this->isConnected() && inputAbsolute) {
    inputAbsolute->setValue((uint8_t*)&_absoluteReport, sizeof(_absoluteReport));
    inputAbsolute->notify();
    delay(_delay_ms);
  }
  
  // Release
  _absoluteReport.buttons &= ~button;
  _absoluteReport.pressure = 0;
  _absoluteReport.tipSwitch = 0;
  
  if (this->isConnected() && inputAbsolute) {
    inputAbsolute->setValue((uint8_t*)&_absoluteReport, sizeof(_absoluteReport));
    inputAbsolute->notify();
    delay(_delay_ms);
  }
}

void BleKeyboard::beginStroke(uint16_t x, uint16_t y, uint16_t initialPressure) {
  moveToWithPressure(x, y, initialPressure, true);
}

void BleKeyboard::updateStroke(uint16_t x, uint16_t y, uint16_t pressure) {
  moveToWithPressure(x, y, pressure, true);
}

void BleKeyboard::endStroke(uint16_t x, uint16_t y) {
  moveToWithPressure(x, y, 0, false);
}

uint16_t BleKeyboard::getPressure() const {
  return _absoluteReport.pressure;
}

bool BleKeyboard::getTipSwitch() const {
  return _absoluteReport.tipSwitch != 0;
}

bool BleKeyboard::gamepadIsPressed(int8_t button) {
  // Handle regular buttons 1-64
  if (button >= 1 && button <= 64) {
    uint8_t field = (button - 1) / 32;
    uint8_t bit = (button - 1) % 32;
    return (_gamepadReport.buttons[field] & (1UL << bit)) != 0;
  }
  // Handle D-Pad as virtual buttons 65-68
  else if (button >= 65 && button <= 68) {
    uint8_t currentHat = _gamepadReport.hat;
    
    switch (button) {
      case 65: // DPAD_UP
        return (currentHat == HAT_UP || currentHat == HAT_UP_RIGHT || currentHat == HAT_UP_LEFT);
      case 66: // DPAD_RIGHT
        return (currentHat == HAT_RIGHT || currentHat == HAT_UP_RIGHT || currentHat == HAT_DOWN_RIGHT);
      case 67: // DPAD_DOWN
        return (currentHat == HAT_DOWN || currentHat == HAT_DOWN_RIGHT || currentHat == HAT_DOWN_LEFT);
      case 68: // DPAD_LEFT
        return (currentHat == HAT_LEFT || currentHat == HAT_UP_LEFT || currentHat == HAT_DOWN_LEFT);
    }
  }
  return false;
}

void BleKeyboard::gamepadSetAxis(int8_t axis, int16_t value) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    _gamepadReport.axes[axis] = value;
  }
  sendGamepadReport();
}

int16_t BleKeyboard::gamepadGetAxis(int8_t axis) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    return _gamepadReport.axes[axis];
  }
  return 0;
}

void BleKeyboard::gamepadSetAllAxes(int16_t values[GAMEPAD_AXIS_COUNT]) {
  memcpy(_gamepadReport.axes, values, sizeof(_gamepadReport.axes));
  sendGamepadReport();
}

void BleKeyboard::gamepadSetLeftStick(int16_t x, int16_t y) {
    _gamepadReport.axes[AXIS_LX] = x;
    _gamepadReport.axes[AXIS_LY] = y;
    sendGamepadReport();
}

void BleKeyboard::gamepadSetRightStick(int16_t x, int16_t y) {
    _gamepadReport.axes[AXIS_RX] = x;
    _gamepadReport.axes[AXIS_RY] = y;
    sendGamepadReport();
}

void BleKeyboard::gamepadSetTriggers(int16_t left, int16_t right) {
    _gamepadReport.axes[AXIS_LT] = left;
    _gamepadReport.axes[AXIS_RT] = right;
    sendGamepadReport();
}

void BleKeyboard::gamepadGetLeftStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_LX);
    y = gamepadGetAxis(AXIS_LY);
}

void BleKeyboard::gamepadGetRightStick(int16_t &x, int16_t &y) {
    x = gamepadGetAxis(AXIS_RX);
    y = gamepadGetAxis(AXIS_RY);
}

void BleKeyboard::onVibrate(void (*callback)(uint8_t leftMotor, uint8_t rightMotor)) {
  _onVibrateCallback = callback;
  Serial.printf("[%s] Vibrate callback registered\n", LOG_TAG);
}

bool BleKeyboard::isHapticsSupported() const {
  return (inputGamepad != nullptr) && (_onVibrateCallback != nullptr);
}

void BleKeyboard::sendGamepadReport() {
    if (this->isConnected() && inputGamepad) {
    inputGamepad->setValue((uint8_t*)&_gamepadReport, sizeof(_gamepadReport));
    inputGamepad->notify();
    delay(_delay_ms);
  }
}

void BleKeyboard::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) {
    Serial.printf("[%s] ESP-HID onConnect callback triggered - Security: %s, Encrypted: %s\n", LOG_TAG,
             isSecurityEnabled() ? "Enabled" : "Disabled", 
             desc->sec_state.encrypted ? "Yes" : "No");
    
    if (isSecurityEnabled()) {
        if (!desc->sec_state.encrypted) {
            Serial.printf("[%s] Initiating pairing for secure connection\n", LOG_TAG);
            NimBLEDevice::startSecurity(desc->conn_handle);
        }
    }
    
    if (inputNKRO) inputNKRO->notify();
    if (inputMediaKeys) inputMediaKeys->notify(); 
    
    Serial.printf("[%s] Client connected - Actual connection count: %d\n", LOG_TAG, NimBLEDevice::getServer()->getConnectedCount());
}

void BleKeyboard::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  NimBLEServerCallbacks::onDisconnect(pServer, connInfo, reason);
  
  // Restart advertising immediately when disconnected
  if (advertising) {
    advertising->start();
    Serial.printf("[%s] Advertising restarted after disconnect (reason: %d)\n", LOG_TAG, reason);
  }
}

void BleKeyboard::onWrite(NimBLECharacteristic* me) {
  Serial.printf("[%s] ESP-HID onWrite callback triggered!\n", LOG_TAG);
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  size_t length = me->getValue().length();
  
  // Check if this is haptic data from the gamepad characteristic
  if (me == inputGamepad && length >= 2) {
    // Gamepad output report format for haptics: [LeftMotor, RightMotor]
    // Most systems send 2 bytes for dual motor vibration
    uint8_t leftMotor = value[0];
    uint8_t rightMotor = (length > 1) ? value[1] : value[0]; // Fallback to single motor
    
    Serial.printf("[%s] Haptic feedback received: left=%d, right=%d\n", LOG_TAG, leftMotor, rightMotor);
    
    // Call user callback if registered
    if (_onVibrateCallback) {
      _onVibrateCallback(leftMotor, rightMotor);
    }
  } else {
    // Handle other characteristics (existing code)
    Serial.printf("[%s] special keys: %d\n", LOG_TAG, *value);
  }
}

void pollConnection(void * arg)
{
    BleKeyboard * kb = static_cast<BleKeyboard*>(arg);
    uint8_t cnt = NimBLEDevice::getServer()->getConnectedCount();

    if (kb->last_connected_count && !cnt) {   // Connection just dropped
        Serial.printf("[%s] Poller: link lost - restarting advertising\n", LOG_TAG);
        
        // Small delay to ensure BLE stack is ready
        delay(100);
        
        if (kb->advertising) {
            kb->advertising->stop();
            delay(50);
            if (!kb->advertising->start()) {
                Serial.printf("[%s] Poller: Failed to restart advertising, will retry\n", LOG_TAG);
            }
        }
    }
    kb->last_connected_count = cnt;
}

void BleKeyboard::enableBonding(bool enable) {
    bonding_enabled = enable;
    Serial.printf("[%s] Bonding %s\n", LOG_TAG, bonding_enabled ? "enabled" : "disabled");
}

void BleKeyboard::clearBonds() {
    NimBLEDevice::deleteAllBonds();
    Serial.printf("[%s] All bonds cleared\n", LOG_TAG);
}

bool BleKeyboard::isBonded() const {
    return NimBLEDevice::getNumBonds() > 0;
}
