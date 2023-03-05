#include "i2c-peripheral.h"

extern "C" {
#include <fcntl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
}

#include <system_error>

i2cPeripheral::i2cPeripheral(const std::string& t_device, const uint8_t t_address) {
  OpenBus(t_device);
  ConnectToPeripheral(t_address);
}

i2cPeripheral::~i2cPeripheral() { close(m_bus_fd); }

void i2cPeripheral::WriteRegisterByte(const uint8_t t_register_address, const uint8_t t_value) {
  i2c_smbus_data data;
  data.byte = t_value;
  const auto err = i2c_smbus_access(m_bus_fd, I2C_SMBUS_WRITE, t_register_address, I2C_SMBUS_BYTE_DATA, &data);
  if (err) {
    const auto msg =
        "Could not write value (" + std::to_string(t_value) + ") to register " + std::to_string(t_register_address);
    throw std::system_error(errno, std::system_category(), msg);
  }
}

uint8_t i2cPeripheral::ReadRegisterByte(const uint8_t t_register_address) {
  i2c_smbus_data data;
  const auto err = i2c_smbus_access(m_bus_fd, I2C_SMBUS_READ, t_register_address, I2C_SMBUS_BYTE_DATA, &data);
  if (err) {
    const auto msg = "Could not read value at register " + std::to_string(t_register_address);
    throw std::system_error(-err, std::system_category(), msg);
  }
  return data.byte & 0xFF;
}

void i2cPeripheral::OpenBus(const std::string& t_device) {
  m_bus_fd = open(t_device.c_str(), O_RDWR);
  if (m_bus_fd < 0) {
    throw std::system_error(errno, std::system_category(), "Could not open i2c bus.");
  }
}

void i2cPeripheral::ConnectToPeripheral(const uint8_t t_address) {
  if (ioctl(m_bus_fd, I2C_SLAVE, t_address) < 0) {
    throw std::system_error(errno, std::system_category(), "Could not set peripheral address.");
  }
}
