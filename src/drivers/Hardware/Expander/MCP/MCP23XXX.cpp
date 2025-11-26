/**
 * @file MCP23XXX.cpp
 * @brief Implementation of the MCP drivers
 */
 
#include "MCP23XXX.h"

bool MCP23XXX::begin_I2C(uint8_t i2c_addr, TwoWire *wire) {
  _wire = wire;
  _i2c_addr = i2c_addr;
  _use_spi = false;
  
  // Test communication by reading a register
  _wire->begin();
  _wire->beginTransmission(_i2c_addr);
  return (_wire->endTransmission() == 0);
}

bool MCP23XXX::begin_SPI(uint8_t cs_pin, SPIClass *theSPI, uint8_t _hw_addr) {
  this->hw_addr = _hw_addr;
  _cs_pin = cs_pin;
  _spi = theSPI;
  _use_spi = true;
  
  pinMode(_cs_pin, OUTPUT);
  digitalWrite(_cs_pin, HIGH);
  _spi->begin();
  return true;
}

bool MCP23XXX::begin_SPI(int8_t cs_pin, int8_t sck_pin, int8_t miso_pin, int8_t mosi_pin, uint8_t _hw_addr) {
  this->hw_addr = _hw_addr;
  _cs_pin = cs_pin;
  _use_spi = true;
  
  pinMode(_cs_pin, OUTPUT);
  digitalWrite(_cs_pin, HIGH);
  pinMode(sck_pin, OUTPUT);
  pinMode(mosi_pin, OUTPUT);
  pinMode(miso_pin, INPUT);
  
  // Initialize SPI settings for software SPI
  _spi = nullptr; // Not using hardware SPI
  return true;
}

uint8_t MCP23XXX::readRegister(uint8_t reg) {
  if (_use_spi) {
    digitalWrite(_cs_pin, LOW);
    
    if (_spi) {
      // Hardware SPI
      _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
      _spi->transfer(MCP23XXX_SPI_READ | (hw_addr << 1));
      _spi->transfer(reg);
      uint8_t value = _spi->transfer(0);
      _spi->endTransaction();
      digitalWrite(_cs_pin, HIGH);
      return value;
    } else {
      // Software SPI
      // Implementation for software SPI will go here eventually
      digitalWrite(_cs_pin, HIGH);
      return 0;
    }
  } else {
    // I2C
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->endTransmission();
    _wire->requestFrom(_i2c_addr, (uint8_t)1);
    return _wire->read();
  }
}

void MCP23XXX::writeRegister(uint8_t reg, uint8_t value) {
  if (_use_spi) {
    digitalWrite(_cs_pin, LOW);
    
    if (_spi) {
      // Hardware SPI
      _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
      _spi->transfer(MCP23XXX_SPI_WRITE | (hw_addr << 1));
      _spi->transfer(reg);
      _spi->transfer(value);
      _spi->endTransaction();
    } else {
      // Software SPI implementation would go here
    }
    digitalWrite(_cs_pin, HIGH);
  } else {
    // I2C
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->write(value);
    _wire->endTransmission();
  }
}

uint16_t MCP23XXX::readRegister16(uint8_t reg) {
  if (pinCount > 8) {
    // MCP23X17 - read both ports
    if (_use_spi) {
      digitalWrite(_cs_pin, LOW);
      _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
      _spi->transfer(MCP23XXX_SPI_READ | (hw_addr << 1));
      _spi->transfer(reg);
      uint16_t value = _spi->transfer(0);
      value |= (uint16_t)_spi->transfer(0) << 8;
      _spi->endTransaction();
      digitalWrite(_cs_pin, HIGH);
      return value;
    } else {
      // I2C
      _wire->beginTransmission(_i2c_addr);
      _wire->write(reg);
      _wire->endTransmission();
      _wire->requestFrom(_i2c_addr, (uint8_t)2);
      uint16_t value = _wire->read();
      value |= (uint16_t)_wire->read() << 8;
      return value;
    }
  }
  return readRegister(reg);
}

void MCP23XXX::writeRegister16(uint8_t reg, uint16_t value) {
  if (pinCount > 8) {
    // MCP23X17 - write both ports
    if (_use_spi) {
      digitalWrite(_cs_pin, LOW);
      _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
      _spi->transfer(MCP23XXX_SPI_WRITE | (hw_addr << 1));
      _spi->transfer(reg);
      _spi->transfer(value & 0xFF);
      _spi->transfer(value >> 8);
      _spi->endTransaction();
      digitalWrite(_cs_pin, HIGH);
    } else {
      // I2C
      _wire->beginTransmission(_i2c_addr);
      _wire->write(reg);
      _wire->write(value & 0xFF);
      _wire->write(value >> 8);
      _wire->endTransmission();
    }
  } else {
    writeRegister(reg, value & 0xFF);
  }
}

uint8_t MCP23XXX::readRegister(uint8_t reg, uint8_t port) {
  uint16_t actualReg = getRegister(reg, port);
  if (_use_spi) {
    return readRegister(actualReg & 0xFF);
  } else {
    return readRegister(actualReg);
  }
}

void MCP23XXX::writeRegister(uint8_t reg, uint8_t value, uint8_t port) {
  uint16_t actualReg = getRegister(reg, port);
  if (_use_spi) {
    writeRegister(actualReg & 0xFF, value);
  } else {
    writeRegister(actualReg, value);
  }
}

void MCP23XXX::pinMode(uint8_t pin, uint8_t mode) {
  uint8_t port = MCP_PORT(pin);
  uint8_t bit = pin % 8;
  
  // Read current IODIR register
  uint8_t iodir = readRegister(MCP23XXX_IODIR, port);
  uint8_t gppu = readRegister(MCP23XXX_GPPU, port);
  
  // Set direction bit
  if (mode == OUTPUT) {
    iodir &= ~(1 << bit);  // Clear bit for output
  } else {
    iodir |= (1 << bit);   // Set bit for input
    
    // Set pull-up resistor
    if (mode == INPUT_PULLUP) {
      gppu |= (1 << bit);
    } else {
      gppu &= ~(1 << bit);
    }
    writeRegister(MCP23XXX_GPPU, gppu, port);
  }
  
  writeRegister(MCP23XXX_IODIR, iodir, port);
}

uint8_t MCP23XXX::digitalRead(uint8_t pin) {
  uint8_t port = MCP_PORT(pin);
  uint8_t bit = pin % 8;
  
  uint8_t value = readRegister(MCP23XXX_GPIO, port);
  return (value & (1 << bit)) ? HIGH : LOW;
}

void MCP23XXX::digitalWrite(uint8_t pin, uint8_t value) {
  uint8_t port = MCP_PORT(pin);
  uint8_t bit = pin % 8;
  
  uint8_t gpio = readRegister(MCP23XXX_GPIO, port);
  
  if (value == LOW) {
    gpio &= ~(1 << bit);
  } else {
    gpio |= (1 << bit);
  }
  
  writeRegister(MCP23XXX_GPIO, gpio, port);
}

uint8_t MCP23XXX::readGPIO(uint8_t port) { return readRegister(MCP23XXX_GPIO, port); }

void MCP23XXX::writeGPIO(uint8_t value, uint8_t port) { writeRegister(MCP23XXX_GPIO, value, port); }

void MCP23XXX::setupInterrupts(bool mirroring, bool openDrain, uint8_t polarity) {
  uint8_t iocon = readRegister(MCP23XXX_IOCON);
  
  // Set bits
  if (mirroring) iocon |= (1 << 6);
  else iocon &= ~(1 << 6);
  
  if (openDrain) iocon |= (1 << 2);
  else iocon &= ~(1 << 2);
  
  if (polarity == HIGH) iocon |= (1 << 1);
  else iocon &= ~(1 << 1);
  
  writeRegister(MCP23XXX_IOCON, iocon);
}

void MCP23XXX::setupInterruptPin(uint8_t pin, uint8_t mode) {
  uint8_t port = MCP_PORT(pin);
  uint8_t bit = pin % 8;
  
  // Enable interrupt
  uint8_t gpinten = readRegister(MCP23XXX_GPINTEN, port);
  gpinten |= (1 << bit);
  writeRegister(MCP23XXX_GPINTEN, gpinten, port);
  
  // Set interrupt control
  uint8_t intcon = readRegister(MCP23XXX_INTCON, port);
  if (mode == CHANGE) {
    intcon &= ~(1 << bit);  // Interrupt on change
  } else {
    intcon |= (1 << bit);   // Interrupt on compared to default value
    
    // Set default value
    uint8_t defval = readRegister(MCP23XXX_DEFVAL, port);
    if (mode == LOW) {
      defval |= (1 << bit);
    } else {
      defval &= ~(1 << bit);
    }
    writeRegister(MCP23XXX_DEFVAL, defval, port);
  }
  writeRegister(MCP23XXX_INTCON, intcon, port);
}

void MCP23XXX::disableInterruptPin(uint8_t pin) {
  uint8_t port = MCP_PORT(pin);
  uint8_t bit = pin % 8;
  
  uint8_t gpinten = readRegister(MCP23XXX_GPINTEN, port);
  gpinten &= ~(1 << bit);
  writeRegister(MCP23XXX_GPINTEN, gpinten, port);
}

void MCP23XXX::clearInterrupts() { getCapturedInterrupt(); }   // reading INTCAP register(s) clears interrupts

uint8_t MCP23XXX::getLastInterruptPin() {
  // Port A
  uint8_t intf = readRegister(MCP23XXX_INTF, 0);
  for (uint8_t pin = 0; pin < 8; pin++) {
    if (intf & (1 << pin)) {
      return pin;
    }
  }

  // Port B ?
  if (pinCount > 8) {
    intf = readRegister(MCP23XXX_INTF, 1);
    for (uint8_t pin = 0; pin < 8; pin++) {
      if (intf & (1 << pin)) {
        return pin + 8;
      }
    }
  }

  return MCP23XXX_INT_ERR;
}

uint16_t MCP23XXX::getCapturedInterrupt() {
  uint16_t intcap = readRegister(MCP23XXX_INTCAP, 0);
  
  if (pinCount > 8) {
    intcap |= (uint16_t)readRegister(MCP23XXX_INTCAP, 1) << 8;
  }
  
  return intcap;
}

uint16_t MCP23XXX::getRegister(uint8_t baseAddress, uint8_t port) {
  // MCP23x08
  uint16_t reg = baseAddress;
  // MCP23x17 BANK=0
  if (pinCount > 8) {
    reg <<= 1;
    reg |= port;
  }
  return reg;
}

MCP23X08::MCP23X08() { pinCount = 8; }

void MCP23X08::enableAddrPins() {
  if (!_use_spi) // I2C dev always use addr, only makes sense for SPI dev
    return;

  uint8_t iocon = readRegister(MCP23XXX_IOCON);
  iocon |= (1 << 3); // Set HAEN bit
  writeRegister(MCP23XXX_IOCON, iocon);
}

MCP23X17::MCP23X17() { pinCount = 16; }

uint8_t MCP23X17::readGPIOA() { return readGPIO(0); }

void MCP23X17::writeGPIOA(uint8_t value) { writeGPIO(value, 0); }

uint8_t MCP23X17::readGPIOB() { return readGPIO(1); }

void MCP23X17::writeGPIOB(uint8_t value) { writeGPIO(value, 1); }

uint16_t MCP23X17::readGPIOAB() { return readRegister16(MCP23XXX_GPIO); }

void MCP23X17::writeGPIOAB(uint16_t value) { writeRegister16(MCP23XXX_GPIO, value);}

void MCP23X17::enableAddrPins() {
  if (!_use_spi) // I2C dev always use addr, only makes sense for SPI dev
    return;

  // Send message to address 0b000 regardless of chip addr
  uint8_t tmp = this->hw_addr;
  this->hw_addr = 0; // Temporary set hw addr to 0
  
  uint8_t iocon = readRegister(MCP23XXX_IOCON);
  iocon |= (1 << 3); // Set HAEN bit
  writeRegister(MCP23XXX_IOCON, iocon);
  
  this->hw_addr = tmp;
  
  // Now set for actual address
  iocon = readRegister(MCP23XXX_IOCON);
  iocon |= (1 << 3); // Set HAEN bit
  writeRegister(MCP23XXX_IOCON, iocon);
}
