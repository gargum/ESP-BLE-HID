/**
 * @file config.h
 * @brief User function toggles for conditional compilation and feature-setting
 */

#define TRANSPORT        BLE

#define KEYBOARD_ENABLE  true
#define MEDIA_ENABLE     true
#define STENO_ENABLE     true
#define MOUSE_ENABLE     true
#define DIGITIZER_ENABLE true
#define GAMEPAD_ENABLE   true

#define LED_ENABLE       false
// #define LED_PIN          20
// #define LED_COUNT        48

#define OLED_ENABLE      false
// #define OLED_HEIGHT      64
// #define OLED_WIDTH       128

#define MCP_ENABLE       false
#define SHIFT_REGISTERS  false

#define UART_ENABLE      false
// #define TX_PIN           21
// #define RX_PIN           20
// #define RTS_PIN          0
// #define CTS_PIN          1

#define I2C_ENABLE       false
// #define SDA_PIN          8
// #define SCL_PIN          9

#define SPI_ENABLE       false
// #define MISO_PIN         5
// #define MOSI_PIN         6
// #define SCK_PIN          4
// #define CS_PIN           7

#define QSPI_ENABLE      false
// #define IO0_PIN          7
// #define IO1_PIN          2
// #define IO2_PIN          5
// #define IO3_PIN          4
// #define CS0_PIN          10
// #define CLK_PIN          6

#define TWAI_ENABLE      false
// #define TWAI_TX_PIN      0
// #define TWAI_RX_PIN      1
// #define TWAI_BUS_PIN     3
// #define TWAI_CLK_PIN     4

#define CAN_BUS_ENABLE   false
// #define CAN_TX_PIN       0
// #define CAN_RX_PIN       1
