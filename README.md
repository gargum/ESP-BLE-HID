# SquidHID

SquidHID allows you to make an ESP32 microcontroller act as a Keyboard, Mouse, Gamepad, or other [HID device](https://en.wikipedia.org/wiki/Human_interface_device).

The purpose of SquidHID is to make it as easy as possible to write firmware for complex devices. When your designs implement various features that are simply not supported by pre-existing tools like QMK or ZMK, SquidHID is here to help!

Currently, all non-USB related development/testing done using boards *without* USB host mode, like the ESP32-C3 Super Mini. In the long term however, support is planned for all ESP32 variants alongside every microcontroller ever used in a Xiao Seeed variant.

NimBLE, USB, and PS/2 are the available transport methods supported by SquidHID at this time, but the plan is to extend this as much as possible!

## Features

| CORE FEATURES                       | EXTENDED FUNCTIONS                                                                  | QOL & ADVANCED FUNCTIONS                                                                                                     |
| ----------------------------------- | ----------------------------------------------------------------------------------- | ---------------------------------------------------------------                                                              |
| Keyboard emulation                  | *NKRO + 6KRO with full support for modifiers & media keys*                          | *Send full text strings, press/release keys, and send full keystrokes*                                                       |
| Mouse emulation                     | *Absolute & Relative pointers you can hotswap between*                              | *Automatic context-aware switching between both pointer modes + full Android & iOS compatibility*                            |
| Gamepad emulation                   | *64 buttons + 1 D-pad, 2 analogue sticks, 2 analogue triggers*                      | *All inputs automatically recognized and populated in emualators like Dolphin and RPCS3*                                     |
| Digitizer emulation                 | *Pressure sensitivity + tip-switch, barrel, & eraser support*                       | *Programmable brushstroke macro support with variable pressure all throughout*                                               |
| Stenotype emulation                 | *PloverHID keys and reports are fully supported*                                    | *No settings to worry about. Mix-and-match stenotype keys with all other input methods to your heart's content!*             |
| Key Matrices                        | *Native support for pin pairs, single pins, MCP23XXX pins, and duplex matrices*     | *Automatically detects which pins use an external pull-up to function. Automatically handles matrices with full Japanese duplex configurations. Automatically configures any MCP23XXX variant if connected and enabled.* |
| Keymaps with Layering               | *Currently allows keymaps with up to 32 layers to be defined in sketches*           | *Syntax is designed to be easy to read, write, modify, and understand*                                                       |
| WS2812B RGB LED / NeoPixel support  | *Includes 21 pre-configured colour options and a range of helper functions*         | *Designed from the ground up to make defining and controlling your LEDs as easy and intuitive as possible for beginners*     |
| OLED support (128x64 only for now)  | *Supports text strings, primitive shapes, even bitmaps and changing single pixels!* | *QMK Logo Editor's "RAW" bitmaps are supported! Simply copy the `static const unsigned char PROGMEM raw_logo[]` bitmap definition, then rename to `static const uint8_t PROGMEM new_name[]`* to use in your sketches! |
| USB, BLE, and PS/2 support          | *Switch between protocols with a one-line change to the config.h file*              | *Automatically detects which protocols exist on your MCU and includes associated features and functions accordingly*         |
| Set the PID, VID, and version       | *Set the name, manufacturer, and the battery level*                                 | *Set what type of device the ESP32 advertises itself as. Choose anything from keyboard to keyring to insulin pump!*          |
| ESP32s in general are all supported | *Compatible even with boards that have no HID capabilities whatsoever*              | *Optimized for the ESP32s with the worst specs. Your board **will** work with this library!*                                 |

## Compatibility

 - [x] Compatible with Android devices with BLE
 - [x] Compatible with Windows devices with BLE
 - [x] Compatible with Linux devices with BLE
 - [x] Compatible with MacOS devices with BLE
 - [x] Compatible with iOS devices with BLE

 Some systems support older forms of Bluetooth like Bluedroid, but not the more modern NimBLE stack. Contemporary hardware in general and any hardware with support for things like BLE Audio will work with this library however.

## Features Currently in Development

- [ ] Split communication - Figuring out BLE mesh wireless support for the full 32,767 board maximum
- [ ] Documentation - Writing the docs for SquidHID
- [ ] Migrating from Arduino IDE/PlatformIO to Standalone - I enjoy making life harder for myself unneccesarily
- [ ] Hardware Abstraction Layer - Need to get the HAL system setup so I can add support for 10+ different platforms en masse
- [ ] Keymap advanced functions - Tap Dance, Combos, Dynamic Macros, Leaders, Alt-Mod, Mouse keys, Digitizer keys, and Joystick keys all still need to be added

## Installation
- (Make sure you can use the ESP32 with the Arduino IDE. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- Download the .ZIP file of this repo
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- You can now go to "File" -> "Examples" -> "SquidHID" and select any of the examples to get started.

## Example - Devices without Buttons

SquidHID is a very general-purpose toolkit, so it can be used to create devices consisting solely of a microcontroller, like mouse jigglers or even non-HID devices.

This sketch demonstrates some of these functions and features:

``` C++
/**
 * This example exists to demonstrate the basic functions of SquidHID
 *
 * Please feel free to use this example as a reference or template!
 */


#include <SQUIDHID.h>

SQUIDHID esp("ESP32-TEST", "gargum", 100);   // Here, we set the name to "ESP32-TEST", the manufacturer to "gargum", and the battery level to 100%

void setup() {
  esp.setAppearance(CYCLING_COMPUTER);          // You can set the device to advertise itself as a wide array of different things!
  esp.begin();                                  // Here, we start up our ESP32 running SquidHID
}

void loop() {
  esp.update();            // SquidHID requires you to call the 'update()' function in sketches. This is used to control things like the scanning interval.

  esp.press(KC_SLSH);      // SquidHID allows you to press down individual keys using the 'press' function!
  delay(2000);
  esp.press(KC_RSFT);      // NKRO is enabled by default, meaning you can just keep pushing down buttons without letting anything go!
  delay(2000);
  esp.press(KC_VOLD);      // Any keyboard button in SquidHID should be compatible with the 'press' function.
  delay(2000);
  esp.release(KC_RSFT);    // Using the 'release' function, you can let go of individual keys you've previously 'pressed'.
  delay(2000);
  esp.releaseAll();        // If you're holding down lots of buttons, you can also simply call the 'releaseAll' function.
  delay(2000);
  esp.write(KC_SLSH);      // You can also simply output any keyboard key one time in one command using 'write'.
  delay(2000);             // Please note that 'write' is NOT compatible with gamepad or mouse inputs at this time.
  esp.print("You can print text strings using SquidHID as well!");
  delay(2000);

  esp.press(MOUSE_RIGHT);  // The press function allows supports mouse buttons. 
  delay(2000);  
  esp.press(GAMEPAD_R1);   // All gamepad buttons are also supported by the press function.
  delay(2000);
  esp.releaseAll();
  esp.mouseReleaseAll();   // Do keep in mind, 'releaseAll' only releases keyboard and gamepad buttons, not mouse inputs.
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
## Example - Matrices and Keymaps

SquidHID allows users to define button matrices for use in devices like keyboards, computer mice, or gamepads.

The system used for defining matrices and keymaps was designed specifically to facilitate complex matrix configurations.

Below is an example sketch that defines the firmware for a 2x4 macropad that employs a very highly unusual matrix configuration:

``` C++
#include <SQUIDHID.h>

SQUIDHID tentacle("SquidHID"); // To start off, simply create and name your board.

// This matrix example includes both onboard MCU pins, and MCP23XXX pins.
// The matrix also includes keys that employ a direct wiring, and pin pairs.
//
// SquidHID does not utilize ROW2COL or COL2ROW, but instead the FROM-TO system.
// Pairs of pins are defined as {FROM, TO}.
// 
// A key wired as ROW2COL translates to {ROW, COL}
// A key wired as COL2ROW translates to {COL, ROW}

MATRIX(matrix) = {
  {1, 2}, {2, 1}, {A0, 3}, {3, A0},     // SquidHID's system means this matrix is valid.
  {B7},   {4},    {1, A0}, {2, 3}       // This makes complex matrices easier to define.
};

LAYER(base) = {
  KC_A,     KC_B,    KC_C,    KC_D,     // Many QMK keycodes are also valid in SquidHID.
  MO(1),    KC_BSPC, KC_SPC,  TG(2)     // Layering is also available in SquidHID.
};

LAYER(func) = {
  KC_VOLD,  KC_MPRV, KC_MNXT, KC_VOLU,  // Media keys are also included in SquidHID.
  TRANS,    TRANS,   TRANS,   TRANS     // Transparent keys also exist in SquidHID.
};

LAYER(game) = {
  GB_UP,    GB_LE,   GB_RI,   GB_DO,    // Some keys like gamepad buttons and steno keys
  TRANS,    TRANS,   TRANS,   TRANS     // use new, shortened aliases for ease of use.
};

KEYMAP(keymap) = {                      // Keymaps in SquidHID are simply sets of layers.
    base,
    func, 
    game
};

void setup() {
  
  tentacle.begin(matrix, keymap);       // After defining your matrix and keymap,
                                        // simply pass it to the begin function.
}

void loop() {
  
  tentacle.update();                    // Now you can call the update function to
                                        // check for/process any updates.
}
```

Alongside your user sketch, there is a config.h file that is used for function toggles and pin definitions:

``` C++
/**
 * @file config.h
 * @brief User function toggles for conditional compilation and feature-setting
 */

#define TRANSPORT        BLE        // Currently, the available transport methods for connecting to hosts are BLE, USB, and PS/2

#define KEYBOARD_ENABLE  true       // Enabling the Keyboard feature allows you to include keyboard buttons in your sketches
#define MEDIA_ENABLE     true       // Enabling the Media feature allows you to include media keys in your sketches
#define STENO_ENABLE     true       // Enabling the Steno feature enables the PloverHID feature, allowing you to create a stenotype machine
#define MOUSE_ENABLE     true       // Enabling the Mouse feature enables the relative pointer feature, allowing you to create a computer mouse or other pointing devicee
#define DIGITIZER_ENABLE true       // Enabling the Digitizer feature enables the absolute pointer feature, which allows you to create things like Android-compatible drawing tablets with tip and barrel switches
#define GAMEPAD_ENABLE   true       // Enabling the Gamepad feature enables the gamepad buttons, dual analogue joystick reports, the hat-switch d-pad reporting, and the dual analogue trigger reports

#define LED_ENABLE       true       // SquidHID comes bundled with a NeoPixel driver, allowing you to easily enable, define, and manipulate RGB LEDs
#define LED_PIN          20         // Simply define which pin is being used as the data pin
#define LED_COUNT        48         // Then, define how many LEDs are present

#define OLED_ENABLE      true       // SquidHID also includes an I2C OLED driver, enabling you to add simple screens to projects
#define OLED_HEIGHT      64         
#define OLED_WIDTH       128

#define MCP_ENABLE       true       // Enabling the MCP feature alongside the I2C and/or the SPI feature will allow MCP23XXX units to be automatically detected and configured for use in sketches

#define UART_ENABLE      true       // SquidHID is intended to support a wide range of protocols, so users may optionally define an RTS and CTS pin for use by the UART driver
#define TX_PIN           21
#define RX_PIN           20
#define RTS_PIN          0
#define CTS_PIN          1

#define I2C_ENABLE       true       
#define SDA_PIN          8
#define SCL_PIN          9

#define SPI_ENABLE       true       
#define MISO_PIN         5
#define MOSI_PIN         6
#define SCK_PIN          4
#define CS_PIN           7
```

## API docs
The interface is designed to copy the aliases used by keycodes in QMK. The aliases and functions for all [QMK Basic Keycodes](https://docs.qmk.fm/keycodes_basic) are fully implemented.
The sole exceptions to this are `KC_ASST` and `KC_MCTL`, which have been excluded from this library because I have no idea what either are meant to do.

A complete list of all available keycodes and commands is included in the `keywords.txt` text file.

To illustrate what this library has to offer, below is a table including the current complete list of the available **appearance codes**.

SquidHID uses **appearance codes** to determine what type of device the ESP32 advertises itself as to hosts over BLE. This corresponds to the icon next to the name of the device visible when scanning for Bluetooth devices to pair to, alongside the accompanying text explaining what the device does that is visible to the user on some operating systems. Every appearance code is tested and working.

| APPEARANCE CODE - REGULAR  | APPEARANCE CODE - UNUSUAL |
| -------------------------- | ------------------------- |
| GENERIC_HID                | OUTDOOR_SPORTS            |
| KEYBOARD                   | LOCATION_DISPLAY          |
| MOUSE                      | LOCATION_POD              |
| JOYSTICK                   | WEIGHT_SCALE              |
| GAMEPAD                    | EAR_THERMOMETER           |
| DIGITIZER                  | BLOOD_PRESSURE            |
| DIGITAL_PEN                | PULSE_OXIMETER            |
| HEADPHONES                 | GLUCOSE_METER             |
| DISPLAY                    | GLUCOSE_CONTINUOUS        |
| REMOTE_CONTROL             | MEDICATION_DELIVERY       |
| REMOTE_PRESENTATION        | INSULIN_PEN               |
| KEYRING                    | INSULIN_PUMP              |
| DESKTOP                    | WHEELCHAIR                |
| SERVER                     | MOBILITY_SCOOTER          |
| LAPTOP                     |
| TABLET                     |
| PHONE                      |
| SMARTWATCH                 |
| CYCLING_COMPUTER           |
| RUNNING_WALKING            |
| WEARABLE                   |
| WEARABLE_IN_SHOE           |
| WEARABLE_ON_SHOE           |
| WEARABLE_ON_HIP            |
| CLOCK                      |
| BARCODE_SCANNER            |
| CARD_READER                |
| IOT_GATEWAY                |



Features and settings that existed in the original project this repo is a fork of are still available:
You don't have to declare `SQUIDHID squidboard;` before using `squidboard.setName("NAME")` and `squidboard.setBatteryLevel(100)`! 
You can instead change `SQUIDHID squidboard;` to `SQUIDHID squidboard("NAME", "MANUFACTURER", 100);`. (Names longer than 15 characters will be truncated.)
By default the battery level will be set to 100%, the device name will be `SquidHID` and the manufacturer will be `SquidHID`.  

There is also a `setDelay` method to set a delay between each key event. E.g. `squidboard.setDelay(10)` (10 milliseconds). The default is `8`. The `setDelay` feature is to maximize compatibility between any devices created using this library, and any underpowered hardware or legacy applications one may wish to use.

## Credits

Credits to [T-vK](https://github.com/T-vK) and [the authors of the USB keyboard library](https://github.com/arduino-libraries/Keyboard/), whose work this project is a fork of!
