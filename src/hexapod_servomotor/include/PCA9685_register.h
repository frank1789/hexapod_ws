/**
 * @file PCA9685_register.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Register map and hardware limits of the NXP PCA9685, the controller on
 * the Adafruit 16-channel 12-bit PWM/servo driver board.
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

#ifndef PCA9685_REGISTER_H_
#define PCA9685_REGISTER_H_

#include <chrono>
#include <cmath>
#include <cstdint>

namespace adafruit::pca9685 {

/**
 * @name Hardware characteristics
 *
 * All values are taken from the NXP PCA9685 data sheet, revision 4
 * (16-channel, 12-bit PWM Fm+ I2C-bus LED controller).
 * @{
 */

/** @brief Frequency of the internal oscillator, in hertz. */
inline constexpr double kOscillatorClockHz{25000000.0};

/** @brief Number of independent PWM outputs on the device. */
inline constexpr int kChannelCount{16};

/** @brief Largest value the 12-bit PWM counter reaches before wrapping. */
inline constexpr std::uint16_t kCounterMax{4095};

/**
 * @brief Value that sets the full-ON or full-OFF bit rather than a duty cycle.
 *
 * Bit 12 of an LEDn_ON_H or LEDn_OFF_H register drives the output permanently
 * high or low, bypassing the PWM comparator. Full-OFF takes precedence.
 */
inline constexpr std::uint16_t kFullOnOffBit{4096};

/**
 * @brief Smallest value the hardware accepts in PRE_SCALE.
 *
 * "The hardware forces a minimum value that can be loaded into the PRE_SCALE
 * register at '3'." Writing less does not lower the period any further.
 */
inline constexpr std::uint8_t kPrescaleMin{3};

/** @brief Largest value PRE_SCALE can hold, an 8-bit register. */
inline constexpr std::uint8_t kPrescaleMax{255};

/** @brief Output frequency obtained with @ref kPrescaleMax, in hertz. */
inline constexpr double kFrequencyMinHz{24.0};

/** @brief Output frequency obtained with @ref kPrescaleMin, in hertz. */
inline constexpr double kFrequencyMaxHz{1526.0};

/**
 * @brief Time the oscillator needs to stabilise after SLEEP is cleared.
 *
 * "The SLEEP bit must be logic 0 for at least 500 us, before a logic 1 is
 * written into the RESTART bit." One millisecond is used to keep a margin.
 */
inline constexpr std::chrono::microseconds kOscillatorStartUp{1000};

/** @brief Lowest I2C address the device can be strapped to. */
inline constexpr std::uint8_t kAddressMin{0x40};

/** @brief Highest I2C address the device can be strapped to. */
inline constexpr std::uint8_t kAddressMax{0x7F};

/** @} */

/**
 * @name Register addresses
 * @{
 */
inline constexpr std::uint8_t kMode1{0x00};       /**< Mode register 1. */
inline constexpr std::uint8_t kMode2{0x01};       /**< Mode register 2. */
inline constexpr std::uint8_t kSubAddr1{0x02};    /**< I2C sub-address 1. */
inline constexpr std::uint8_t kSubAddr2{0x03};    /**< I2C sub-address 2. */
inline constexpr std::uint8_t kSubAddr3{0x04};    /**< I2C sub-address 3. */
inline constexpr std::uint8_t kAllCallAddr{0x05}; /**< LED all-call address. */

inline constexpr std::uint8_t kLed0OnLow{0x06};   /**< Channel 0, ON, low byte. */
inline constexpr std::uint8_t kLed0OnHigh{0x07};  /**< Channel 0, ON, high byte. */
inline constexpr std::uint8_t kLed0OffLow{0x08};  /**< Channel 0, OFF, low byte. */
inline constexpr std::uint8_t kLed0OffHigh{0x09}; /**< Channel 0, OFF, high byte. */

/** @brief Distance in registers between one channel and the next. */
inline constexpr std::uint8_t kRegistersPerChannel{4};

inline constexpr std::uint8_t kAllLedOnLow{0xFA};   /**< Broadcast ON, low byte. */
inline constexpr std::uint8_t kAllLedOnHigh{0xFB};  /**< Broadcast ON, high byte. */
inline constexpr std::uint8_t kAllLedOffLow{0xFC};  /**< Broadcast OFF, low byte. */
inline constexpr std::uint8_t kAllLedOffHigh{0xFD}; /**< Broadcast OFF, high byte. */

inline constexpr std::uint8_t kPreScale{0xFE}; /**< Output frequency prescaler. */
/** @} */

/**
 * @name MODE1 bits
 * @{
 */
inline constexpr std::uint8_t kMode1Restart{0x80}; /**< Restart all PWM channels. */
inline constexpr std::uint8_t kMode1ExtClk{0x40};  /**< Use an external clock. */
inline constexpr std::uint8_t kMode1AutoInc{0x20}; /**< Auto-increment the register pointer. */
inline constexpr std::uint8_t kMode1Sleep{0x10};   /**< Low power mode, oscillator off. */
inline constexpr std::uint8_t kMode1Sub1{0x08};    /**< Respond to sub-address 1. */
inline constexpr std::uint8_t kMode1Sub2{0x04};    /**< Respond to sub-address 2. */
inline constexpr std::uint8_t kMode1Sub3{0x02};    /**< Respond to sub-address 3. */
inline constexpr std::uint8_t kMode1AllCall{0x01}; /**< Respond to the all-call address. */
/** @} */

/**
 * @name MODE2 bits
 * @{
 */
inline constexpr std::uint8_t kMode2Invert{0x10};            /**< Invert the output logic. */
inline constexpr std::uint8_t kMode2OutputChangeOnAck{0x08}; /**< Update outputs on ACK. */
inline constexpr std::uint8_t kMode2TotemPole{0x04};         /**< Push-pull outputs; clear means open drain. */
/** @} */

/**
 * @name Frequency conversion
 *
 * Free functions rather than driver members, so they can be exercised without
 * a board on the bus.
 * @{
 */

/**
 * @brief Prescaler that produces the requested output frequency.
 *
 * From the data sheet, equation 1:
 * `prescale = round(osc_clock / (4096 * update_rate)) - 1`.
 *
 * The result is not clamped; @ref kPrescaleMin still applies and the caller has
 * to enforce it.
 *
 * @param t_frequency requested frequency in hertz
 * @return the value to write into PRE_SCALE
 */
inline std::uint8_t PrescaleFromFrequency(const double t_frequency) {
  const auto ticks = kOscillatorClockHz / (4096.0 * t_frequency);
  return static_cast<std::uint8_t>(std::lround(ticks) - 1);
}

/**
 * @brief Output frequency a given prescaler produces, in hertz.
 *
 * The inverse of @ref PrescaleFromFrequency. Because the prescaler is an
 * integer, feeding a frequency through both functions does not return it
 * unchanged: 50 Hz becomes prescale 121, which is 50.14 Hz.
 *
 * @param t_prescale value held in PRE_SCALE
 * @return the frequency in hertz
 */
inline constexpr double FrequencyFromPrescale(const std::uint8_t t_prescale) {
  return kOscillatorClockHz / (4096.0 * (static_cast<double>(t_prescale) + 1.0));
}

/** @} */

}  // namespace adafruit::pca9685

#endif  // PCA9685_REGISTER_H_
