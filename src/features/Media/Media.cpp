#include "Media.h"
#include "NimBLEDevice.h"

static const char* MEDIA_TAG = "BLEMEDIA";

BLEMEDIA::BLEMEDIA() 
    : inputMediaKeys(nullptr), _mediaKeyBitmask(0), _delay_ms(7) {
}

void BLEMEDIA::begin(NimBLECharacteristic* mediaChar, uint32_t delay_ms) {
    inputMediaKeys = mediaChar;
    _delay_ms = delay_ms;
    _mediaKeyBitmask = 0;
}

bool BLEMEDIA::isConnected() {
    if (!inputMediaKeys) return false;
    return NimBLEDevice::getServer() && NimBLEDevice::getServer()->getConnectedCount() > 0;
}

size_t BLEMEDIA::press(uint16_t mediaKey) {
    addMediaKey(mediaKey);
    return 1;
}

size_t BLEMEDIA::release(uint16_t mediaKey) {
    removeMediaKey(mediaKey);
    return 1;
}

size_t BLEMEDIA::write(uint16_t mediaKey) {
    uint16_t p = press(mediaKey);
    release(mediaKey);
    return p;
}

void BLEMEDIA::setMediaKeyBitmask(uint32_t bitmask) {
    _mediaKeyBitmask = bitmask;
    sendMediaReport();
}

uint32_t BLEMEDIA::getMediaKeyBitmask() {
    return _mediaKeyBitmask;
}

uint32_t BLEMEDIA::mediaKeyToBitmask(uint16_t usageCode) {
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

void BLEMEDIA::addMediaKey(uint16_t mediaKey) {
    uint32_t keyBitmask = mediaKeyToBitmask(mediaKey);
    _mediaKeyBitmask |= keyBitmask;
    sendMediaReport();
}

void BLEMEDIA::removeMediaKey(uint16_t mediaKey) {
    uint32_t keyBitmask = mediaKeyToBitmask(mediaKey);
    _mediaKeyBitmask &= ~keyBitmask;
    sendMediaReport();
}

void BLEMEDIA::sendMediaReport() {
    if (isConnected() && inputMediaKeys) {
        inputMediaKeys->setValue((uint8_t*)&_mediaKeyBitmask, sizeof(uint32_t));
        inputMediaKeys->notify();
        delay(_delay_ms);
    }
}

void BLEMEDIA::releaseAll() {
    _mediaKeyBitmask = 0;
    sendMediaReport();
}
