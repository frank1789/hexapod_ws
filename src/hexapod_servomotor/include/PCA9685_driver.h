/**
 * @file PCA9685_driver.h
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

#ifndef PCA9685_DRIVER_H_
#define PCA9685_DRIVER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "PCA9685_register.h"
#include "i2c-peripheral.h"

namespace adafruit {

/**
 * @brief Controls one PCA9685 board over I2C.
 *
 * The device drives sixteen independent PWM channels from a single 12-bit
 * counter clocked by a 25 MHz oscillator. Every channel has an ON and an OFF
 * comparator value, so both the duty cycle and the phase are programmable.
 *
 * Construction is deliberately split in two: the default constructor does not
 * touch the bus, so a board can be a member of a class that reads its
 * configuration before opening the hardware. Nothing but @ref Initialize may be
 * called first; every other method throws while the driver is uninitialised.
 *
 * @warning The outputs move real servos. The destructor switches every channel
 * to full-OFF and puts the device to sleep, so that a crashing or exiting
 * process leaves the joints unpowered rather than holding their last command.
 *
 * @note The class is not thread safe. Serialise access if more than one thread
 * drives the same board.
 */
class PCA9685 {
 public:
  /**
   * @brief Construct a driver that is not attached to any device yet.
   *
   * Call @ref Initialize before any other method.
   */
  PCA9685() noexcept = default;

  /**
   * @brief Construct the driver and open the device straight away.
   *
   * @param t_device path of the I2C bus, for instance `/dev/i2c-1`
   * @param t_address 7-bit address of the board, 0x40 to 0x7F
   *
   * @throws std::invalid_argument if the address is outside the range the
   * device can be strapped to.
   * @throws std::system_error if the bus cannot be opened or the device does
   * not answer.
   */
  PCA9685(const std::string& t_device, std::uint8_t t_address);

  PCA9685(const PCA9685&) = delete;
  PCA9685& operator=(const PCA9685&) = delete;
  PCA9685(PCA9685&&) noexcept = default;
  PCA9685& operator=(PCA9685&&) noexcept = default;

  /**
   * @brief Switch every output off, send the device to sleep and close the bus.
   *
   * Errors are logged and swallowed: a destructor must not throw.
   */
  ~PCA9685();

  /**
   * @brief Open the bus, reset the device and leave it awake and idle.
   *
   * The sequence follows the data sheet: all channels are cleared, the outputs
   * are configured as totem pole, the all-call address is enabled, and SLEEP is
   * cleared before the oscillator is given time to stabilise.
   *
   * @param t_device path of the I2C bus, for instance `/dev/i2c-1`
   * @param t_address 7-bit address of the board, 0x40 to 0x7F
   *
   * @throws std::invalid_argument if the address is out of range.
   * @throws std::system_error if the bus cannot be opened or the device does
   * not answer.
   */
  void Initialize(const std::string& t_device, std::uint8_t t_address);

  /**
   * @brief Tell whether @ref Initialize has completed successfully.
   */
  [[nodiscard]] bool IsInitialised() const noexcept;

  /**
   * @brief Programme the frequency shared by all sixteen outputs.
   *
   * The prescaler is an integer, so the frequency actually produced is rarely
   * the one requested. Use @ref GetActualFrequency to read back what the device
   * settled on; the difference matters when a pulse width is converted into
   * counter ticks.
   *
   * PRE_SCALE can only be written while asleep, so the device is put to sleep
   * and woken again, which briefly interrupts the outputs.
   *
   * @param t_freq requested frequency in hertz, 24 Hz to 1526 Hz
   *
   * @throws std::invalid_argument if the frequency is outside that range.
   * @throws std::logic_error if the driver is not initialised.
   * @throws std::system_error on a bus failure.
   */
  void SetPWMFrequency(double t_freq);

  /**
   * @brief Frequency the device actually produces, in hertz.
   *
   * Computed back from the integer prescaler that was written.
   */
  [[nodiscard]] double GetActualFrequency() const noexcept;

  /**
   * @brief Drive every channel with the same ON and OFF counter values.
   *
   * @param t_on counter value at which the outputs go high, 0 to 4095
   * @param t_off counter value at which they go low, 0 to 4095
   *
   * @throws std::invalid_argument if a value exceeds the counter range.
   * @throws std::logic_error if the driver is not initialised.
   */
  void SetAllPWM(std::uint16_t t_on, std::uint16_t t_off);

  /**
   * @brief Drive one channel.
   *
   * @param t_channel channel index, 0 to 15
   * @param t_on counter value at which the output goes high, 0 to 4095
   * @param t_off counter value at which it goes low, 0 to 4095
   *
   * @throws std::invalid_argument if the channel or a counter value is out of
   * range. Without this check the register arithmetic would silently write into
   * the registers of another channel.
   * @throws std::logic_error if the driver is not initialised.
   */
  void SetSinglePWM(int t_channel, std::uint16_t t_on, std::uint16_t t_off);

  /**
   * @brief Drive one channel with a pulse expressed in milliseconds.
   *
   * The conversion uses @ref GetActualFrequency, so it stays correct even when
   * the prescaler rounded the requested frequency.
   *
   * @param t_channel channel index, 0 to 15
   * @param t_milliseconds pulse width; it must fit inside one period
   *
   * @throws std::invalid_argument if the pulse does not fit in one period.
   */
  void SetPWMms(int t_channel, double t_milliseconds);

  /**
   * @brief Switch one channel off through the full-OFF bit.
   *
   * @param t_channel channel index, 0 to 15
   */
  void SetChannelOff(int t_channel);

  /**
   * @brief Switch every output off through the full-OFF bit.
   *
   * The quickest way to make the machine safe without closing the bus.
   */
  void AllOutputsOff();

  /**
   * @brief Send the device to sleep, stopping the oscillator.
   *
   * The outputs stop being refreshed, so switch them off first.
   */
  void Sleep();

  /**
   * @brief Wake the device and restart the channels that were active.
   */
  void Wake();

  /**
   * @brief Throw when a channel index is outside 0 to 15.
   *
   * Public and static because it is a pure check on an argument: it can be
   * exercised, and relied upon, without a board on the bus.
   *
   * @param t_channel channel index to check
   * @throws std::invalid_argument if the index is out of range.
   */
  static void EnsureValidChannel(int t_channel);

  /**
   * @brief Throw when a counter value does not fit in the 12-bit range.
   *
   * @param t_value value to check
   * @param t_name name used in the message, for instance "ON"
   * @throws std::invalid_argument if the value exceeds 4095, which would set
   * the full-ON or full-OFF bit instead of a duty cycle.
   */
  static void EnsureValidCounter(std::uint16_t t_value, const char* t_name);

 private:
  /** @brief Throw when a method is used before @ref Initialize. */
  void EnsureInitialised(const char* t_operation) const;

  /** @brief Address of the first of the four registers of a channel. */
  [[nodiscard]] static std::uint8_t ChannelRegister(int t_channel) noexcept;

  std::unique_ptr<i2cPeripheral> m_i2c_device{nullptr};
  std::string m_device_path{};
  std::uint8_t m_address{0};
  double m_frequency{50.0};
  std::uint8_t m_prescale{0};
};

}  // namespace adafruit

#endif  // PCA9685_DRIVER_H_
