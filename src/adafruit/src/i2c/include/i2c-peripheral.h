#ifndef SERVOMOTORS_I2C_PERIPHERAL_H_
#define SERVOMOTORS_I2C_PERIPHERAL_H_

#include <cstdint>
#include <string>

class i2cPeripheral {
 public:
  explicit i2cPeripheral() noexcept = default;
  explicit i2cPeripheral(const std::string& t_device, const uint8_t t_address);

  ~i2cPeripheral();

  void WriteRegisterByte(const uint8_t t_register_address, const uint8_t t_value);

  uint8_t ReadRegisterByte(const uint8_t t_register_address);

 private:
  void OpenBus(const std::string& t_device);
  void ConnectToPeripheral(const uint8_t t_address);

  int m_bus_fd;
};

#endif  // SERVOMOTORS_I2C_PERIPHERAL_H_
