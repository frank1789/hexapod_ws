/**
 * @file servocontroller.cc
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

#include "servocontroller.h"

#include <algorithm>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <cmath>
#include <stdexcept>
#include <thread>

#include "PCA9685_register.h"
#include "utility_function.h"

namespace hexapod {

namespace {

constexpr auto kDefaultPWMFreq{50.0};   /**< Refresh rate expected by hobby servos. */
constexpr int kReservedMotors{32};      /**< Reserved space for motors. */
constexpr int kDefaultMotorsPerSide{9}; /**< Three legs per side, three joints per leg. */
constexpr int kDefaultSettleMs{50};     /**< Time given to a servo to reach its target. */
constexpr int kTestStepDegree{5};       /**< Sweep granularity of the startup test. */

}  // namespace

ServoController::ServoController() : rclcpp::Node("servomotors_node") {
  m_motors.reserve(kReservedMotors);
  lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::debug);

  m_config_directory = ament_index_cpp::get_package_share_directory("hexapod_servomotor") + "/config";
  RCLCPP_INFO_STREAM(get_logger(), "configuration directory: " << m_config_directory);

  DeclareParameters();
  OpenDrivers();
  RegisterMotors();
  RestoreDefaultPosition();

  RCLCPP_INFO(get_logger(), "servomotors node ready with %zu motors", m_motors.size());
  if (m_startup_test) {
    RCLCPP_WARN(get_logger(),
                "perform_startup_test is enabled: every joint will sweep its full travel, "
                "make sure the robot is supported off the ground");
  }
}

ServoController::~ServoController() {
  // The two drivers switch their outputs off in their own destructors; say so
  // here as well, because this is the message an operator looks for.
  RCLCPP_INFO(get_logger(), "shutting down, the servos will stop being driven");
}

void ServoController::DeclareParameters() {
  m_i2c_bus = declare_parameter<std::string>("i2c_bus", "/dev/i2c-1");

  const auto left = declare_parameter<int>("left_driver_address", 0x40);
  const auto right = declare_parameter<int>("right_driver_address", 0x41);
  if (left == right) {
    throw std::invalid_argument("left_driver_address and right_driver_address are both " + std::to_string(left) +
                                ": the two boards must be strapped to different addresses");
  }
  m_left_address = static_cast<std::uint8_t>(left);
  m_right_address = static_cast<std::uint8_t>(right);

  m_frequency = declare_parameter<double>("pwm_frequency", kDefaultPWMFreq);
  m_min_pulse_us = declare_parameter<double>("min_pulse_width_us", kDefaultMinPulseWidthUs);
  m_max_pulse_us = declare_parameter<double>("max_pulse_width_us", kDefaultMaxPulseWidthUs);
  if (m_min_pulse_us >= m_max_pulse_us) {
    throw std::invalid_argument("min_pulse_width_us (" + std::to_string(m_min_pulse_us) +
                                ") must be smaller than max_pulse_width_us (" + std::to_string(m_max_pulse_us) + ")");
  }

  // A pulse cannot outlast the period it lives in.
  const auto period_us = 1e6 / m_frequency;
  if (m_max_pulse_us >= period_us) {
    throw std::invalid_argument("max_pulse_width_us (" + std::to_string(m_max_pulse_us) + ") does not fit in the " +
                                std::to_string(period_us) + " us period of a " + std::to_string(m_frequency) +
                                " Hz signal");
  }

  m_motors_per_side = declare_parameter<int>("motors_per_side", kDefaultMotorsPerSide);
  if (m_motors_per_side <= 0 || m_motors_per_side > adafruit::pca9685::kChannelCount) {
    throw std::invalid_argument("motors_per_side must be between 1 and " +
                                std::to_string(adafruit::pca9685::kChannelCount));
  }

  const auto settle_ms = declare_parameter<int>("settle_time_ms", kDefaultSettleMs);
  if (settle_ms < 0) {
    throw std::invalid_argument("settle_time_ms cannot be negative");
  }
  m_settle_time = std::chrono::milliseconds{settle_ms};

  m_startup_test = declare_parameter<bool>("perform_startup_test", false);
  m_motors_script = declare_parameter<std::string>("motors_script", "motors.lua");
  m_homing_script = declare_parameter<std::string>("homing_script", "homing.lua");

  RCLCPP_INFO(get_logger(),
              "configuration: bus %s, boards 0x%02X and 0x%02X, %.1f Hz, pulse %.0f-%.0f us, "
              "settle %d ms",
              m_i2c_bus.c_str(), static_cast<unsigned>(m_left_address), static_cast<unsigned>(m_right_address),
              m_frequency, m_min_pulse_us, m_max_pulse_us,
              // declare_parameter<int> hands back an int64_t.
              static_cast<int>(settle_ms));
}

void ServoController::OpenDrivers() {
  m_servo_driver_left.Initialize(m_i2c_bus, m_left_address);
  m_servo_driver_right.Initialize(m_i2c_bus, m_right_address);

  m_servo_driver_left.SetPWMFrequency(m_frequency);
  m_servo_driver_right.SetPWMFrequency(m_frequency);

  const auto actual = m_servo_driver_left.GetActualFrequency();
  if (std::abs(actual - m_frequency) > 0.5) {
    RCLCPP_WARN(get_logger(), "pulse widths are computed for %.2f Hz, not the requested %.2f Hz", actual, m_frequency);
  }
}

bool ServoController::StartupTestRequested() const noexcept { return m_startup_test; }

sol::protected_function ServoController::LoadScript(const std::string& t_filename, const std::string& t_entry_point) {
  const auto path = m_config_directory + "/" + t_filename;

  auto script = lua.load_file(path);
  if (!script.valid()) {
    const sol::error err = script;
    throw std::runtime_error("cannot load Lua script \"" + path + "\": " + err.what());
  }

  const sol::protected_function_result executed = script();
  if (!executed.valid()) {
    const sol::error err = executed;
    throw std::runtime_error("cannot execute Lua script \"" + path + "\": " + err.what());
  }

  sol::protected_function entry_point = lua[t_entry_point];
  if (!entry_point.valid()) {
    throw std::runtime_error("Lua script \"" + path + "\" does not define \"" + t_entry_point + "\"");
  }

  RCLCPP_DEBUG_STREAM(get_logger(), "loaded " << path << ", entry point " << t_entry_point);
  return entry_point;
}

void ServoController::PerformTest() {
  RCLCPP_WARN(get_logger(), "starting the joint sweep over %zu motors", m_motors.size());

  for (auto& motor : m_motors) {
    RCLCPP_INFO_STREAM(get_logger(), "sweeping " << motor.GetNameMotor());
    for (auto angle = static_cast<int>(kMinAngleDegree); angle <= static_cast<int>(kMaxAngleDegree);
         angle += kTestStepDegree) {
      motor.SetAngle(angle);
      WriteOnMotor(motor);
    }
    RestoreDefaultPosition();
  }

  RCLCPP_INFO(get_logger(), "joint sweep finished");
  RestoreDefaultPosition();
}

void ServoController::RestoreDefaultPosition() {
  auto get_homing_angle = LoadScript(m_homing_script, "homing");

  for (auto& motor : m_motors) {
    const sol::protected_function_result result = get_homing_angle(motor.GetNameMotor());
    if (!result.valid()) {
      const sol::error err = result;
      throw std::runtime_error("homing() failed for motor \"" + motor.GetNameMotor() + "\": " + err.what());
    }

    const sol::optional<double> angle = result;
    if (!angle.has_value()) {
      throw std::runtime_error("\"" + m_homing_script + "\" defines no angle for motor \"" + motor.GetNameMotor() +
                               "\"");
    }

    motor.SetAngle(*angle);
    WriteOnMotor(motor);
  }

  RCLCPP_INFO(get_logger(), "all %zu motors moved to their rest position", m_motors.size());
}

void ServoController::RegisterMotors() {
  auto generator = LoadScript(m_motors_script, "generate_motors_configuration");

  for (const auto& side : {"L", "R"}) {
    const sol::protected_function_result result = generator(m_motors_per_side, side);
    if (!result.valid()) {
      const sol::error err = result;
      throw std::runtime_error(std::string("generate_motors_configuration(\"") + side + "\") failed: " + err.what());
    }
  }

  const sol::optional<sol::table> motors_table = lua["Motors"];
  if (!motors_table.has_value() || motors_table->size() == 0) {
    throw std::runtime_error("\"" + m_motors_script +
                             "\" produced no motor: the global \"Motors\" table is missing "
                             "or empty");
  }

  for (const auto& entry : *motors_table) {
    if (entry.second.get_type() != sol::type::table) {
      throw std::runtime_error("\"" + m_motors_script + "\": every entry of \"Motors\" must be a {name, pin} table");
    }

    auto motor_entry = entry.second.as<sol::table>();
    const sol::optional<std::string> name = motor_entry[1];
    const sol::optional<int> pin = motor_entry[2];
    if (!name.has_value() || !pin.has_value()) {
      throw std::runtime_error("\"" + m_motors_script + "\": an entry of \"Motors\" is not a valid {name, pin} pair");
    }

    // The pin becomes a PCA9685 channel, so it has to exist on the board.
    if (*pin < 0 || *pin >= adafruit::pca9685::kChannelCount) {
      throw std::runtime_error("\"" + m_motors_script + "\": motor \"" + *name + "\" uses pin " + std::to_string(*pin) +
                               ", outside 0 to " + std::to_string(adafruit::pca9685::kChannelCount - 1));
    }

    m_motors.emplace_back(*name, *pin);
    RCLCPP_DEBUG_STREAM(get_logger(), "registered " << m_motors.back());
  }

  RCLCPP_INFO(get_logger(), "registered %zu motors from \"%s\"", m_motors.size(), m_motors_script.c_str());
}

void ServoController::WriteOnMotor(const Motor& t_motor) {
  auto& driver = t_motor.GetNameMotor().starts_with("L") ? m_servo_driver_left : m_servo_driver_right;

  driver.SetSinglePWM(t_motor.GetPinMotor(), 0, PulseWidth(t_motor.GetAngle()));
  std::this_thread::sleep_for(m_settle_time);
}

std::uint16_t ServoController::PulseWidth(const double t_angle) const {
  const auto angle = std::clamp(t_angle, kMinAngleDegree, kMaxAngleDegree);
  if (std::abs(angle - t_angle) > 0.0) {
    RCLCPP_WARN(get_logger(), "angle %.1f degrees is outside [%.0f, %.0f], clamped to %.1f", t_angle, kMinAngleDegree,
                kMaxAngleDegree, angle);
  }

  const auto pulse_us = map(angle, kMinAngleDegree, kMaxAngleDegree, m_min_pulse_us, m_max_pulse_us);

  // Ticks of the 12-bit counter, using the frequency the board really produces.
  const auto frequency = m_servo_driver_left.GetActualFrequency();
  const auto ticks = std::lround(pulse_us * 1e-6 * frequency * (adafruit::pca9685::kCounterMax + 1));

  return static_cast<std::uint16_t>(std::clamp<long>(ticks, 0, static_cast<long>(adafruit::pca9685::kCounterMax)));
}

}  // namespace hexapod
