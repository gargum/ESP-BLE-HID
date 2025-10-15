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

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,          // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,          // USAGE (Keyboard)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  // ------------------------------------------------- Keyboard
  REPORT_ID(1),       KEYBOARD_ID,   //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,          //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,          //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,          //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,          //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x05,          //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,          //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,          //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,          //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,          //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,          //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,          //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x06,          //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(1), 0x65,          //   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,          //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x65,          //   USAGE_MAXIMUM (0x65)
  HIDINPUT(1),        0x00,          //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                 // END_COLLECTION
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,          // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,          // USAGE (Consumer Control)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  REPORT_ID(1),       MEDIA_KEYS_ID, //   REPORT_ID (2)
  USAGE_PAGE(1),      0x0C,          //   USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(2), 0xFF, 0x03,    //   LOGICAL_MAXIMUM (0x03FF) - Maximum 16-bit usage code
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1) - One bit, one code
  REPORT_COUNT(1),    0x1C,          //   REPORT_COUNT (1C) - 28 codes, so 28 bits
  
  // System Controls
  USAGE(2),           0x30, 0x01,    //   USAGE (System Power - 0x0130)    [bit 1]
  USAGE(2),           0x34, 0x01,    //   USAGE (System Sleep - 0x0134)    [bit 2]
  USAGE(2),           0x35, 0x01,    //   USAGE (System Wake - 0x0135)     [bit 3]
  
  // Transport Controls
  USAGE(1),           0xB5,    //   USAGE (Scan Next Track - 0x00B5)       [bit 4]
  USAGE(1),           0xB6,    //   USAGE (Scan Previous Track - 0x00B6)   [bit 5]
  USAGE(1),           0xB7,    //   USAGE (Stop - 0x00B7)                  [bit 6]
  USAGE(1),           0xCD,    //   USAGE (Play/Pause - 0x00CD)            [bit 7]
  USAGE(1),           0xB3,    //   USAGE (Fast Forward - 0x00B3)          [bit 8]
  USAGE(1),           0xB4,    //   USAGE (Rewind - 0x00B4)                [bit 9]
  USAGE(1),           0xB8,    //   USAGE (Eject - 0x00B8)                 [bit 10]
  
  // Volume Controls
  USAGE(1),           0xE2,    //   USAGE (Mute - 0x00E2)                  [bit 11]
  USAGE(1),           0xE9,    //   USAGE (Volume Increment - 0x00E9)      [bit 12]
  USAGE(1),           0xEA,    //   USAGE (Volume Decrement - 0x00EA)      [bit 13]
  
  // Display Controls
  USAGE(1),           0x6F,    //   USAGE (Brightness Up - 0x006F)         [bit 14]
  USAGE(1),           0x70,    //   USAGE (Brightness Down - 0x0070)       [bit 15]
  
  // Application Launch
  USAGE(2),           0x94, 0x01,    //   USAGE (My Computer - 0x0194)     [bit 16]
  USAGE(2),           0x92, 0x01,    //   USAGE (Calculator - 0x0192)      [bit 17]
  USAGE(2),           0x8A, 0x01,    //   USAGE (Mail - 0x018A)            [bit 18]
  USAGE(2),           0x83, 0x01,    //   USAGE (Media Selection - 0x0183) [bit 19]
  USAGE(2),           0x86, 0x01,    //   USAGE (Control Panel - 0x0186)   [bit 20]
  USAGE(2),           0x87, 0x01,    //   USAGE (Launchpad - 0x0187)       [bit 21]
  
  // Browser Controls
  USAGE(2),           0x23, 0x02,    //   USAGE (WWW Home - 0x0223)        [bit 22]
  USAGE(2),           0x2A, 0x02,    //   USAGE (WWW favorites - 0x022A)   [bit 23]
  USAGE(2),           0x21, 0x02,    //   USAGE (WWW search - 0x0221)      [bit 24]
  USAGE(2),           0x26, 0x02,    //   USAGE (WWW stop - 0x0226)        [bit 25]
  USAGE(2),           0x24, 0x02,    //   USAGE (WWW back - 0x0224)        [bit 26]
  USAGE(2),           0x25, 0x02,    //   USAGE (WWW forward - 0x0225)     [bit 27]
  USAGE(2),           0x27, 0x02,    //   USAGE (WWW refresh - 0x0227)     [bit 28]
  
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0)                  // END_COLLECTION
};

BleKeyboard::BleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) {}

void BleKeyboard::begin(void)
{
  BLEDevice::init(String(deviceName.c_str()));
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  hid = new BLEHIDDevice(pServer);
  inputKeyboard = hid->inputReport(KEYBOARD_ID);
  outputKeyboard = hid->outputReport(KEYBOARD_ID);
  inputMediaKeys = hid->inputReport(MEDIA_KEYS_ID);

  outputKeyboard->setCallbacks(this);
  hid->manufacturer()->setValue(String(deviceManufacturer.c_str()));
  hid->pnp(0x02, vid, pid, version);
  hid->hidInfo(0x00, 0x01);

#if defined(USE_NIMBLE)
    // For NimBLE
    BLEDevice::setSecurityAuth(true, true, true);
    BLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT); // "Just Works"
#else
    // For regular BLE stack
    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); // Bonding without MITM
    pSecurity->setCapability(ESP_IO_CAP_NONE); // "Just Works"
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
#endif // USE_NIMBLE

  hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();
  onStarted(pServer);

  advertising = pServer->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(false);
  
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advertisementData.setCompleteServices(BLEUUID(hid->hidService()->getUUID()));
  advertising->setAdvertisementData(advertisementData);
  
  advertising->start();
  hid->setBatteryLevel(batteryLevel);

  ESP_LOGI(LOG_TAG, "Advertising started!");
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

void BleKeyboard::sendReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();
#if defined(USE_NIMBLE)        
    // vTaskDelay(delayTicks);
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }	
}

void BleKeyboard::sendReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
    // Create a 32-bit bitmask to hold all 28 media key bits
    uint32_t mediaKeyBitmask = 0;
    
    // Convert the MediaKeyReport to the appropriate bit position
    // MediaKeyReport format: {usage_id, usage_page}
    uint16_t usageCode = (*keys)[0] | ((*keys)[1] << 8);
    
    // Map the usage code to the correct bit position in the 28-bit field
    // This mapping depends on the order of USAGE declarations in the HID descriptor
    switch (usageCode) {
      case 0x0130: mediaKeyBitmask = (1UL << 0); break;  // System Power
      case 0x0134: mediaKeyBitmask = (1UL << 1); break;  // System Sleep  
      case 0x0135: mediaKeyBitmask = (1UL << 2); break;  // System Wake
      case 0x00B5: mediaKeyBitmask = (1UL << 3); break;  // Next Track
      case 0x00B6: mediaKeyBitmask = (1UL << 4); break;  // Previous Track
      case 0x00B7: mediaKeyBitmask = (1UL << 5); break;  // Stop
      case 0x00CD: mediaKeyBitmask = (1UL << 6); break;  // Play/Pause
      case 0x00B3: mediaKeyBitmask = (1UL << 7); break;  // Fast Forward
      case 0x00B4: mediaKeyBitmask = (1UL << 8); break;  // Rewind
      case 0x00B8: mediaKeyBitmask = (1UL << 9); break;  // Eject
      case 0x00E2: mediaKeyBitmask = (1UL << 10); break; // Mute
      case 0x00E9: mediaKeyBitmask = (1UL << 11); break; // Volume Up
      case 0x00EA: mediaKeyBitmask = (1UL << 12); break; // Volume Down
      case 0x006F: mediaKeyBitmask = (1UL << 13); break; // Brightness Up
      case 0x0070: mediaKeyBitmask = (1UL << 14); break; // Brightness Down
      case 0x0194: mediaKeyBitmask = (1UL << 15); break; // My Computer
      case 0x0192: mediaKeyBitmask = (1UL << 16); break; // Calculator
      case 0x018A: mediaKeyBitmask = (1UL << 17); break; // Mail
      case 0x0183: mediaKeyBitmask = (1UL << 18); break; // Media Selection
      case 0x0186: mediaKeyBitmask = (1UL << 19); break; // Control Panel
      case 0x0187: mediaKeyBitmask = (1UL << 20); break; // Launchpad
      case 0x0223: mediaKeyBitmask = (1UL << 21); break; // WWW Home
      case 0x022A: mediaKeyBitmask = (1UL << 22); break; // WWW Favorites
      case 0x0221: mediaKeyBitmask = (1UL << 23); break; // WWW Search
      case 0x0226: mediaKeyBitmask = (1UL << 24); break; // WWW Stop
      case 0x0224: mediaKeyBitmask = (1UL << 25); break; // WWW Back
      case 0x0225: mediaKeyBitmask = (1UL << 26); break; // WWW Forward
      case 0x0227: mediaKeyBitmask = (1UL << 27); break; // WWW Refresh
      default: break;
    }
    
    // Send as 32-bit value (4 bytes) - the HID stack will use only the needed 28 bits
    this->inputMediaKeys->setValue((uint8_t*)&mediaKeyBitmask, sizeof(uint32_t));
    this->inputMediaKeys->notify();
#if defined(USE_NIMBLE)        
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }	
}

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
size_t BleKeyboard::press(uint8_t k)
{
    uint8_t i;
    
    // Check if it's a modifier key (using the helper function so it's easier for people to modify)
    if (isModifierKey(k)) {
        // It's a modifier key - set the appropriate bit in the modifiers byte
        _keyReport.modifiers |= k;
        k = 0;
    } 
    else if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    } 
    
    // Add k to the key report only if it's not already present
    // and if there is an empty slot (for non-modifier keys only)
    if (k != 0) {
        if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
            _keyReport.keys[2] != k && _keyReport.keys[3] != k &&
            _keyReport.keys[4] != k && _keyReport.keys[5] != k) {

            for (i = 0; i < 6; i++) {
                if (_keyReport.keys[i] == 0x00) {
                    _keyReport.keys[i] = k;
                    break;
                }
            }
            if (i == 6) {
                setWriteError();
                return 0;
            }
        }
    }
    
    sendReport(&_keyReport);
    return 1;
}

size_t BleKeyboard::press(const MediaKeyReport k)
{
    // Instead of OR-ing, just set the media key directly
    _mediaKeyReport[0] = k[0];
    _mediaKeyReport[1] = k[1];
    
    sendReport(&_mediaKeyReport);
    return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleKeyboard::release(uint8_t k)
{
    uint8_t i;
    
    // Check if it's a modifier key (using the helper function so it's easier for people to modify)
    if (isModifierKey(k)) {
        // It's a modifier key - clear the appropriate bit in the modifiers byte
        _keyReport.modifiers &= ~k;
        k = 0;
    }
    else if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    }
    
    // Test the key report to see if k is present. Clear it if it exists.
    // Check all positions in case the key is present more than once (which it shouldn't be)
    if (k != 0) {
        for (i = 0; i < 6; i++) {
            if (_keyReport.keys[i] == k) {
                _keyReport.keys[i] = 0x00;
            }
        }
    }
    
    sendReport(&_keyReport);
    return 1;
}

size_t BleKeyboard::release(const MediaKeyReport k)
{
    // Clear the media keys
    _mediaKeyReport[0] = 0;
    _mediaKeyReport[1] = 0;
    
    sendReport(&_mediaKeyReport);
    return 1;
}

void BleKeyboard::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
        _mediaKeyReport[0] = 0;
        _mediaKeyReport[1] = 0;
	sendReport(&_keyReport);
	sendReport(&_mediaKeyReport);
}

size_t BleKeyboard::write(uint8_t c)
{
	uint8_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const MediaKeyReport c)
{
	uint16_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
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
    _keyReport.modifiers = modifiers;
    sendReport(&_keyReport);
}

uint8_t BleKeyboard::getModifiers() {
    return _keyReport.modifiers;
}

void BleKeyboard::onConnect(BLEServer* pServer) {
  this->connected = true;

#if !defined(USE_NIMBLE)
  // You might not need to manually set these
  this->inputKeyboard->notify();
  this->inputMediaKeys->notify();
#endif // !USE_NIMBLE

}

void BleKeyboard::onDisconnect(BLEServer* pServer) {
  this->connected = false;

#if !defined(USE_NIMBLE)
  // You might not need to manually set these
  this->inputKeyboard->notify();
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
