# ESP-BLE-HID

This library allows you to make the ESP32 act as a Bluetooth Keyboard, Mouse, Gamepad, or other [HID device](https://en.wikipedia.org/wiki/Human_interface_device)

All development and testing is done on boards *without* the ability to act as USB hosts such as the ESP32-C3 Super Mini.

## Features

 - [x] Send key strokes
 - [x] Send text
 - [x] Press/release individual keys
 - [x] Media keys
 - [x] Read Numlock/Capslock/Scrolllock state
 - [x] Set battery level
 - [x] 6KRO & NKRO support 
 - [x] Relative pointers 
 - [ ] Absolute pointers
 - [ ] Digitizers with pen/barrel
 - [ ] Touchscreens with multitouch 
 - [ ] Joystick emulation
 - [x] Compatible with Android
 - [x] Compatible with Windows
 - [x] Compatible with Linux
 - [x] Compatible with MacOS
 - [x] Compatible with iOS

## Installation
- (Make sure you can use the ESP32 with the Arduino IDE. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- [Download the latest release of this library from the release page.](https://github.com/T-vK/ESP32-BLE-Keyboard/releases)
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- You can now go to "File" -> "Examples" -> "ESP32 BLE Keyboard" and select any of the examples to get started.

## Example

``` C++
/**
 * This example demonstrates a little of what this library can do with the ESP32, even without USB Host Mode
 */
#include <BleKeyboard.h>

BleKeyboard bleKeyboard;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  //Name must be set before calling bleKeyboard.begin()
  bleKeyboard.setName("Keyboard Demo");
  bleKeyboard.begin();
  bleKeyboard.use6KRO(); //NKRO is turned on by default, 6KRO must be explicitly specified
  bleKeyboard.setBatteryLevel(100); //Any number can be put here and it will report properly
}

void loop() {
  if(bleKeyboard.isConnected()) {
    Serial.println("Sending 'Hello world'...");
    bleKeyboard.print("Hello world");

    delay(1000);

    Serial.println("Sending Enter key...");
    bleKeyboard.write(KC_ENT);

    delay(1000);

    Serial.println("Sending Play/Pause media key...");
    bleKeyboard.write(KC_MPLY);

    delay(1000);

    Serial.println("Maximizing the screen brightness...");
    bleKeyboard.press(KC_BRIU);
    delay(2000);
    bleKeyboard.release(KC_BRIU);

    delay(1000);
    
   //
   // Below is an example of pressing multiple keyboard modifiers 
   // which by default is commented out. 
   // 
   /* Serial.println("Sending Ctrl+Alt+Delete...");
    bleKeyboard.press(KC_LCTL);
    bleKeyboard.press(KC_LALT);
    bleKeyboard.press(KC_DEL);
    delay(100);
    bleKeyboard.releaseAll();
    */

  }
  Serial.println("Waiting 5 seconds...");
  delay(5000);
}
```

## API docs
The interface is designed to copy the aliases used by keycodes in QMK. The aliases and functions for all QMK Basic Keycodes are fully implemented.
The sole exceptions to this are `KC_ASST` and `KC_MCTL`, which have been excluded from this library because I have no idea what either are meant to do.

[QMK Docs - Basic Keycodes](https://docs.qmk.fm/keycodes_basic)

A complete list of all available keycodes and commands is included in the `keywords.txt` text file.

To illustrate what this library has to offer, here is the current complete list of just the available *media* keycodes:

| FULL KEYCODE                       | ALIAS / SHORTFORM KEYCODE |
| ---------------------------------- | ------- |
| KEY_SYSTEM_POWER                   | KC_PWR  |
| KEY_SYSTEM_SLEEP                   | KC_SLEP |
| KEY_SYSTEM_WAKE                    | KC_WAKE |
| KEY_NEXT_TRACK                     | KC_MNXT |
| KEY_PREVIOUS_TRACK                 | KC_MPRV |
| KEY_FAST_FORWARD                   | KC_MFFD |
| KEY_REWIND                         | KC_MRWD |
| KEY_STOP                           | KC_MSTP |
| KEY_PLAY_PAUSE                     | KC_MPLY |
| KEY_MUTE                           | KC_MUTE |
| KEY_VOLUME_UP                      | KC_VOLU |
| KEY_VOLUME_DOWN                    | KC_VOLD |
| KEY_WWW_HOME                       | KC_WHOM |
| KEY_LOCAL_MACHINE_BROWSER          | KC_MYCM |
| KEY_CALCULATOR                     | KC_CALC |
| KEY_WWW_BOOKMARKS                  | KC_WFAV |
| KEY_WWW_SEARCH                     | KC_WSCH |
| KEY_WWW_STOP                       | KC_WSTP |
| KEY_WWW_REFRESH                    | KC_WREF |
| KEY_WWW_BACK                       | KC_WBAK |
| KEY_WWW_FORWARD                    | KC_WFWD |
| KEY_CONSUMER_CONTROL_CONFIGURATION | KC_MSEL |
| KEY_EMAIL_READER                   | KC_MAIL |
| KEY_EJECT                          | KC_EJCT |
| KEY_BRIGHTNESS_UP                  | KC_BRIU |
| KEY_BRIGHTNESS_DOWN                | KC_BRID |
| KEY_CONTROL_PANEL                  | KC_CPNL |
| KEY_LAUNCHPAD                      | KC_LPAD |



Features and settings that existed in the original project this repo is a fork of are still available:
You don't have to declare `BleKeyboard bleKeyboard;` before using `bleKeyboard.setName("NAME")` and `bleKeyboard.setBatteryLevel(100)`! 
You can instead change `BleKeyboard bleKeyboard;` to `BleKeyboard bleKeyboard("NAME", "MANUFACTURER", 100);`. (Names longer than 15 characters will be truncated.)
By default the battery level will be set to 100%, the device name will be `ESP32 Keyboard` and the manufacturer will be `Espressif`.  

There is also a `setDelay` method to set a delay between each key event. E.g. `bleKeyboard.setDelay(10)` (10 milliseconds). The default is `8`.  
The `setDelay` feature is to maximize compatibility between any devices created using this library, and any underpowered hardware or legacy applications one may wish to use.

## NimBLE-Mode
The NimBLE mode enables a significant saving of RAM and FLASH memory.

### Comparison (SendKeyStrokes.ino at compile-time)

**Standard**
```
RAM:   [=         ]   9.3% (used 30548 bytes from 327680 bytes)
Flash: [========  ]  75.8% (used 994120 bytes from 1310720 bytes)
```

**NimBLE mode**
```
RAM:   [=         ]   8.3% (used 27180 bytes from 327680 bytes)
Flash: [====      ]  44.2% (used 579158 bytes from 1310720 bytes)
```

### Comparison (SendKeyStrokes.ino at run-time)

|   | Standard | NimBLE mode | difference
|---|--:|--:|--:|
| `ESP.getHeapSize()`   | 296.804 | 321.252 | **+ 24.448**  |
| `ESP.getFreeHeap()`   | 143.572 | 260.764 | **+ 117.192** |
| `ESP.getSketchSize()` | 994.224 | 579.264 | **- 414.960** |

## How to activate NimBLE mode?

### ArduinoIDE: 
Uncomment the first line in BleKeyboard.h
```C++
#define USE_NIMBLE
```

### PlatformIO:
Change your `platformio.ini` to the following settings
```ini
lib_deps = 
  NimBLE-Arduino

build_flags = 
  -D USE_NIMBLE
```

## Credits
Credits to [T-vK](https://github.com/T-vK) and [the authors of the USB keyboard library](https://github.com/arduino-libraries/Keyboard/), whose work this project is a fork of!
