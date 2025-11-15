/**
 * @file config.h
 * @brief User function toggles for conditional compilation and feature-setting
 */

// Configuration to select BLE implementation
#define IMPLEMENTATION   NIMBLE
#define KEYBOARD_ENABLE  true
#define MEDIA_ENABLE     true
#define GEMINIPR_ENABLE  true
#define MOUSE_ENABLE     true
#define DIGITIZER_ENABLE true
#define GAMEPAD_ENABLE   true

// Converting implementation to enum value
#if IMPLEMENTATION == NIMBLE
    #define IMPL SquidFactory::Implementation::NIMBLE
#elif IMPLEMENTATION == ARDUINO_BLE
    #define IMPL SquidFactory::Implementation::ARDUINO_BLE
#else
    #define IMPL SquidFactory::Implementation::NIMBLE
#endif
