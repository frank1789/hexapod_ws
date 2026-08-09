/**
 * @file i2c-peripheral.cc
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

#include "i2c-peripheral.h"

extern "C" {
#include <fcntl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
}

#include <cerrno>
#include <chrono>
#include <rclcpp/logger.hpp>
#include <rclcpp/logging.hpp>
#include <system_error>
#include <thread>
#include <utility>

namespace {

/** @brief Logger shared by every peripheral. */
rclcpp::Logger Log() { return rclcpp::get_logger("i2c"); }

/** @brief Mask of the byte the SMBus helper returns. */
constexpr int kByteMask{0xFF};

/** @brief Pause between two attempts at the same transfer. */
constexpr std::chrono::milliseconds kRetryDelay{2};

/**
 * @brief Tell whether an error is worth retrying.
 *
 * A busy or interrupted bus usually succeeds on the next attempt; a missing
 * device or a permission problem never will, and retrying only hides it.
 */
bool IsTransient(const int t_error) {
  return t_error == EAGAIN || t_error == EINTR || t_error == EBUSY || t_error == ETIMEDOUT;
}

}  // namespace

I2cPeripheral::I2cPeripheral(const std::string& t_device, const std::uint8_t t_address)
    : device_(t_device), address_(t_address) {
  OpenBus(t_device);
  ConnectToPeripheral(t_address);
  RCLCPP_DEBUG(Log(), "opened %s for device 0x%02X", t_device.c_str(), static_cast<unsigned>(t_address));
}

I2cPeripheral::I2cPeripheral(I2cPeripheral&& other) noexcept
    : device_(std::move(other.device_)), bus_fd_(std::exchange(other.bus_fd_, kClosed)), address_(other.address_) {}

I2cPeripheral& I2cPeripheral::operator=(I2cPeripheral&& other) noexcept {
  if (this != &other) {
    if (bus_fd_ != kClosed) {
      close(bus_fd_);
    }
    bus_fd_ = std::exchange(other.bus_fd_, kClosed);
    device_ = std::move(other.device_);
    address_ = other.address_;
  }
  return *this;
}

I2cPeripheral::~I2cPeripheral() {
  // Guarded: a default constructed object never opened anything, and closing an
  // uninitialised descriptor would close whatever file happens to hold it.
  if (bus_fd_ != kClosed) {
    close(bus_fd_);
  }
}

bool I2cPeripheral::IsOpen() const noexcept { return bus_fd_ != kClosed; }

void I2cPeripheral::WriteRegisterByte(const std::uint8_t t_register_address, const std::uint8_t t_value) {
  i2c_smbus_data data{};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) kernel ABI
  data.byte = t_value;

  for (auto attempt = 1; attempt <= kMaxAttempts; ++attempt) {
    errno = 0;
    const auto failed = i2c_smbus_access(bus_fd_, I2C_SMBUS_WRITE, t_register_address, I2C_SMBUS_BYTE_DATA, &data);
    if (failed == 0) {
      return;
    }

    const auto saved_errno = errno;
    if (!IsTransient(saved_errno) || attempt == kMaxAttempts) {
      const auto message = "Could not write value (" + std::to_string(t_value) + ") to register " +
                           std::to_string(t_register_address) + " of device " + std::to_string(address_) + " on " +
                           device_;
      throw std::system_error(saved_errno, std::system_category(), message);
    }

    RCLCPP_WARN(Log(), "write to register %u failed (attempt %d of %d), retrying",
                static_cast<unsigned>(t_register_address), attempt, kMaxAttempts);
    std::this_thread::sleep_for(kRetryDelay);
  }
}

std::uint8_t I2cPeripheral::ReadRegisterByte(const std::uint8_t t_register_address) {
  i2c_smbus_data data{};

  for (auto attempt = 1; attempt <= kMaxAttempts; ++attempt) {
    errno = 0;
    const auto failed = i2c_smbus_access(bus_fd_, I2C_SMBUS_READ, t_register_address, I2C_SMBUS_BYTE_DATA, &data);
    if (failed == 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) kernel ABI
      return data.byte & kByteMask;
    }

    // errno, not the return value: i2c_smbus_access reports -1 and leaves the
    // reason in errno, so the previous code built its error from the constant 1.
    const auto saved_errno = errno;
    if (!IsTransient(saved_errno) || attempt == kMaxAttempts) {
      const auto message = "Could not read register " + std::to_string(t_register_address) + " of device " +
                           std::to_string(address_) + " on " + device_;
      throw std::system_error(saved_errno, std::system_category(), message);
    }

    RCLCPP_WARN(Log(), "read of register %u failed (attempt %d of %d), retrying",
                static_cast<unsigned>(t_register_address), attempt, kMaxAttempts);
    std::this_thread::sleep_for(kRetryDelay);
  }

  return 0;  // unreachable: the loop either returns or throws.
}

void I2cPeripheral::OpenBus(const std::string& t_device) {
  bus_fd_ = open(t_device.c_str(), O_RDWR);
  if (bus_fd_ < 0) {
    bus_fd_ = kClosed;
    throw std::system_error(
        errno, std::system_category(),
        "Could not open i2c bus " + t_device + " (is the I2C interface enabled and is the user in the i2c group?)");
  }
}

void I2cPeripheral::ConnectToPeripheral(const std::uint8_t t_address) {
  if (ioctl(bus_fd_, I2C_SLAVE, t_address) < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Could not select device " + std::to_string(t_address) + " on " + device_);
  }
}
