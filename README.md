# ESP-BLE-HID

This library allows you to make the ESP32 act as a Bluetooth Keyboard, Mouse, Gamepad, or other [HID device](https://en.wikipedia.org/wiki/Human_interface_device).

ESP-BLE-HID is intended to serve as an ESP32-based alternative to tools like QMK and ZMK with added support for advanced features.

All development/testing is performed on boards *without* USB host mode, like the ESP32-C3 Super Mini.

## Features

| CORE FEATURES                       | EXTENDED FUNCTIONS                                                  | QOL & ADVANCED FUNCTIONS                                                                                             |
| ----------------------------------- | ------------------------------------------------------------------- | ---------------------------------------------------------------                                                      |
| Keyboard emulation                  | *NKRO + 6KRO with full support for modifiers & media keys*          | *Send full text strings, press/release keys, and send full keystrokes*                                               |
| Mouse emulation                     | *Absolute & Relative pointers you can hotswap between*              | *Automatic context-aware switching between both pointer modes*                                                       |
| Gamepad emulation                   | *64 buttons + 1 D-pad, 2 analogue sticks & 2 analogue triggers*     | *All inputs automatically recognized and populated in emualators like Dolphin and RPCS3*                             |
| Digitizer emulation                 | *Pressure sensitivity + tip-switch support*                         | *Programmable brushstroke macro support with variable pressure all throughout*                                       |
| Set the PID, VID, and version       | *Set the name, manufacturer, and the battery level*                 | *Set what type of device the ESP32 advertises itself as. Choose anything from keyboard to keyring to insulin pump!*  | 
| 6-Digit PIN (Optional)              | *Hotswap between using a PIN to connect & Just Works no-PIN mode*   | *Change your PIN, even after you've already flashed and connected to the device!*                                    |
| ESP32s with BLE are all supported   | *Compatible with boards that have no HID capabilities whatsoever*   | *Optimized for the ESP32s with the worst specs. If your ESP32 has BLE, it **will** work with this library!*          |

## Compatibility

 - [x] Compatible with Android* *(Android itself lacks absolute pointer support)*
 - [x] Compatible with Windows
 - [x] Compatible with Linux
 - [x] Compatible with MacOS
 - [x] Compatible with iOS* *(iOS itself lacks absolute pointer support)*

## Installation
- (Make sure you can use the ESP32 with the Arduino IDE. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- [Download the latest release of this library from the release page.](https://github.com/T-vK/ESP32-BLE-Keyboard/releases)
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- You can now go to "File" -> "Examples" -> "ESP-BLE-HID" and select any of the examples to get started.

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
  bleKeyboard.setName("Keyboard Demo"); // The name the ESP32 advertises itself as
  bleKeyboard.setManufacturer("Santa's Elves");
  bleKeyboard.setAppearance(DESKTOP); // The type of device the ESP32 advertises itself as. I've chosen desktop, because I can
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
The interface is designed to copy the aliases used by keycodes in QMK. The aliases and functions for all [QMK Basic Keycodes](https://docs.qmk.fm/keycodes_basic) are fully implemented.
The sole exceptions to this are `KC_ASST` and `KC_MCTL`, which have been excluded from this library because I have no idea what either are meant to do.

A complete list of all available keycodes and commands is included in the `keywords.txt` text file.

To illustrate what this library has to offer, below is a table including the current complete list of the available **media** keycodes alongside the available **appearance codes**. **Media** keycodes are self-explanatory, however **appearance codes** require more elaboration.

ESP-BLE-HID uses **appearance codes** to determine what type of device the ESP32 advertises itself as to hosts. This corresponds to the icon next to the name of the device visible when scanning for Bluetooth devices to pair to, alongside the accompanying text explaining what the device does that is visible to the user on some operating systems. Every appearance code is tested and working.

| FULL MEDIA KEYCODE                 | MEDIA KEYCODE ALIAS       | | APPEARANCE CODE - REGULAR  | APPEARANCE CODE - UNUSUAL |
| ---------------------------------- | ------------------------- |-| -------------------------- | ------------------------- |
| KEY_SYSTEM_POWER                   | KC_PWR                    | | GENERIC_HID                | OUTDOOR_SPORTS            |
| KEY_SYSTEM_SLEEP                   | KC_SLEP                   | | KEYBOARD                   | LOCATION_DISPLAY          |
| KEY_SYSTEM_WAKE                    | KC_WAKE                   | | MOUSE                      | LOCATION_POD              |
| KEY_NEXT_TRACK                     | KC_MNXT                   | | JOYSTICK                   | WEIGHT_SCALE              |
| KEY_PREVIOUS_TRACK                 | KC_MPRV                   | | GAMEPAD                    | EAR_THERMOMETER           |
| KEY_FAST_FORWARD                   | KC_MFFD                   | | DIGITIZER                  | BLOOD_PRESSURE            |
| KEY_REWIND                         | KC_MRWD                   | | DIGITAL_PEN                | PULSE_OXIMETER            |
| KEY_STOP                           | KC_MSTP                   | | HEADPHONES                 | GLUCOSE_METER             |
| KEY_PLAY_PAUSE                     | KC_MPLY                   | | DISPLAY                    | GLUCOSE_CONTINUOUS        |
| KEY_MUTE                           | KC_MUTE                   | | REMOTE_CONTROL             | MEDICATION_DELIVERY       |
| KEY_VOLUME_UP                      | KC_VOLU                   | | REMOTE_PRESENTATION        | INSULIN_PEN               |
| KEY_VOLUME_DOWN                    | KC_VOLD                   | | KEYRING                    | INSULIN_PUMP              |
| KEY_WWW_HOME                       | KC_WHOM                   | | DESKTOP                    | WHEELCHAIR                |
| KEY_LOCAL_MACHINE_BROWSER          | KC_MYCM                   | | SERVER                     | MOBILITY_SCOOTER          |
| KEY_CALCULATOR                     | KC_CALC                   | | LAPTOP                     |
| KEY_WWW_BOOKMARKS                  | KC_WFAV                   | | TABLET                     |
| KEY_WWW_SEARCH                     | KC_WSCH                   | | PHONE                      |
| KEY_WWW_STOP                       | KC_WSTP                   | | SMARTWATCH                 |
| KEY_WWW_REFRESH                    | KC_WREF                   | | CYCLING_COMPUTER           |
| KEY_WWW_BACK                       | KC_WBAK                   | | RUNNING_WALKING            |
| KEY_WWW_FORWARD                    | KC_WFWD                   | | WEARABLE                   |
| KEY_CONSUMER_CONTROL_CONFIGURATION | KC_MSEL                   | | WEARABLE_IN_SHOE           |
| KEY_EMAIL_READER                   | KC_MAIL                   | | WEARABLE_ON_SHOE           |
| KEY_EJECT                          | KC_EJCT                   | | WEARABLE_ON_HIP            |
| KEY_BRIGHTNESS_UP                  | KC_BRIU                   | | CLOCK                      |
| KEY_BRIGHTNESS_DOWN                | KC_BRID                   | | BARCODE_SCANNER            |
| KEY_CONTROL_PANEL                  | KC_CPNL                   | | CARD_READER                |
| KEY_LAUNCHPAD                      | KC_LPAD                   | | IOT_GATEWAY                |



Features and settings that existed in the original project this repo is a fork of are still available:
You don't have to declare `BleKeyboard bleKeyboard;` before using `bleKeyboard.setName("NAME")` and `bleKeyboard.setBatteryLevel(100)`! 
You can instead change `BleKeyboard bleKeyboard;` to `BleKeyboard bleKeyboard("NAME", "MANUFACTURER", 100);`. (Names longer than 15 characters will be truncated.)
By default the battery level will be set to 100%, the device name will be `ESP32 Keyboard` and the manufacturer will be `Espressif`.  

There is also a `setDelay` method to set a delay between each key event. E.g. `bleKeyboard.setDelay(10)` (10 milliseconds). The default is `8`. The `setDelay` feature is to maximize compatibility between any devices created using this library, and any underpowered hardware or legacy applications one may wish to use.

## NimBLE-Mode
The NimBLE mode enables a significant saving of RAM and FLASH memory.

The ESP32-C3 Super Mini was used for these tests and comparisons, which are now up-to-date!

### Comparison (SendKeyStrokes.ino)

**Standard**
```
RAM:   [=         ]   12% (used 40088 bytes out of the 327680 bytes in total)
Flash: [========  ]   83% (used 1097410 bytes out of the 1310720 bytes in total)
```

**NimBLE mode**
```
RAM:   [=         ]   7% (used 23568 bytes out of the 327680 bytes in total)
Flash: [====      ]  46% (used 612884 bytes out of the 1310720 bytes in total)
```

## How to activate NimBLE mode?

### ArduinoIDE: 
Uncomment the first line in BleKeyboard.h
```C++
#define USE_NIMBLE
```

## Credits
Credits to [T-vK](https://github.com/T-vK) and [the authors of the USB keyboard library](https://github.com/arduino-libraries/Keyboard/), whose work this project is a fork of!
