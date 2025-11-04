# ESP-BLE-HID

This NimBLE-based library allows you to make the ESP32 act as a Bluetooth Keyboard, Mouse, Gamepad, or other [HID device](https://en.wikipedia.org/wiki/Human_interface_device).

ESP-BLE-HID is intended to serve as an ESP32-based alternative to tools like QMK and ZMK with added support for advanced features.

All development/testing is performed on boards *without* USB host mode, like the ESP32-C3 Super Mini.

## Features

| CORE FEATURES                       | EXTENDED FUNCTIONS                                                        | QOL & ADVANCED FUNCTIONS                                                                                             |
| ----------------------------------- | ------------------------------------------------------------------------- | ---------------------------------------------------------------                                                      |
| Keyboard emulation                  | *NKRO + 6KRO with full support for modifiers & media keys*                | *Send full text strings, press/release keys, and send full keystrokes*                                               |
| Mouse emulation                     | *Absolute & Relative pointers you can hotswap between*                    | *Automatic context-aware switching between both pointer modes*                                                       |
| Gamepad emulation                   | *64 buttons + 1 D-pad, 2 analogue sticks, 2 analogue triggers, + haptics* | *All inputs automatically recognized and populated in emualators like Dolphin and RPCS3*                             |
| Digitizer emulation                 | *Pressure sensitivity + tip-switch support*                               | *Programmable brushstroke macro support with variable pressure all throughout*                                       |
| Stenotype emulation                 | *GeminiPR keys and reports are fully supported*                           | *No settings to worry about. Mix-and-match stenotype keys with all other input methods to your heart's content!*     |
| Set the PID, VID, and version       | *Set the name, manufacturer, and the battery level*                       | *Set what type of device the ESP32 advertises itself as. Choose anything from keyboard to keyring to insulin pump!*  |
| 6-Digit PIN (Optional)              | *Hotswap between using a PIN to connect & Just Works no-PIN mode*         | *Change your PIN, even after you've already flashed and connected to the device!*                                    |
| ESP32s with BLE are all supported   | *Compatible with boards that have no HID capabilities whatsoever*         | *Optimized for the ESP32s with the worst specs. If your ESP32 has BLE, it **will** work with this library!*          |

## Compatibility

 - [x] Compatible with Android* *(Android itself lacks absolute pointer support)*
 - [x] Compatible with Windows
 - [x] Compatible with Linux
 - [x] Compatible with MacOS
 - [x] Compatible with iOS* *(iOS itself lacks absolute pointer support)*

## Features Currently in Development

- [ ] Matrix support - Developing the system for defining key matrices and encoder pins
- [ ] Keymap support - Developing the system to create keymaps corresponding to a user-defined matrix
- [ ] Split communication - Figuring out ESP-NOW wireless support for the full 20 board maximum
- [ ] Documentation - Writing the docs for ESP-BLE-HID
- [ ] Migrating from Arduino IDE/PlatformIO to Standalone - I enjoy making life harder for myself unneccesarily
- [ ] nRF support - Developing an actually easy/user-friendly system for flashing Nordic chips with my firmware

## Installation
- (Make sure you can use the ESP32 with the Arduino IDE. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- Download the .ZIP file of this repo
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- You can now go to "File" -> "Examples" -> "ESP-BLE-HID" and select any of the examples to get started.

## Example

``` C++
/**
 * This example exists to demonstrate the basic functions of ESP-BLE-HID
 *
 * Please feel free to use this example as a reference or template!
 */


#include <BleKeyboard.h>

BleKeyboard esp("ESP32-TEST", "gargum", 100);   // Here, we set the name to "ESP32-TEST", the manufacturer to "gargum", and the battery level to 100%

void setup() {
  esp.setAppearance(CYCLING_COMPUTER);          // You can set the device to advertise itself as a wide array of different things!
  esp.begin();                                  // Here, we start up our ESP32 running ESP-BLE-HID
}

void loop() {

  esp.press(KC_SLSH);      // ESP-BLE-HID allows you to press down individual keys using the 'press' function!
  delay(2000);
  esp.press(KC_RSFT);      // NKRO is enabled by default, meaning you can just keep pushing down buttons without letting anything go!
  delay(2000);
  esp.press(KC_VOLD);      // Any keyboard button in ESP-BLE-HID should be compatible with the 'press' function.
  delay(2000);
  esp.release(KC_RSFT);    // Using the 'release' function, you can let go of individual keys you've previously 'pressed'.
  delay(2000);
  esp.releaseAll();        // If you're holding down lots of buttons, you can also simply call the 'releaseAll' function.
  delay(2000);
  esp.write(KC_SLSH);      // You can also simply output any keyboard key one time in one command using 'write'.
  delay(2000);             // Please note that 'write' is NOT compatible with gamepad or mouse inputs at this time.
  esp.print("You can print text strings using ESP-BLE-HID as well!");
  delay(2000);

  esp.press(MOUSE_RIGHT);  // The press function allows supports mouse buttons. 
  delay(2000);  
  esp.press(5000, 5000, MOUSE_RIGHT); // When using the press function with a mouse button, you are even able to feed in coordinates!
  delay(2000);
  esp.press(GAMEPAD_R1);   // All gamepad buttons are also supported by the press function.
  delay(2000);
  esp.mouseReleaseAll();   // Do keep in mind, 'releaseAll' only releases keyboard buttons, not mouse or gamepad inputs.
  esp.gamepadReleaseAll(); // This was done to prevent 'releaseAll' from causing problems for more complex designs.
  delay(2000);
  
  esp.press(GAMEPAD_L1);   // The 'release' function on the other hand can parse all the same things 'press' can.
  delay(2000);
  esp.release(GAMEPAD_L1); // This means you can always use 'release' when you want to let go of just one button.
  delay(2000);

         //X    Y
  esp.move(100, 100);       // The 'move' function allows you to perform relative movements, like a computer mouse.
  delay(2000);
         //X    Y    W  hW
  esp.move(100, 100, 1, 1); // 'move' also allows you to set the state of the vertical and horizontal mouse wheels respectively.
  delay(2000);
           //X     Y
  esp.moveTo(1000, 1000);   // 'moveTo' parses absolute coordinates, like a finger on a touchscreen or like a Wacom tablet.
  delay(2000);
           //X     Y     W  hW
  esp.moveTo(1000, 1000, 1, 1); // Just like 'move', 'moveTo' also allows you to set the state of the vertical and horizontal mouse wheels.
  delay(2000);

  esp.click(MOUSE_LEFT);    // The 'click' function is the mouse's version of the 'write' function. I decided to call it 'click' because 'write' seemed confusing in this context.
  delay(2000);

  esp.click(5000, 5000, MOUSE_LEFT); // Just like the 'moveTo' function, 'click' is able to parse absolute coordinates. (By default, it will just click wherever the cursor is.)
  delay(2000);
  
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

## NimBLE Support
NimBLE saves a significant amount of RAM and FLASH, plus Bluedroid is not supported by some microcontrollers with BLE such as Nordic nRF series microcontrollers.

For these reasons, this library requries the use of NimBLE, at this time the [Arduino-NimBLE](https://github.com/h2zero/NimBLE-Arduino) library to be specific.

## Credits

Credits to [T-vK](https://github.com/T-vK) and [the authors of the USB keyboard library](https://github.com/arduino-libraries/Keyboard/), whose work this project is a fork of!
