/**
 * @file i2c-peripheral.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Thin wrapper over one device on a Linux I2C bus.
 * @version 0.2.0
 * @date 2026-08-09
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SERVOMOTORS_I2C_PERIPHERAL_H_
#define SERVOMOTORS_I2C_PERIPHERAL_H_

#include <cstdint>
#include <string>

/**
 * @brief One device on a Linux I2C bus, addressed through the SMBus helpers.
 *
 * The object owns the file descriptor of the bus and closes it on destruction.
 * Transfers are retried a few times when the kernel reports a transient error,
 * which happens on a long or noisy bus, and every permanent failure is reported
 * as a `std::system_error` carrying `errno`.
 *
 * @note Copying is disabled because the file descriptor has a single owner.
 * Moving transfers that ownership.
 */
class I2cPeripheral {
 public:
  /** @brief Number of times a transfer is retried before it is given up. */
  static constexpr int kMaxAttempts{3};

  /**
   * @brief Construct a peripheral that is not attached to any bus.
   */
  I2cPeripheral() noexcept = default;

  /**
   * @brief Open the bus and select the device.
   *
   * @param t_device path of the bus, for instance `/dev/i2c-1`
   * @param t_address 7-bit address of the device
   *
   * @throws std::system_error if the bus cannot be opened, typically because
   * the file does not exist (the I2C interface is disabled) or the user is not
   * a member of the `i2c` group.
   */
  I2cPeripheral(const std::string& t_device, std::uint8_t t_address);

  I2cPeripheral(const I2cPeripheral&) = delete;
  I2cPeripheral& operator=(const I2cPeripheral&) = delete;

  /** @brief Take over the file descriptor of @p other. */
  I2cPeripheral(I2cPeripheral&& other) noexcept;

  /** @brief Close the current descriptor and take over the one of @p other. */
  I2cPeripheral& operator=(I2cPeripheral&& other) noexcept;

  /** @brief Close the bus, if one is open. */
  ~I2cPeripheral();

  /**
   * @brief Write one byte into a register of the device.
   *
   * @param t_register_address register to write
   * @param t_value byte to store
   *
   * @throws std::system_error if the transfer keeps failing.
   */
  void WriteRegisterByte(std::uint8_t t_register_address, std::uint8_t t_value);

  /**
   * @brief Read one byte from a register of the device.
   *
   * @param t_register_address register to read
   * @return the byte the device returned
   *
   * @throws std::system_error if the transfer keeps failing.
   */
  std::uint8_t ReadRegisterByte(std::uint8_t t_register_address);

  /** @brief Tell whether a bus is currently open. */
  [[nodiscard]] bool IsOpen() const noexcept;

 private:
  void OpenBus(const std::string& t_device);
  void ConnectToPeripheral(std::uint8_t t_address);

  /** @brief An invalid descriptor, so the destructor knows there is nothing to close. */
  static constexpr int kClosed{-1};

  // Declared largest first: the previous order cost eight bytes of padding.
  std::string device_;
  int bus_fd_{kClosed};
  std::uint8_t address_{0};
};

#endif  // SERVOMOTORS_I2C_PERIPHERAL_H_
