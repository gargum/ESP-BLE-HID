/**
* @file Data.h
* @brief Global/Public data for everything in the library to use - keeps things a tad bit cleaner
*/

#include <unordered_set>
#include <functional>
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>
#include <string>
#include <map>

#include "drivers/Software/HID/SquidHIDTypes.h"
#include "drivers/Software/Event/Types.h"
#include "drivers/Software/Log/Log.h"
#include "drivers/Appearance.h"
#include "config.h"

// Report IDs
#define NKRO_ID       0x01
#define MEDIA_KEYS_ID 0x02
#define SPACETRANS_ID 0x03
#define SPACEROTAT_ID 0x04
#define SPACECLICK_ID 0x05
#define MOUSE_ID      0x06
#define DIGITIZER_ID  0x07
#define GAMEPAD_ID    0x08
#define STENO_ID      0x50

// Logging Tags
#define MAIN_TAG        "SQUIDHID"
#define MATRIX_TAG      "SQUIDMATRIX"
#define KEYMAP_TAG      "SQUIDKEYMAP"

#define BLE_TAG         "SQUIDBLE"
#define USB_TAG         "SQUIDUSB"
#define PS2_TAG         "SQUIDPS2"

#define NKRO_TAG        "SQUIDNKRO"
#define MEDIA_TAG       "SQUIDMEDIA"
#define MOUSE_TAG       "SQUIDMOUSE"
#define DIGI_TAG        "SQUIDTABLET"
#define GAMEPAD_TAG     "SQUIDGAMEPAD"
#define SPACEMOUSE_TAG  "SQUID6DOF"
#define STENO_TAG       "SQUIDSTENO"

// General Data
#define SQUIDHID_VERSION          "0.9.0"
#define SQUIDHID_VERSION_MAJOR    0
#define SQUIDHID_VERSION_MINOR    9
#define SQUIDHID_VERSION_REVISION 0

// HID Data
#define MAX_DESCRIPTOR_SIZE       512 // BLE has a 512 byte max so I just made that the global max

// Matrix Data
#define SCAN_INTERVAL             1
#define POLL_INTERVAL             250

// NKRO Data
#define NKRO_KEY_COUNT            252

// Digitizer Data
#define DEFAULT_WIDTH             1920
#define DEFAULT_HEIGHT            1080

// Gamepad Data
#define GAMEPAD_BUTTON_COUNT      64
#define GAMEPAD_ANALOGUE_COUNT    6
