/**
 * @file servocontroller.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief ROS 2 node driving the eighteen hexapod servos through two PCA9685 boards.
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

#ifndef HEXAPOD_SERVO_CONTROLLER_H_
#define HEXAPOD_SERVO_CONTROLLER_H_

#include <chrono>
#include <cstdint>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <vector>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "PCA9685_driver.h"
#include "motor.h"

namespace hexapod {

/**
 * @brief Drives the hexapod joints.
 *
 * The robot has six legs of three joints each. Every joint is a hobby servo
 * wired to one of two Adafruit PCA9685 boards sharing the same I2C bus: the
 * board at `left_driver_address` carries the motors whose name begins with `L`,
 * the one at `right_driver_address` carries the rest.
 *
 * Which motors exist, and where they rest, is not compiled in. Two Lua scripts
 * in the `config` directory of the installed package answer those questions:
 *
 * - `motors.lua` defines `generate_motors_configuration(count, side)` and fills
 *   a global `Motors` table with `{name, pin}` pairs;
 * - `homing.lua` defines `homing(name)` returning the rest angle in degrees.
 *
 * Editing either one changes the robot without a rebuild.
 *
 * @warning Startup sweeps every joint over its whole travel only when
 * `perform_startup_test` is enabled. It is off by default: an unexpected sweep
 * on a robot standing on its legs makes it fall over, and a joint that is
 * mechanically blocked stalls its servo.
 */
class ServoController : public rclcpp::Node {
 public:
  /**
   * @brief Construct the node, open both boards and move the joints to rest.
   *
   * Reads the parameters, opens the two PCA9685 boards, registers the motors
   * described by `motors.lua` and applies the homing position.
   *
   * @throws std::runtime_error if a Lua script is missing, fails to run or
   * describes a motor the homing table does not cover.
   * @throws std::invalid_argument if a parameter is out of range.
   * @throws std::system_error if the I2C bus cannot be opened.
   */
  ServoController();

  /** @brief Destroy the node; the boards park their outputs on the way out. */
  ~ServoController() override;

  /**
   * @brief Move every motor to the angle `homing.lua` gives for its name.
   *
   * The script is reloaded on every call, so the rest position can be retuned
   * while the node is running.
   *
   * @throws std::runtime_error if the script cannot be loaded or leaves a
   * motor without an angle.
   */
  void RestoreDefaultPosition();

  /**
   * @brief Sweep each motor across its full travel, one motor at a time.
   *
   * Intended for bench testing a freshly wired robot.
   *
   * @warning Only run this with the robot supported off the ground.
   */
  void PerformTest();

  /**
   * @brief Tell whether the startup sweep was requested by parameter.
   */
  [[nodiscard]] bool StartupTestRequested() const noexcept;

 private:
  /**
   * @brief Read and validate every node parameter.
   *
   * @throws std::invalid_argument if a value cannot be used.
   */
  void DeclareParameters();

  /** @brief Open both boards and apply the PWM frequency. */
  void OpenDrivers();

  /** @brief Build the motor table by running `motors.lua`. */
  void RegisterMotors();

  /** @brief Send one motor its current angle. */
  void WriteOnMotor(const Motor& t_motor);

  /**
   * @brief Convert an angle into a value of the 12-bit PWM counter.
   *
   * The angle is clamped to `[0, 180]` degrees and mapped onto the configured
   * pulse width range, then expressed in counter ticks using the frequency the
   * board actually produces, which the integer prescaler rarely makes equal to
   * the requested one.
   *
   * @param t_angle angle in degrees
   * @return counter value at which the output must go low
   */
  [[nodiscard]] std::uint16_t PulseWidth(double t_angle) const;

  /**
   * @brief Load a Lua script from the package share directory and return one of
   * its global functions.
   *
   * @param t_filename script file name, relative to `config/`
   * @param t_entry_point name of the global function to return
   * @return the requested function
   *
   * @throws std::runtime_error if the script cannot be loaded or executed, or
   * does not define the requested function.
   */
  sol::protected_function LoadScript(const std::string& t_filename, const std::string& t_entry_point);

  /** @brief Board driving the motors whose name starts with `L`. */
  adafruit::PCA9685 servo_driver_left_;

  /** @brief Board driving every other motor. */
  adafruit::PCA9685 servo_driver_right_;

  // Declared largest first so the node carries no avoidable padding: the two
  // boards, then the interpreter, the strings, the vector, and finally the
  // scalars in decreasing width.
  sol::state lua_;                           /**< Lua interpreter used for the configuration. */
  std::string config_directory_;             /**< Where the Lua scripts were installed. */
  std::string i2c_bus_;                      /**< Path of the I2C bus. */
  std::string motors_script_;                /**< File name of the motor table script. */
  std::string homing_script_;                /**< File name of the rest position script. */
  std::vector<Motor> motors_;                /**< Motors described by the motor table script. */
  double frequency_{0.0};                    /**< Requested PWM frequency, in hertz. */
  double min_pulse_us_{0.0};                 /**< Pulse width for 0 degrees, in microseconds. */
  double max_pulse_us_{0.0};                 /**< Pulse width for 180 degrees, in microseconds. */
  std::chrono::milliseconds settle_time_{0}; /**< Pause after each servo command. */
  int motors_per_side_{0};                   /**< Motors generated for each side of the robot. */
  std::uint8_t left_address_{0};             /**< Address of the left board. */
  std::uint8_t right_address_{0};            /**< Address of the right board. */
  bool startup_test_{false};                 /**< Whether to sweep the joints at startup. */
};

}  // namespace hexapod

#endif  // HEXAPOD_SERVO_CONTROLLER_H_
