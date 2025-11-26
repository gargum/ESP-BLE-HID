/*!
 * @file MCP23XXX.h
 */

#ifndef MCP23XXX_H
#define MCP23XXX_H

#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>

// registers
#define MCP23XXX_IODIR 0x00   //!< I/O direction register
#define MCP23XXX_IPOL 0x01    //!< Input polarity register
#define MCP23XXX_GPINTEN 0x02 //!< Interrupt-on-change control register
#define MCP23XXX_DEFVAL 0x03  //!< Default compare register for interrupt-on-change
#define MCP23XXX_INTCON 0x04  //!< Interrupt control register
#define MCP23XXX_IOCON 0x05   //!< Configuration register
#define MCP23XXX_GPPU 0x06    //!< Pull-up resistor configuration register
#define MCP23XXX_INTF 0x07    //!< Interrupt flag register
#define MCP23XXX_INTCAP 0x08  //!< Interrupt capture register
#define MCP23XXX_GPIO 0x09    //!< Port register
#define MCP23XXX_OLAT 0x0A    //!< Output latch register

#define MCP23XXX_ADDR 0x20    //!< Default I2C Address
#define MCP23XXX_SPI_READ 0x41 //!< SPI read command
#define MCP23XXX_SPI_WRITE 0x40 //!< SPI write command

#define MCP_PORT(pin) ((pin < 8) ? 0 : 1) //!< Determine port from pin number
#define MCP23XXX_INT_ERR 255 //!< Interrupt error

// These are just the names of the IO pins on the MCP.
#define A0 0x80
#define A1 0x81
#define A2 0x82
#define A3 0x83
#define A4 0x84
#define A5 0x85
#define A6 0x86
#define A7 0x87
#define B0 0x88
#define B1 0x89
#define B2 0x8A
#define B3 0x8B
#define B4 0x8C
#define B5 0x8D
#define B6 0x8E
#define B7 0x8F

// Base class for all MCP23XXX variants.
class MCP23XXX {
public:
  // init
  bool begin_I2C(uint8_t i2c_addr = MCP23XXX_ADDR, TwoWire *wire = &Wire);
  bool begin_SPI(uint8_t cs_pin, SPIClass *theSPI = &SPI, uint8_t _hw_addr = 0x00);
  bool begin_SPI(int8_t cs_pin, int8_t sck_pin, int8_t miso_pin, int8_t mosi_pin, uint8_t _hw_addr = 0x00);

  // main Arduino API methods
  void pinMode(uint8_t pin, uint8_t mode);
  uint8_t digitalRead(uint8_t pin);
  void digitalWrite(uint8_t pin, uint8_t value);

  // bulk access
  uint8_t readGPIO(uint8_t port = 0);
  void writeGPIO(uint8_t value, uint8_t port = 0);

  // interrupts
  void setupInterrupts(bool mirroring, bool openDrain, uint8_t polarity);
  void setupInterruptPin(uint8_t pin, uint8_t mode = CHANGE);
  void disableInterruptPin(uint8_t pin);
  void clearInterrupts();
  uint8_t getLastInterruptPin();
  uint16_t getCapturedInterrupt();

protected:
  TwoWire *_wire = nullptr;           ///< Pointer to I2C bus interface
  uint8_t _i2c_addr;                  ///< I2C device address
  uint8_t _cs_pin = -1;               ///< SPI chip select pin
  SPIClass *_spi = nullptr;           ///< Pointer to SPI bus interface
  bool _use_spi = false;              ///< True if using SPI
  uint8_t pinCount;                   ///< Total number of GPIO pins
  uint8_t hw_addr;                    ///< HW address matching A2/A1/A0 pins

  // Low-level communication methods
  uint8_t readRegister(uint8_t reg);
  void writeRegister(uint8_t reg, uint8_t value);
  uint16_t readRegister16(uint8_t reg);
  void writeRegister16(uint8_t reg, uint16_t value);
  uint8_t readRegister(uint8_t reg, uint8_t port);
  void writeRegister(uint8_t reg, uint8_t value, uint8_t port);
  
  uint16_t getRegister(uint8_t baseAddress, uint8_t port = 0);

private:
  uint8_t buffer[4];
};

// Class for the MCP23008 I2C and MCP23S08 SPI IO expanders.
class MCP23X08 : public MCP23XXX {
public:
  MCP23X08();
  void enableAddrPins();
};

// Class for MCP23017 I2C and MCP23S17 SPI IO expanders.
class MCP23X17 : public MCP23XXX {
public:
  MCP23X17();
  uint8_t readGPIOA();
  void writeGPIOA(uint8_t value);
  uint8_t readGPIOB();
  void writeGPIOB(uint8_t value);
  uint16_t readGPIOAB();
  void writeGPIOAB(uint16_t value);
  void enableAddrPins();
};

// Helper macros
#define IS_MCP_PIN(pin) ((pin) >= 0x80 && (pin) <= 0x8F)

#endif
