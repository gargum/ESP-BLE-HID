#include "BleKeyboard.h"

#if defined(USE_NIMBLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#else
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#endif // USE_NIMBLE
#include "HIDTypes.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include "esp_log.h"
static const char* LOG_TAG = "BleKeyboard";

// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02
#define NKRO_KEYBOARD_ID 0x03
#define MOUSE_ID 0x04
#define DIGITIZER_ID 0x05
#define GAMEPAD_ID 0x06

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
  LOGICAL_MINIMUM(1), 0x00,             // LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,             // LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,             // REPORT_SIZE (1)
  REPORT_COUNT(1),    0x68,             // REPORT_COUNT (104) - 104 keys
  USAGE_MINIMUM(1),   0x00,             // USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x67,             // USAGE_MAXIMUM (103) - Maximum key index
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
  USAGE(2),           0x30, 0x01,       // USAGE (System Power - 0x0130)    [bit 1]
  USAGE(2),           0x34, 0x01,       // USAGE (System Sleep - 0x0134)    [bit 2]
  USAGE(2),           0x35, 0x01,       // USAGE (System Wake - 0x0135)     [bit 3] 
  // Transport Controls
  USAGE(1),           0xB5,             // USAGE (Scan Next Track - 0x00B5)       [bit 4]
  USAGE(1),           0xB6,             // USAGE (Scan Previous Track - 0x00B6)   [bit 5]
  USAGE(1),           0xB7,             // USAGE (Stop - 0x00B7)                  [bit 6]
  USAGE(1),           0xCD,             // USAGE (Play/Pause - 0x00CD)            [bit 7]
  USAGE(1),           0xB3,             // USAGE (Fast Forward - 0x00B3)          [bit 8]
  USAGE(1),           0xB4,             // USAGE (Rewind - 0x00B4)                [bit 9]
  USAGE(1),           0xB8,             // USAGE (Eject - 0x00B8)                 [bit 10]
  // Volume Controls
  USAGE(1),           0xE2,             // USAGE (Mute - 0x00E2)                  [bit 11]
  USAGE(1),           0xE9,             // USAGE (Volume Increment - 0x00E9)      [bit 12]
  USAGE(1),           0xEA,             // USAGE (Volume Decrement - 0x00EA)      [bit 13] 
  // Display Controls
  USAGE(1),           0x6F,             // USAGE (Brightness Up - 0x006F)         [bit 14]
  USAGE(1),           0x70,             // USAGE (Brightness Down - 0x0070)       [bit 15] 
  // Application Launch
  USAGE(2),           0x94, 0x01,       // USAGE (My Computer - 0x0194)     [bit 16]
  USAGE(2),           0x92, 0x01,       // USAGE (Calculator - 0x0192)      [bit 17]
  USAGE(2),           0x8A, 0x01,       // USAGE (Mail - 0x018A)            [bit 18]
  USAGE(2),           0x83, 0x01,       // USAGE (Media Selection - 0x0183) [bit 19]
  USAGE(2),           0x86, 0x01,       // USAGE (Control Panel - 0x0186)   [bit 20]
  USAGE(2),           0x87, 0x01,       // USAGE (Launchpad - 0x0187)       [bit 21]
  // Browser Controls
  USAGE(2),           0x23, 0x02,       // USAGE (WWW Home - 0x0223)        [bit 22]
  USAGE(2),           0x2A, 0x02,       // USAGE (WWW favorites - 0x022A)   [bit 23]
  USAGE(2),           0x21, 0x02,       // USAGE (WWW search - 0x0221)      [bit 24]
  USAGE(2),           0x26, 0x02,       // USAGE (WWW stop - 0x0226)        [bit 25]
  USAGE(2),           0x24, 0x02,       // USAGE (WWW back - 0x0224)        [bit 26]
  USAGE(2),           0x25, 0x02,       // USAGE (WWW forward - 0x0225)     [bit 27]
  USAGE(2),           0x27, 0x02,       // USAGE (WWW refresh - 0x0227)     [bit 28]
  HIDINPUT(1),        0x02,             // INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                      // END_COLLECTION
  // ------------------------------------------------- Relative Pointer - Needed for Pointers on Android/iOS
  // Mouse Report Descriptor
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
  END_COLLECTION(0),                      // END_COLLECTION (Application)

};

BleKeyboard::BleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) 
    , securityPin("") 
    , _mediaKeyBitmask(0) 
    , _useNKRO(true)
    , _mouseButtons(0)
    , _useAbsolute(false) {
  // Initialize reports
  memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
  memset(&_mouseReport, 0, sizeof(_mouseReport));
  memset(&_absoluteReport, 0, sizeof(_absoluteReport));
  memset(&_gamepadReport, 0, sizeof(_gamepadReport));
  _gamepadReport.hat = HAT_CENTER; // Initialize hat to center position
}

BleKeyboard::~BleKeyboard() {}

void BleKeyboard::begin(void)
{
  // Check if BLE is already initialized
  if (!BLEDevice::getInitialized()) {
    BLEDevice::init(String(deviceName.c_str()));
  }
  
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  // Initialize keyboard input report
  hid = new BLEHIDDevice(pServer);
  outputKeyboard = hid->outputReport(KEYBOARD_ID);
  inputMediaKeys = hid->inputReport(MEDIA_KEYS_ID);
  inputNKRO = hid->inputReport(NKRO_KEYBOARD_ID);

  outputKeyboard->setCallbacks(this);
  hid->manufacturer()->setValue(String(deviceManufacturer.c_str()));
  hid->pnp(0x02, vid, pid, version);
  hid->hidInfo(0x00, 0x01);

  // Initialize mouse input report
  inputMouse = hid->inputReport(0x04);    // Use report ID 4 for relative pointer
  inputAbsolute = hid->inputReport(0x05); // Use report ID 5 for absolute pointer
  inputGamepad = hid->inputReport(0x06);  // Use report ID 6 for gamepad
  
  if (isPinSecurityEnabled()) {
      setStaticPasskey();
  }
  

#if defined(USE_NIMBLE)
    if (securityPin.empty()) {
        ESP_LOGI(LOG_TAG, "Using Just Works security mode");
        BLEDevice::setSecurityAuth(true, true, true);
        BLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    } else {
        ESP_LOGI(LOG_TAG, "Using PIN security mode: %s", securityPin.c_str());
        BLEDevice::setSecurityAuth(true, true, true);
        // Use KEYBOARD_ONLY to force PIN entry flow
        BLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);
        // Set security callbacks
        BLEDevice::setSecurityCallbacks(this);
    }
#else
    BLESecurity* pSecurity = new BLESecurity();
    if (securityPin.empty()) {
        ESP_LOGI(LOG_TAG, "Using Just Works security mode");
        pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
        pSecurity->setCapability(ESP_IO_CAP_NONE);
        pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    } else {
        ESP_LOGI(LOG_TAG, "Using PIN security mode: %s", securityPin.c_str());
        pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
        // Use KEYBOARD_ONLY to force PIN entry flow
        pSecurity->setCapability(ESP_IO_CAP_IN);
        pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        // Set security callbacks
        BLEDevice::setSecurityCallbacks(this);
    }
#endif

  hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();

  advertising = pServer->getAdvertising();
  
  // Set advertising parameters before setting data
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(true); // Changed to true for better discovery
  
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advertisementData.setName(deviceName.c_str()); // Explicitly set the name
  advertisementData.setCompleteServices(BLEUUID(hid->hidService()->getUUID()));
  
  // Also set scan response data
  BLEAdvertisementData scanResponseData;
  scanResponseData.setCompleteServices(BLEUUID(hid->hidService()->getUUID()));
  scanResponseData.setName(deviceName.c_str());
  
  advertising->setAdvertisementData(advertisementData);
  advertising->setScanResponseData(scanResponseData);
  
  // Call onStarted after HID is set up but before advertising starts
  onStarted(pServer);
  
  advertising->start();
  hid->setBatteryLevel(batteryLevel);

  ESP_LOGI(LOG_TAG, "Advertising started!");
  ESP_LOGI(LOG_TAG, "Device name: %s", deviceName.c_str());
  ESP_LOGI(LOG_TAG, "Service UUID: %s", hid->hidService()->getUUID().toString().c_str());
  ESP_LOGI(LOG_TAG, "Using %s mode by default", _useNKRO ? "NKRO" : "6KRO");
}

void BleKeyboard::end(void)
{
}

bool BleKeyboard::isConnected(void) {
  return this->connected;
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

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 * 
 * @param ms Time in milliseconds
 */
void BleKeyboard::setDelay(uint32_t ms) {
  this->_delay_ms = ms;
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

void BleKeyboard::setStaticPasskey() {
    if (!isPinSecurityEnabled()) {
        return;
    }
    
    // Convert the user-set PIN to integer
    uint32_t pin = std::stoi(securityPin);
    
#if defined(USE_NIMBLE)
    // For NimBLE - set the static passkey
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_ONLY;
    ble_hs_cfg.sm_sc = 1; // Secure Connections
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    
    // Set static passkey
    ble_sm_set_static_passkey(pin);
    ESP_LOGI(LOG_TAG, "NimBLE static passkey set to: %06d", pin);
#else
    // For ESP-IDF BLE - set the static passkey
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &pin, sizeof(uint32_t));
    
    // Also set the IO capability to indicate we provide the PIN
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    
    ESP_LOGI(LOG_TAG, "ESP-IDF static passkey set to: %06d", pin);
#endif
}

#if !defined(USE_NIMBLE)
void BleKeyboard::gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    // For now, let's keep this empty - we'll handle PIN in the security callbacks
    // This prevents compilation errors while we focus on the main issue
}
#endif

void BleKeyboard::setSecurityPin(const std::string& pin) {
    // Validate PIN format (6 digits for BLE)
    if (pin.length() == 6) {
        bool valid = true;
        for (char c : pin) {
            if (!isdigit(c)) {
                valid = false;
                break;
            }
        }
        if (valid) {
            // Ensure PIN is exactly 6 digits with leading zeros if needed
            this->securityPin = pin;
            ESP_LOGI(LOG_TAG, "Security PIN set: %s (will use PIN pairing)", securityPin.c_str());
            
            // Re-configure security with new PIN if BLE is already initialized
            if (BLEDevice::getInitialized()) {
                setStaticPasskey();
            }
            return;
        }
    }
    
    // Invalid PIN - fall back to Just Works
    ESP_LOGW(LOG_TAG, "Invalid PIN format: '%s'. Must be 6 digits. Using Just Works mode.", pin.c_str());
    this->securityPin = "";
}

void BleKeyboard::clearSecurityPin() {
    this->securityPin = "";
    ESP_LOGI(LOG_TAG, "Security PIN cleared - using Just Works mode");
}

bool BleKeyboard::isPinSecurityEnabled() const {
    return !securityPin.empty();
}

#if defined(USE_NIMBLE)

uint32_t BleKeyboard::onPassKeyRequest() {
    if (isPinSecurityEnabled()) {
        ESP_LOGI(LOG_TAG, "PassKeyRequest - Using user-set PIN: %s", securityPin.c_str());
        // Convert the user-set string PIN to integer
        return std::stoi(securityPin);
    } else {
        ESP_LOGI(LOG_TAG, "PassKeyRequest - Just Works mode, no PIN required");
        return 0; // For Just Works mode
    }
}

void BleKeyboard::onPassKeyNotify(uint32_t pass_key) {
    // This should NOT be called in our desired flow - we want PassKeyRequest instead
    ESP_LOGW(LOG_TAG, "Unexpected PassKeyNotify: %06d - This indicates wrong security flow!", pass_key);
}

bool BleKeyboard::onConfirmPIN(uint32_t pass_key) {
    if (!isPinSecurityEnabled()) {
        ESP_LOGI(LOG_TAG, "ConfirmPIN - Just Works mode, auto-accept");
        return true;
    }
    
    // Convert expected PIN to integer for direct comparison
    uint32_t expected_pin = std::stoi(securityPin);
    ESP_LOGI(LOG_TAG, "ConfirmPIN - Entered: %06d, Expected: %06d", pass_key, expected_pin);
    
    bool match = (pass_key == expected_pin);
    ESP_LOGI(LOG_TAG, "PIN %s", match ? "MATCH" : "MISMATCH");
    
    if (!match) {
        ESP_LOGE(LOG_TAG, "PIN verification failed: entered=%06d, expected=%06d", 
                 pass_key, expected_pin);
    }
    
    return match;
}

void BleKeyboard::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    ESP_LOGI(LOG_TAG, "Authentication complete - encrypted: %d, authenticated: %d, bonded: %d",
             desc->sec_state.encrypted, desc->sec_state.authenticated, desc->sec_state.bonded);
}

#else

uint32_t BleKeyboard::onPassKeyRequest() {
    if (isPinSecurityEnabled()) {
        ESP_LOGI(LOG_TAG, "PassKeyRequest - Using user-set PIN: %s", securityPin.c_str());
        // Convert the user-set string PIN to integer
        return std::stoi(securityPin);
    } else {
        ESP_LOGI(LOG_TAG, "PassKeyRequest - Just Works mode, no PIN required");
        return 0; // For Just Works mode
    }
}

void BleKeyboard::onPassKeyNotify(uint32_t pass_key) {
    // This should NOT be called in our desired flow - we want PassKeyRequest instead
    ESP_LOGW(LOG_TAG, "Unexpected PassKeyNotify: %06d - This indicates wrong security flow!", pass_key);
}

bool BleKeyboard::onSecurityRequest() {
    ESP_LOGI(LOG_TAG, "SecurityRequest");
    return true;
}

bool BleKeyboard::onConfirmPIN(uint32_t pass_key) {
    if (!isPinSecurityEnabled()) {
        ESP_LOGI(LOG_TAG, "ConfirmPIN - Just Works mode, auto-accept");
        return true;
    }
    
    // Convert expected PIN to integer for direct comparison
    uint32_t expected_pin = std::stoi(securityPin);
    ESP_LOGI(LOG_TAG, "ConfirmPIN - Entered: %06d, Expected: %06d", pass_key, expected_pin);
    
    bool match = (pass_key == expected_pin);
    ESP_LOGI(LOG_TAG, "PIN %s", match ? "MATCH" : "MISMATCH");
    
    if (!match) {
        ESP_LOGE(LOG_TAG, "PIN verification failed: entered=%06d, expected=%06d", 
                 pass_key, expected_pin);
    }
    
    return match;
}

void BleKeyboard::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
    ESP_LOGI(LOG_TAG, "Authentication complete: %s", 
             cmpl.success ? "SUCCESS" : "FAILED");
    if (cmpl.success) {
        ESP_LOGI(LOG_TAG, "Auth Mode: %d, Key Present: %d, Key Type: %d",
                 cmpl.auth_mode, cmpl.key_present, cmpl.key_type);
    } else {
        ESP_LOGE(LOG_TAG, "Authentication failed, reason: %d", cmpl.fail_reason);
    }
}

#endif

void BleKeyboard::sendReport()
{
  if (this->isConnected())
  {
    // Send the current media key bitmask
    this->inputMediaKeys->setValue((uint8_t*)&_mediaKeyBitmask, sizeof(uint32_t));
    this->inputMediaKeys->notify();
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }	
}

void BleKeyboard::sendNKROReport() {
  if (this->isConnected() && inputNKRO) {
    inputNKRO->setValue((uint8_t*)&_keyReportNKRO, sizeof(KeyReportNKRO));
    inputNKRO->notify();
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
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
  ESP_LOGI(LOG_TAG, "Switched to %s mode", _useNKRO ? "NKRO" : "6KRO");
}

void BleKeyboard::use6KRO(bool state) {
  _useNKRO = !state; // state = enabled, therefore _useNKRO = not true/enabled = false
  ESP_LOGI(LOG_TAG, "Switched to %s mode", _useNKRO ? "NKRO" : "6KRO");
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
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
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
	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
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
  // Always use NKRO internally
  if (isModifierKey(k)) {
    // Modifiers don't count toward the 6-key limit
    _keyReportNKRO.modifiers |= k;
  } else if (k >= 136) { 
    k = k - 136;
  }
  
  if (k != 0) {
    // Check if we're already at 6 non-modifier keys
    uint8_t pressedKeys = countPressedKeys();
    
    if (pressedKeys >= 6) {
      // In 6KRO mode, don't allow more than 6 keys
      if (!_useNKRO) {
        setWriteError();
        return 0;
      }
    }
    
    // Update the bitmask
    updateNKROBitmask(k, true);
  }
  
  sendNKROReport();
  return 1;
}

size_t BleKeyboard::press(uint16_t mediaKey)
{
    addMediaKey(mediaKey);
    return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleKeyboard::release(uint8_t k) {
  if (isModifierKey(k)) {
    _keyReportNKRO.modifiers &= ~k;
  } else if (k >= 136) {
    k = k - 136;
  }
  
  if (k != 0) {
    updateNKROBitmask(k, false);
  }
  
  sendNKROReport();
  return 1;
}

size_t BleKeyboard::release(uint16_t mediaKey)
{
    removeMediaKey(mediaKey);
    return 1;
}


void BleKeyboard::releaseAll(void)
{
    memset(&_keyReportNKRO, 0, sizeof(_keyReportNKRO));
    sendNKROReport();
    
    _mediaKeyBitmask = 0;
    sendReport();
}

size_t BleKeyboard::write(uint8_t c)
{
	uint8_t p = press(c);  // Keydown
	release(c);            // Keyup
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

bool BleKeyboard::isModifierKey(uint8_t k) {
    return (k >= 0x01 && k <= 0x80 && ((k & (k-1)) == 0));
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
  for (int i = 0; i < sizeof(_keyReportNKRO.keys_bitmask); i++) {
    uint8_t byte = _keyReportNKRO.keys_bitmask[i];
    while (byte) {
      count += byte & 1;
      byte >>= 1;
    }
  }
  return count;
}

void BleKeyboard::mouseClick(uint8_t b) {
  if (_useAbsolute) {
    // In absolute mode without coordinates, use current position
    mouseClick(_absoluteReport.x, _absoluteReport.y, b);
  } else {
    // Relative mode
    _mouseButtons = b;
    mouseMove(0, 0, 0, 0);
    _mouseButtons = 0;
    mouseMove(0, 0, 0, 0);
  }
}

void BleKeyboard::mouseClick(uint16_t x, uint16_t y, uint8_t b) {
  // Auto-switch to absolute mode if coordinates are provided
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  mousePress(x, y, b);
  mouseRelease(x, y, b);
}

void BleKeyboard::mouseMove(signed char x, signed char y, signed char wheel, signed char hWheel) {
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
    
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }
}

void BleKeyboard::mouseMoveTo(uint16_t x, uint16_t y, signed char wheel, signed char hWheel) {
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
    
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }
}

void BleKeyboard::mousePress(uint8_t b) {
  if (_useAbsolute) {
    // In absolute mode without coordinates, use current position
    mousePress(_absoluteReport.x, _absoluteReport.y, b);
  } else {
    // Relative mode
    _mouseButtons |= b;
    mouseMove(0, 0, 0, 0);
  }
}

void BleKeyboard::mousePress(uint16_t x, uint16_t y, uint8_t b) {
  // Auto-switch to absolute mode if coordinates are provided
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  _absoluteReport.buttons |= b;
  mouseMoveTo(x, y);
}

void BleKeyboard::mouseRelease(uint8_t b) {
  if (_useAbsolute) {
    // In absolute mode without coordinates, use current position
    mouseRelease(_absoluteReport.x, _absoluteReport.y, b);
  } else {
    // Relative mode
    _mouseButtons &= ~b;
    mouseMove(0, 0, 0, 0);
  }
}

void BleKeyboard::mouseRelease(uint16_t x, uint16_t y, uint8_t b) {
  // Auto-switches to absolute mode if coordinates are provided
  if (!_useAbsolute) {
    useAbsolute(true);
  }
  
  _absoluteReport.buttons &= ~b;
  mouseMoveTo(x, y);
}

bool BleKeyboard::mouseIsPressed(uint8_t b) {
  if (_useAbsolute) {
    return (_absoluteReport.buttons & b) != 0;
  } else {
    return (_mouseButtons & b) != 0;
  }
}

void BleKeyboard::mouseReleaseAll() {
  if (_useAbsolute) {
    _absoluteReport.buttons = 0;
    mouseMoveTo(_absoluteReport.x, _absoluteReport.y);
  } else {
    _mouseButtons = 0;
    mouseMove(0, 0, 0, 0);
  }
}

void BleKeyboard::useAbsolute(bool enable) {
  _useAbsolute = enable;
  ESP_LOGI(LOG_TAG, "Switched to %s pointer mode", _useAbsolute ? "absolute" : "relative");
}

void BleKeyboard::useRelative(bool enable) {
  _useAbsolute = !enable;
  ESP_LOGI(LOG_TAG, "Switched to %s pointer mode", _useAbsolute ? "absolute" : "relative");
}

void BleKeyboard::setAbsoluteRange(uint16_t minVal, uint16_t maxVal) {
  // This is just to scale your coordinates - the actual range is fixed to 32767 on both axes
  ESP_LOGI(LOG_TAG, "Absolute pointer range set to %d-%d", minVal, maxVal);
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
    
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
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
    
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }
  
  // Release
  _absoluteReport.buttons &= ~button;
  _absoluteReport.pressure = 0;
  _absoluteReport.tipSwitch = 0;
  
  if (this->isConnected() && inputAbsolute) {
    inputAbsolute->setValue((uint8_t*)&_absoluteReport, sizeof(_absoluteReport));
    inputAbsolute->notify();
    
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
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

void BleKeyboard::gamepadPress(uint8_t button) {
  // Handle regular buttons 1-64
  if (button >= 1 && button <= 64) {
    uint8_t field = (button - 1) / 32;
    uint8_t bit = (button - 1) % 32;
    _gamepadReport.buttons[field] |= (1UL << bit);
  }
  // Handle D-Pad as virtual buttons 65-68
  else if (button >= 65 && button <= 68) {
    uint8_t currentHat = _gamepadReport.hat;
    uint8_t newDirection = currentHat;
    
    switch (button) {
      case 65: // DPAD_UP
        if (currentHat == HAT_CENTER) newDirection = HAT_UP;
        else if (currentHat == HAT_RIGHT) newDirection = HAT_UP_RIGHT;
        else if (currentHat == HAT_LEFT) newDirection = HAT_UP_LEFT;
        else if (currentHat == HAT_DOWN_RIGHT) newDirection = HAT_UP_RIGHT;
        else if (currentHat == HAT_DOWN_LEFT) newDirection = HAT_UP_LEFT;
        else if (currentHat == HAT_DOWN) newDirection = HAT_UP;
        break;
      case 66: // DPAD_RIGHT
        if (currentHat == HAT_CENTER) newDirection = HAT_RIGHT;
        else if (currentHat == HAT_UP) newDirection = HAT_UP_RIGHT;
        else if (currentHat == HAT_DOWN) newDirection = HAT_DOWN_RIGHT;
        else if (currentHat == HAT_UP_LEFT) newDirection = HAT_UP_RIGHT;
        else if (currentHat == HAT_DOWN_LEFT) newDirection = HAT_DOWN_RIGHT;
        else if (currentHat == HAT_LEFT) newDirection = HAT_RIGHT;
        break;
      case 67: // DPAD_DOWN
        if (currentHat == HAT_CENTER) newDirection = HAT_DOWN;
        else if (currentHat == HAT_RIGHT) newDirection = HAT_DOWN_RIGHT;
        else if (currentHat == HAT_LEFT) newDirection = HAT_DOWN_LEFT;
        else if (currentHat == HAT_UP_RIGHT) newDirection = HAT_DOWN_RIGHT;
        else if (currentHat == HAT_UP_LEFT) newDirection = HAT_DOWN_LEFT;
        else if (currentHat == HAT_UP) newDirection = HAT_DOWN;
        break;
      case 68: // DPAD_LEFT
        if (currentHat == HAT_CENTER) newDirection = HAT_LEFT;
        else if (currentHat == HAT_UP) newDirection = HAT_UP_LEFT;
        else if (currentHat == HAT_DOWN) newDirection = HAT_DOWN_LEFT;
        else if (currentHat == HAT_UP_RIGHT) newDirection = HAT_UP_LEFT;
        else if (currentHat == HAT_DOWN_RIGHT) newDirection = HAT_DOWN_LEFT;
        else if (currentHat == HAT_RIGHT) newDirection = HAT_LEFT;
        break;
    }
    _gamepadReport.hat = newDirection;
  }
  sendGamepadReport();
}

void BleKeyboard::gamepadRelease(uint8_t button) {
  // Handle regular buttons 1-64
  if (button >= 1 && button <= 64) {
    uint8_t field = (button - 1) / 32;
    uint8_t bit = (button - 1) % 32;
    _gamepadReport.buttons[field] &= ~(1UL << bit);
  }
  // Handle D-Pad as virtual buttons 65-68
  else if (button >= 65 && button <= 68) {
    uint8_t currentHat = _gamepadReport.hat;
    uint8_t newDirection = currentHat;
    
    switch (button) {
      case 65: // DPAD_UP - release up
        if (currentHat == HAT_UP) newDirection = HAT_CENTER;
        else if (currentHat == HAT_UP_RIGHT) newDirection = HAT_RIGHT;
        else if (currentHat == HAT_UP_LEFT) newDirection = HAT_LEFT;
        break;
      case 66: // DPAD_RIGHT - release right
        if (currentHat == HAT_RIGHT) newDirection = HAT_CENTER;
        else if (currentHat == HAT_UP_RIGHT) newDirection = HAT_UP;
        else if (currentHat == HAT_DOWN_RIGHT) newDirection = HAT_DOWN;
        break;
      case 67: // DPAD_DOWN - release down
        if (currentHat == HAT_DOWN) newDirection = HAT_CENTER;
        else if (currentHat == HAT_DOWN_RIGHT) newDirection = HAT_RIGHT;
        else if (currentHat == HAT_DOWN_LEFT) newDirection = HAT_LEFT;
        break;
      case 68: // DPAD_LEFT - release left
        if (currentHat == HAT_LEFT) newDirection = HAT_CENTER;
        else if (currentHat == HAT_UP_LEFT) newDirection = HAT_UP;
        else if (currentHat == HAT_DOWN_LEFT) newDirection = HAT_DOWN;
        break;
    }
    _gamepadReport.hat = newDirection;
  }
  sendGamepadReport();
}

bool BleKeyboard::gamepadIsPressed(uint8_t button) {
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

void BleKeyboard::gamepadReleaseAll() {
  _gamepadReport.buttons[0] = 0;
  _gamepadReport.buttons[1] = 0;
  _gamepadReport.hat = HAT_CENTER; // Also center the D-Pad
  sendGamepadReport();
}

void BleKeyboard::gamepadSetAxis(uint8_t axis, int16_t value) {
  if (axis < GAMEPAD_AXIS_COUNT) {
    _gamepadReport.axes[axis] = value;
  }
  sendGamepadReport();
}

int16_t BleKeyboard::gamepadGetAxis(uint8_t axis) {
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

void BleKeyboard::sendGamepadReport() {
    if (this->isConnected() && inputGamepad) {
    inputGamepad->setValue((uint8_t*)&_gamepadReport, sizeof(_gamepadReport));
    inputGamepad->notify();
    
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }
}

void BleKeyboard::onConnect(BLEServer* pServer) {
  this->connected = true;

#if !defined(USE_NIMBLE)
  // You might not need to manually set these
  this->inputNKRO->notify();
  this->inputMediaKeys->notify();
#endif // !USE_NIMBLE

}

void BleKeyboard::onDisconnect(BLEServer* pServer) {
  this->connected = false;

#if !defined(USE_NIMBLE)
  // You might not need to manually set these
  this->inputNKRO->notify();
  this->inputMediaKeys->notify();
  
  advertising->start();
#endif // !USE_NIMBLE

}

void BleKeyboard::onWrite(BLECharacteristic* me) {
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  (void)value;
  ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}

void BleKeyboard::delay_ms(uint64_t ms) {
  uint64_t m = esp_timer_get_time();
  if(ms){
    uint64_t e = (m + (ms * 1000));
    if(m > e){ //overflow
        while(esp_timer_get_time() > e) { }
    }
    while(esp_timer_get_time() < e) {}
  }
}
