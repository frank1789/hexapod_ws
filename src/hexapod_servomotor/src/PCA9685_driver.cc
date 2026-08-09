/**
 * @file PCA9685_driver.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Driver for the Adafruit 16-channel 12-bit PWM/servo board (NXP PCA9685).
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

#include "PCA9685_driver.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <rclcpp/logger.hpp>
#include <rclcpp/logging.hpp>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace adafruit {

namespace {

/** @brief Logger shared by every board, so the source is obvious in the log. */
rclcpp::Logger Log() { return rclcpp::get_logger("pca9685"); }

/** @brief Format a byte as 0xNN, because register values only read well in hex. */
std::string Hex(std::uint8_t t_value) {
  std::ostringstream stream;
  stream << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(t_value);
  return stream.str();
}

}  // namespace

PCA9685::PCA9685(const std::string& t_device, const std::uint8_t t_address) { Initialize(t_device, t_address); }

PCA9685::~PCA9685() {
  if (m_i2c_device == nullptr) {
    return;
  }

  // A destructor must not throw, but leaving the servos driven would be worse
  // than a noisy log line: report and carry on.
  try {
    AllOutputsOff();
    Sleep();
    RCLCPP_INFO(Log(), "board %s on %s: outputs off, device asleep", Hex(m_address).c_str(), m_device_path.c_str());
  } catch (const std::exception& error) {
    RCLCPP_ERROR(Log(), "board %s on %s: could not park the outputs: %s", Hex(m_address).c_str(), m_device_path.c_str(),
                 error.what());
  }
}

void PCA9685::Initialize(const std::string& t_device, const std::uint8_t t_address) {
  if (t_address < pca9685::kAddressMin || t_address > pca9685::kAddressMax) {
    throw std::invalid_argument("PCA9685 address " + Hex(t_address) + " is outside the range " +
                                Hex(pca9685::kAddressMin) + " to " + Hex(pca9685::kAddressMax));
  }

  RCLCPP_INFO(Log(), "opening board %s on %s", Hex(t_address).c_str(), t_device.c_str());

  m_device_path = t_device;
  m_address = t_address;
  m_i2c_device = std::make_unique<i2cPeripheral>(t_device, t_address);

  // Park the outputs before anything else, so a board that was left driving
  // servos by a previous run does not twitch while it is being configured.
  SetAllPWM(0, 0);

  m_i2c_device->WriteRegisterByte(pca9685::kMode2, pca9685::kMode2TotemPole);
  m_i2c_device->WriteRegisterByte(pca9685::kMode1, pca9685::kMode1AllCall);
  std::this_thread::sleep_for(pca9685::kOscillatorStartUp);

  Wake();

  // Read back what the board is actually doing rather than assuming a default.
  m_prescale = m_i2c_device->ReadRegisterByte(pca9685::kPreScale);
  m_frequency = pca9685::FrequencyFromPrescale(m_prescale);

  RCLCPP_INFO(Log(), "board %s ready: prescale %u, output frequency %.2f Hz", Hex(m_address).c_str(),
              static_cast<unsigned>(m_prescale), m_frequency);
}

bool PCA9685::IsInitialised() const noexcept { return m_i2c_device != nullptr; }

void PCA9685::EnsureInitialised(const char* t_operation) const {
  if (m_i2c_device == nullptr) {
    throw std::logic_error(std::string("PCA9685: ") + t_operation + " requested before Initialize()");
  }
}

void PCA9685::EnsureValidChannel(const int t_channel) {
  if (t_channel < 0 || t_channel >= pca9685::kChannelCount) {
    throw std::invalid_argument("PCA9685 channel " + std::to_string(t_channel) + " is outside 0 to " +
                                std::to_string(pca9685::kChannelCount - 1));
  }
}

void PCA9685::EnsureValidCounter(const std::uint16_t t_value, const char* t_name) {
  if (t_value > pca9685::kCounterMax) {
    // Anything above the counter maximum sets bit 12, which is the full-ON or
    // full-OFF flag: the output would latch instead of modulating.
    throw std::invalid_argument(std::string("PCA9685 ") + t_name + " value " + std::to_string(t_value) +
                                " exceeds the 12-bit counter maximum " + std::to_string(pca9685::kCounterMax));
  }
}

std::uint8_t PCA9685::ChannelRegister(const int t_channel) noexcept {
  return static_cast<std::uint8_t>(pca9685::kLed0OnLow + pca9685::kRegistersPerChannel * t_channel);
}

void PCA9685::SetPWMFrequency(const double t_freq) {
  EnsureInitialised("SetPWMFrequency");

  if (!(t_freq >= pca9685::kFrequencyMinHz) || !(t_freq <= pca9685::kFrequencyMaxHz)) {
    // The comparison is negated so that a NaN request is rejected too.
    throw std::invalid_argument("PCA9685: requested frequency " + std::to_string(t_freq) + " Hz is outside the range " +
                                std::to_string(pca9685::kFrequencyMinHz) + " to " +
                                std::to_string(pca9685::kFrequencyMaxHz) + " Hz");
  }

  auto prescale = pca9685::PrescaleFromFrequency(t_freq);
  if (prescale < pca9685::kPrescaleMin) {
    RCLCPP_WARN(Log(), "prescale %u is below the hardware minimum %u, clamping", static_cast<unsigned>(prescale),
                static_cast<unsigned>(pca9685::kPrescaleMin));
    prescale = pca9685::kPrescaleMin;
  }

  // PRE_SCALE only accepts a write while the oscillator is stopped.
  const auto previous_mode = m_i2c_device->ReadRegisterByte(pca9685::kMode1);
  const auto sleep_mode = static_cast<std::uint8_t>((previous_mode & ~pca9685::kMode1Restart) | pca9685::kMode1Sleep);

  m_i2c_device->WriteRegisterByte(pca9685::kMode1, sleep_mode);
  m_i2c_device->WriteRegisterByte(pca9685::kPreScale, prescale);
  m_i2c_device->WriteRegisterByte(pca9685::kMode1, previous_mode);

  // The data sheet requires SLEEP to be low for at least 500 us before RESTART.
  std::this_thread::sleep_for(pca9685::kOscillatorStartUp);
  m_i2c_device->WriteRegisterByte(pca9685::kMode1, static_cast<std::uint8_t>(previous_mode | pca9685::kMode1Restart));

  m_prescale = prescale;
  m_frequency = pca9685::FrequencyFromPrescale(prescale);

  if (std::abs(m_frequency - t_freq) > 0.5) {
    // The prescaler is an integer, so the request is rarely met exactly. Say so:
    // a pulse width computed from the requested value would be slightly wrong.
    RCLCPP_WARN(Log(), "board %s: requested %.2f Hz, prescale %u gives %.2f Hz", Hex(m_address).c_str(), t_freq,
                static_cast<unsigned>(prescale), m_frequency);
  } else {
    RCLCPP_INFO(Log(), "board %s: output frequency %.2f Hz (prescale %u)", Hex(m_address).c_str(), m_frequency,
                static_cast<unsigned>(prescale));
  }
}

double PCA9685::GetActualFrequency() const noexcept { return m_frequency; }

void PCA9685::SetAllPWM(const std::uint16_t t_on, const std::uint16_t t_off) {
  EnsureInitialised("SetAllPWM");
  EnsureValidCounter(t_on, "ON");
  EnsureValidCounter(t_off, "OFF");

  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOnLow, t_on & 0xFF);
  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOnHigh, t_on >> 8);
  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOffLow, t_off & 0xFF);
  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOffHigh, t_off >> 8);
}

void PCA9685::SetSinglePWM(const int t_channel, const std::uint16_t t_on, const std::uint16_t t_off) {
  EnsureInitialised("SetSinglePWM");
  EnsureValidChannel(t_channel);
  EnsureValidCounter(t_on, "ON");
  EnsureValidCounter(t_off, "OFF");

  const auto base = ChannelRegister(t_channel);
  m_i2c_device->WriteRegisterByte(base + 0, t_on & 0xFF);
  m_i2c_device->WriteRegisterByte(base + 1, t_on >> 8);
  m_i2c_device->WriteRegisterByte(base + 2, t_off & 0xFF);
  m_i2c_device->WriteRegisterByte(base + 3, t_off >> 8);

  RCLCPP_DEBUG(Log(), "board %s channel %d: on %u, off %u", Hex(m_address).c_str(), t_channel,
               static_cast<unsigned>(t_on), static_cast<unsigned>(t_off));
}

void PCA9685::SetPWMms(const int t_channel, const double t_milliseconds) {
  EnsureInitialised("SetPWMms");
  EnsureValidChannel(t_channel);

  const auto period_ms = 1000.0 / m_frequency;
  if (t_milliseconds < 0.0 || t_milliseconds > period_ms) {
    throw std::invalid_argument("PCA9685: a pulse of " + std::to_string(t_milliseconds) + " ms does not fit in the " +
                                std::to_string(period_ms) + " ms period");
  }

  const auto ticks = std::lround(t_milliseconds * (pca9685::kCounterMax + 1) / period_ms);
  SetSinglePWM(t_channel, 0, static_cast<std::uint16_t>(std::min<long>(ticks, pca9685::kCounterMax)));
}

void PCA9685::SetChannelOff(const int t_channel) {
  EnsureInitialised("SetChannelOff");
  EnsureValidChannel(t_channel);

  const auto base = ChannelRegister(t_channel);
  m_i2c_device->WriteRegisterByte(base + 0, 0);
  m_i2c_device->WriteRegisterByte(base + 1, 0);
  m_i2c_device->WriteRegisterByte(base + 2, 0);
  // Bit 4 of the OFF high byte is the full-OFF flag.
  m_i2c_device->WriteRegisterByte(base + 3, pca9685::kFullOnOffBit >> 8);
}

void PCA9685::AllOutputsOff() {
  EnsureInitialised("AllOutputsOff");

  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOnLow, 0);
  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOnHigh, 0);
  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOffLow, 0);
  m_i2c_device->WriteRegisterByte(pca9685::kAllLedOffHigh, pca9685::kFullOnOffBit >> 8);
}

void PCA9685::Sleep() {
  EnsureInitialised("Sleep");

  const auto mode = m_i2c_device->ReadRegisterByte(pca9685::kMode1);
  m_i2c_device->WriteRegisterByte(pca9685::kMode1, static_cast<std::uint8_t>(mode | pca9685::kMode1Sleep));
}

void PCA9685::Wake() {
  EnsureInitialised("Wake");

  const auto mode = m_i2c_device->ReadRegisterByte(pca9685::kMode1);
  m_i2c_device->WriteRegisterByte(pca9685::kMode1, static_cast<std::uint8_t>(mode & ~pca9685::kMode1Sleep));
  std::this_thread::sleep_for(pca9685::kOscillatorStartUp);
}

}  // namespace adafruit
