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
#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "PCA9685_register.h"
#include "utility_function.h"

namespace hexapod {

namespace {

constexpr auto kDefaultPWMFreq{50.0};   /**< Refresh rate expected by hobby servos. */
constexpr int kReservedMotors{32};      /**< Reserved space for motors. */
constexpr int kDefaultMotorsPerSide{9}; /**< Three legs per side, three joints per leg. */
constexpr int kDefaultSettleMs{50};     /**< Time given to a servo to reach its target. */
constexpr int kTestStepDegree{5};       /**< Sweep granularity of the startup test. */

constexpr char kDefaultCommandTopic[]{"joint_command"}; /**< Where poses arrive. */
constexpr double kDefaultWriteRateHz{10.0};             /**< How often a pose is written. */

/** @brief Only the newest pose matters, so the queue holds exactly one. */
constexpr int kCommandQueueDepth{1};

/** @brief How often a repeated complaint about a command may be logged, in ms. */
constexpr int kLogThrottleMs{2000};

}  // namespace

ServoController::ServoController() : rclcpp::Node("servomotors_node") {
  motors_.reserve(kReservedMotors);
  lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::debug);

  config_directory_ = ament_index_cpp::get_package_share_directory("hexapod_servomotor") + "/config";
  RCLCPP_INFO_STREAM(get_logger(), "configuration directory: " << config_directory_);

  DeclareParameters();
  OpenDrivers();
  RegisterMotors();
  RestoreDefaultPosition();

  // Subscribed only once the motors exist and the joints are at rest, so a pose
  // arriving during start-up cannot be written against an empty motor table.
  command_subscriber_ = create_subscription<sensor_msgs::msg::JointState>(
      command_topic_, rclcpp::QoS(kCommandQueueDepth),
      [this](const sensor_msgs::msg::JointState& command) { OnJointCommand(command); });

  const auto write_period = std::chrono::duration<double>{1.0 / get_parameter("write_rate_hz").as_double()};
  write_timer_ = create_wall_timer(std::chrono::duration_cast<std::chrono::milliseconds>(write_period),
                                   [this]() { WritePendingPose(); });

  RCLCPP_INFO(get_logger(), "listening for poses on %s", command_topic_.c_str());
  RCLCPP_INFO(get_logger(), "servomotors node ready with %zu motors", motors_.size());
  if (startup_test_) {
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
  i2c_bus_ = declare_parameter<std::string>("i2c_bus", "/dev/i2c-1");

  const auto left = declare_parameter<int>("left_driver_address", 0x40);
  const auto right = declare_parameter<int>("right_driver_address", 0x41);
  if (left == right) {
    throw std::invalid_argument("left_driver_address and right_driver_address are both " + std::to_string(left) +
                                ": the two boards must be strapped to different addresses");
  }
  left_address_ = static_cast<std::uint8_t>(left);
  right_address_ = static_cast<std::uint8_t>(right);

  frequency_ = declare_parameter<double>("pwm_frequency", kDefaultPWMFreq);
  min_pulse_us_ = declare_parameter<double>("min_pulse_width_us", kDefaultMinPulseWidthUs);
  max_pulse_us_ = declare_parameter<double>("max_pulse_width_us", kDefaultMaxPulseWidthUs);
  if (min_pulse_us_ >= max_pulse_us_) {
    throw std::invalid_argument("min_pulse_width_us (" + std::to_string(min_pulse_us_) +
                                ") must be smaller than max_pulse_width_us (" + std::to_string(max_pulse_us_) + ")");
  }

  // A pulse cannot outlast the period it lives in.
  const auto period_us = 1e6 / frequency_;
  if (max_pulse_us_ >= period_us) {
    throw std::invalid_argument("max_pulse_width_us (" + std::to_string(max_pulse_us_) + ") does not fit in the " +
                                std::to_string(period_us) + " us period of a " + std::to_string(frequency_) +
                                " Hz signal");
  }

  motors_per_side_ = declare_parameter<int>("motors_per_side", kDefaultMotorsPerSide);
  if (motors_per_side_ <= 0 || motors_per_side_ > adafruit::pca9685::kChannelCount) {
    throw std::invalid_argument("motors_per_side must be between 1 and " +
                                std::to_string(adafruit::pca9685::kChannelCount));
  }

  const auto settle_ms = declare_parameter<int>("settle_time_ms", kDefaultSettleMs);
  if (settle_ms < 0) {
    throw std::invalid_argument("settle_time_ms cannot be negative");
  }
  settle_time_ = std::chrono::milliseconds{settle_ms};

  command_topic_ = declare_parameter<std::string>("command_topic", kDefaultCommandTopic);
  if (command_topic_.empty()) {
    throw std::invalid_argument("command_topic must not be empty");
  }

  // Writing a pose costs settle_time_ms per motor, so asking for more writes
  // per second than the bus can deliver only queues work. The rate is capped
  // against the time a full pose actually takes.
  const auto write_rate_hz = declare_parameter<double>("write_rate_hz", kDefaultWriteRateHz);
  if (write_rate_hz <= 0.0) {
    throw std::invalid_argument("write_rate_hz must be greater than zero");
  }

  startup_test_ = declare_parameter<bool>("perform_startup_test", false);
  motors_script_ = declare_parameter<std::string>("motors_script", "motors.lua");
  homing_script_ = declare_parameter<std::string>("homing_script", "homing.lua");

  RCLCPP_INFO(get_logger(),
              "configuration: bus %s, boards 0x%02X and 0x%02X, %.1f Hz, pulse %.0f-%.0f us, "
              "settle %d ms",
              i2c_bus_.c_str(), static_cast<unsigned>(left_address_), static_cast<unsigned>(right_address_), frequency_,
              min_pulse_us_, max_pulse_us_,
              // declare_parameter<int> hands back an int64_t.
              static_cast<int>(settle_ms));
}

void ServoController::OpenDrivers() {
  servo_driver_left_.Initialize(i2c_bus_, left_address_);
  servo_driver_right_.Initialize(i2c_bus_, right_address_);

  servo_driver_left_.SetPWMFrequency(frequency_);
  servo_driver_right_.SetPWMFrequency(frequency_);

  const auto actual = servo_driver_left_.GetActualFrequency();
  if (std::abs(actual - frequency_) > 0.5) {
    RCLCPP_WARN(get_logger(), "pulse widths are computed for %.2f Hz, not the requested %.2f Hz", actual, frequency_);
  }
}

bool ServoController::StartupTestRequested() const noexcept { return startup_test_; }

sol::protected_function ServoController::LoadScript(const std::string& t_filename, const std::string& t_entry_point) {
  const auto path = config_directory_ + "/" + t_filename;

  auto script = lua_.load_file(path);
  if (!script.valid()) {
    const sol::error err = script;
    throw std::runtime_error("cannot load Lua script \"" + path + "\": " + err.what());
  }

  const sol::protected_function_result executed = script();
  if (!executed.valid()) {
    const sol::error err = executed;
    throw std::runtime_error("cannot execute Lua script \"" + path + "\": " + err.what());
  }

  sol::protected_function entry_point = lua_[t_entry_point];
  if (!entry_point.valid()) {
    throw std::runtime_error("Lua script \"" + path + "\" does not define \"" + t_entry_point + "\"");
  }

  RCLCPP_DEBUG_STREAM(get_logger(), "loaded " << path << ", entry point " << t_entry_point);
  return entry_point;
}

void ServoController::PerformTest() {
  RCLCPP_WARN(get_logger(), "starting the joint sweep over %zu motors", motors_.size());

  for (auto& motor : motors_) {
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
  auto get_homing_angle = LoadScript(homing_script_, "homing");

  for (auto& motor : motors_) {
    const sol::protected_function_result result = get_homing_angle(motor.GetNameMotor());
    if (!result.valid()) {
      const sol::error err = result;
      throw std::runtime_error("homing() failed for motor \"" + motor.GetNameMotor() + "\": " + err.what());
    }

    const sol::optional<double> angle = result;
    if (!angle.has_value()) {
      throw std::runtime_error("\"" + homing_script_ + "\" defines no angle for motor \"" + motor.GetNameMotor() +
                               "\"");
    }

    motor.SetAngle(*angle);
    WriteOnMotor(motor);
  }

  RCLCPP_INFO(get_logger(), "all %zu motors moved to their rest position", motors_.size());
}

void ServoController::RegisterMotors() {
  auto generator = LoadScript(motors_script_, "generate_motors_configuration");

  for (const auto& side : {"L", "R"}) {
    const sol::protected_function_result result = generator(motors_per_side_, side);
    if (!result.valid()) {
      const sol::error err = result;
      throw std::runtime_error(std::string("generate_motors_configuration(\"") + side + "\") failed: " + err.what());
    }
  }

  const sol::optional<sol::table> motors_table = lua_["Motors"];
  if (!motors_table.has_value() || motors_table->size() == 0) {
    throw std::runtime_error("\"" + motors_script_ +
                             "\" produced no motor: the global \"Motors\" table is missing "
                             "or empty");
  }

  for (const auto& entry : *motors_table) {
    if (entry.second.get_type() != sol::type::table) {
      throw std::runtime_error("\"" + motors_script_ + "\": every entry of \"Motors\" must be a {name, pin} table");
    }

    auto motor_entry = entry.second.as<sol::table>();
    const sol::optional<std::string> name = motor_entry[1];
    const sol::optional<int> pin = motor_entry[2];
    if (!name.has_value() || !pin.has_value()) {
      throw std::runtime_error("\"" + motors_script_ + "\": an entry of \"Motors\" is not a valid {name, pin} pair");
    }

    // The pin becomes a PCA9685 channel, so it has to exist on the board.
    if (*pin < 0 || *pin >= adafruit::pca9685::kChannelCount) {
      throw std::runtime_error("\"" + motors_script_ + "\": motor \"" + *name + "\" uses pin " + std::to_string(*pin) +
                               ", outside 0 to " + std::to_string(adafruit::pca9685::kChannelCount - 1));
    }

    motors_.emplace_back(*name, *pin);
    RCLCPP_DEBUG_STREAM(get_logger(), "registered " << motors_.back());
  }

  RCLCPP_INFO(get_logger(), "registered %zu motors from \"%s\"", motors_.size(), motors_script_.c_str());
}

void ServoController::WriteOnMotor(const Motor& t_motor) {
  auto& driver = t_motor.GetNameMotor().starts_with("L") ? servo_driver_left_ : servo_driver_right_;

  driver.SetSinglePWM(t_motor.GetPinMotor(), 0, PulseWidth(t_motor.GetAngle()));
  std::this_thread::sleep_for(settle_time_);
}

Motor* ServoController::FindMotor(const std::string& t_name) {
  // Eighteen motors: a linear scan costs less than the map that would avoid it.
  for (auto& motor : motors_) {
    if (motor.GetNameMotor() == t_name) {
      return &motor;
    }
  }
  return nullptr;
}

void ServoController::OnJointCommand(const sensor_msgs::msg::JointState& t_command) {
  // JointState carries the two arrays independently, so nothing but this check
  // stops a truncated message from pairing an angle with the wrong joint.
  if (t_command.name.size() != t_command.position.size()) {
    ++rejected_commands_;
    RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs,
                          "rejected a command with %zu names against %zu positions", t_command.name.size(),
                          t_command.position.size());
    return;
  }

  if (t_command.name.empty()) {
    ++rejected_commands_;
    RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs,
                          "rejected an empty command, there is nothing to move");
    return;
  }

  // Built into the pending buffers only after the whole message has been
  // accepted, so a bad command never half-overwrites a good pose.
  std::vector<std::string> names;
  std::vector<double> degrees;
  names.reserve(t_command.name.size());
  degrees.reserve(t_command.position.size());

  for (std::size_t index = 0; index < t_command.name.size(); ++index) {
    const auto degree = t_command.position[index] * 180.0 / std::numbers::pi;
    if (!std::isfinite(degree)) {
      ++rejected_commands_;
      RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs,
                            "rejected a command: joint %s carries a non-finite angle", t_command.name[index].c_str());
      return;
    }
    names.push_back(t_command.name[index]);
    degrees.push_back(degree);
  }

  pending_names_ = std::move(names);
  pending_degrees_ = std::move(degrees);
}

void ServoController::WritePendingPose() {
  if (pending_names_.empty()) {
    return;
  }

  // Taken by move: a pose is written once. Holding it would make the timer
  // rewrite the same angles for ever, keeping the I2C bus busy for nothing.
  const auto names = std::move(pending_names_);
  const auto degrees = std::move(pending_degrees_);
  pending_names_.clear();
  pending_degrees_.clear();

  for (std::size_t index = 0; index < names.size(); ++index) {
    auto* motor = FindMotor(names[index]);
    if (motor == nullptr) {
      // The sender and motors.lua disagree. Say so rather than moving a joint
      // that happens to sit at the same position in some other ordering.
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), kLogThrottleMs,
                           "no motor is called %s, the command for it was ignored", names[index].c_str());
      continue;
    }

    // SetAngle clamps to the mechanical range, and PulseWidth clamps again
    // before anything reaches the board.
    motor->SetAngle(degrees[index]);
    WriteOnMotor(*motor);
  }
}

std::uint16_t ServoController::PulseWidth(const double t_angle) const {
  const auto angle = std::clamp(t_angle, kMinAngleDegree, kMaxAngleDegree);
  if (std::abs(angle - t_angle) > 0.0) {
    RCLCPP_WARN(get_logger(), "angle %.1f degrees is outside [%.0f, %.0f], clamped to %.1f", t_angle, kMinAngleDegree,
                kMaxAngleDegree, angle);
  }

  const auto pulse_us = map(angle, kMinAngleDegree, kMaxAngleDegree, min_pulse_us_, max_pulse_us_);

  // Ticks of the 12-bit counter, using the frequency the board really produces.
  const auto frequency = servo_driver_left_.GetActualFrequency();
  const auto ticks = std::lround(pulse_us * 1e-6 * frequency * (adafruit::pca9685::kCounterMax + 1));

  return static_cast<std::uint16_t>(std::clamp<long>(ticks, 0, static_cast<long>(adafruit::pca9685::kCounterMax)));
}

}  // namespace hexapod
